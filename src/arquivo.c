/**
 * @file arquivo.c
 * @brief Leitura e gravação do arquivo CSV de clientes.
 */
#include "arquivo.h"
#include "validacao.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define NUM_CAMPOS 9
#define TAM_LINHA_ARQUIVO 1024 /* bem maior que a soma dos campos (~460) */
#define BOM_UTF8 "\xEF\xBB\xBF" /* marca que o Excel/Bloco de Notas pode inserir */

/* ------------------------------------------------------------------ */
/* Leitura                                                             */
/* ------------------------------------------------------------------ */

/** Consome o restante de uma linha que não coube no buffer. */
static void descartar_resto_da_linha(FILE *arquivo)
{
    int c;
    do {
        c = fgetc(arquivo);
    } while (c != '\n' && c != EOF);
}

/**
 * Divide a linha nos separadores ';', trocando-os por '\0' (a linha é
 * modificada) e guardando o início de cada campo em `campos`.
 *
 * Não usamos strtok porque ela pula campos vazios (";;"), o que faria
 * uma linha com campo faltando parecer válida com os dados deslocados.
 *
 * @return a quantidade de campos, ou -1 se houver mais que `maximo`.
 */
static int dividir_campos(char *linha, char *campos[], int maximo)
{
    int quantidade = 0;
    char *inicio = linha;
    char *p;

    for (p = linha;; p++) {
        if (*p == ';' || *p == '\0') {
            int fim_da_linha = (*p == '\0');
            if (quantidade == maximo) {
                return -1;
            }
            *p = '\0';
            campos[quantidade++] = inicio;
            if (fim_da_linha) {
                return quantidade;
            }
            inicio = p + 1;
        }
    }
}

/**
 * Converte uma linha do CSV em Cliente.
 *
 * Os dados do arquivo passam pelas MESMAS validações da digitação. Assim,
 * mesmo que alguém edite o CSV à mão, nenhum dado inválido entra no sistema.
 *
 * @return 1 se a linha for válida, 0 caso contrário.
 */
static int linha_para_cliente(char *linha, Cliente *cliente)
{
    char *campos[NUM_CAMPOS];

    if (dividir_campos(linha, campos, NUM_CAMPOS) != NUM_CAMPOS) {
        return 0;
    }
    return normalizar_cpf(campos[0], cliente->cpf, sizeof cliente->cpf)
        && normalizar_texto(campos[1], cliente->nome, sizeof cliente->nome)
        && normalizar_email(campos[2], cliente->email, sizeof cliente->email)
        && normalizar_texto(campos[3], cliente->rua, sizeof cliente->rua)
        && normalizar_texto(campos[4], cliente->numero, sizeof cliente->numero)
        && normalizar_texto(campos[5], cliente->bairro, sizeof cliente->bairro)
        && normalizar_texto(campos[6], cliente->cidade, sizeof cliente->cidade)
        && normalizar_uf(campos[7], cliente->estado, sizeof cliente->estado)
        && normalizar_cep(campos[8], cliente->cep, sizeof cliente->cep);
}

ResultadoArquivo arquivo_carregar(const char *caminho, ListaClientes *lista,
                                  size_t *linhas_ignoradas)
{
    char linha[TAM_LINHA_ARQUIVO];
    size_t numero_linha = 0;
    size_t ignoradas = 0;
    ResultadoArquivo resultado = ARQUIVO_OK;
    FILE *arquivo;

    if (linhas_ignoradas != NULL) {
        *linhas_ignoradas = 0;
    }

    errno = 0;
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL) {
        /* Primeiro uso do sistema: o arquivo ainda não existe. Isso não é
         * um erro, apenas significa que não há clientes cadastrados. */
        return (errno == ENOENT) ? ARQUIVO_OK : ARQUIVO_ERRO_LEITURA;
    }

    while (fgets(linha, sizeof linha, arquivo) != NULL) {
        Cliente cliente;
        char *conteudo = linha;
        char *quebra = strchr(linha, '\n');
        size_t comprimento;

        numero_linha++;

        if (quebra == NULL && !feof(arquivo)) {
            descartar_resto_da_linha(arquivo); /* linha grande demais */
            ignoradas++;
            continue;
        }
        if (quebra != NULL) {
            *quebra = '\0';
        }
        comprimento = strlen(conteudo);
        if (comprimento > 0 && conteudo[comprimento - 1] == '\r') {
            conteudo[comprimento - 1] = '\0'; /* arquivo salvo com CRLF */
        }
        if (numero_linha == 1 && strncmp(conteudo, BOM_UTF8, 3) == 0) {
            conteudo += 3;
        }

        if (conteudo[0] == '\0') {
            continue; /* linhas em branco são simplesmente puladas */
        }
        if (numero_linha == 1 && strcmp(conteudo, CABECALHO_CSV) == 0) {
            continue;
        }

        if (!linha_para_cliente(conteudo, &cliente)
            || lista_buscar_cpf(lista, cliente.cpf) >= 0) {
            ignoradas++; /* inválida ou CPF duplicado */
            continue;
        }

        if (!lista_adicionar(lista, &cliente)) {
            resultado = ARQUIVO_ERRO_MEMORIA;
            break;
        }
    }

    if (resultado == ARQUIVO_OK && ferror(arquivo)) {
        resultado = ARQUIVO_ERRO_LEITURA;
    }
    fclose(arquivo);

    if (resultado != ARQUIVO_OK) {
        lista_liberar(lista); /* não devolve uma lista pela metade */
    } else if (linhas_ignoradas != NULL) {
        *linhas_ignoradas = ignoradas;
    }
    return resultado;
}

/* ------------------------------------------------------------------ */
/* Gravação                                                            */
/* ------------------------------------------------------------------ */

/**
 * Troca o arquivo original pelo temporário.
 *
 * No Linux/macOS, rename() substitui o destino de forma atômica.
 * No Windows, rename() falha se o destino existir, então usamos
 * MoveFileExA com MOVEFILE_REPLACE_EXISTING, que faz a mesma coisa.
 *
 * No Windows, antivírus e indexador de pesquisa costumam abrir por alguns
 * milissegundos o arquivo recém-gravado, e a troca falha com "acesso
 * negado" de forma intermitente (observado nos testes: ~2% das gravações).
 * Por isso tentamos novamente algumas vezes, só para esses erros transitórios.
 */
static int substituir_arquivo(const char *origem, const char *destino)
{
#ifdef _WIN32
    const int max_tentativas = 20;
    int tentativa;

    for (tentativa = 1; tentativa <= max_tentativas; tentativa++) {
        DWORD erro;

        if (MoveFileExA(origem, destino,
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            return 1;
        }
        erro = GetLastError();
        if (erro != ERROR_ACCESS_DENIED && erro != ERROR_SHARING_VIOLATION) {
            return 0; /* erro permanente (pasta inexistente, disco cheio...) */
        }
        Sleep(50); /* espera o outro processo liberar o arquivo */
    }
    return 0;
#else
    return rename(origem, destino) == 0;
#endif
}

ResultadoArquivo arquivo_salvar(const char *caminho, const ListaClientes *lista)
{
    char temporario[FILENAME_MAX];
    int escrito;
    int sucesso;
    size_t i;
    FILE *arquivo;

    escrito = snprintf(temporario, sizeof temporario, "%s.tmp", caminho);
    if (escrito < 0 || (size_t)escrito >= sizeof temporario) {
        return ARQUIVO_ERRO_ESCRITA;
    }

    arquivo = fopen(temporario, "w");
    if (arquivo == NULL) {
        return ARQUIVO_ERRO_ESCRITA;
    }

    sucesso = fprintf(arquivo, "%s\n", CABECALHO_CSV) >= 0;
    for (i = 0; sucesso && i < lista->quantidade; i++) {
        const Cliente *c = &lista->itens[i];
        sucesso = fprintf(arquivo, "%s;%s;%s;%s;%s;%s;%s;%s;%s\n",
                          c->cpf, c->nome, c->email, c->rua, c->numero,
                          c->bairro, c->cidade, c->estado, c->cep) >= 0;
    }

    /* fclose também pode falhar (ex.: disco cheio ao descarregar o buffer),
     * por isso o retorno dele faz parte da verificação de sucesso. */
    if (fclose(arquivo) != 0) {
        sucesso = 0;
    }

    if (!sucesso || !substituir_arquivo(temporario, caminho)) {
        remove(temporario); /* o arquivo original não foi tocado */
        return ARQUIVO_ERRO_ESCRITA;
    }
    return ARQUIVO_OK;
}

const char *arquivo_descrever_erro(ResultadoArquivo resultado)
{
    switch (resultado) {
    case ARQUIVO_OK:
        return "operação concluída";
    case ARQUIVO_ERRO_LEITURA:
        return "não foi possível ler o arquivo de dados";
    case ARQUIVO_ERRO_ESCRITA:
        return "não foi possível gravar o arquivo de dados "
               "(verifique se a pasta existe e se há permissão de escrita)";
    case ARQUIVO_ERRO_MEMORIA:
        return "memória insuficiente para carregar os clientes";
    }
    return "erro desconhecido";
}
