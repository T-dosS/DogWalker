/**
 * @file arquivo.c
 * @brief Leitura e gravação do arquivo CSV de clientes.
 */
#include "arquivo.h"
#include "validacao.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define NUM_CAMPOS 9
#define TAM_LINHA_ARQUIVO 1024 /* bem maior que a soma dos campos (~460) */
#define BOM_UTF8 "\xEF\xBB\xBF" /* marca que o Excel/Bloco de Notas pode inserir */

/* ------------------------------------------------------------------ */
/* Conjunto de CPFs já lidos (detecção de duplicados)                  */
/* ------------------------------------------------------------------ */

/*
 * Ao carregar o arquivo, cada CPF precisa ser comparado com os já lidos
 * para descartar duplicados. Com busca linear isso custa O(n²): 50 mil
 * clientes levavam ~3 s para carregar. Esta tabela hash de endereçamento
 * aberto faz cada verificação em O(1) médio (50 mil clientes: ~0,1 s).
 *
 * O CPF (11 dígitos) cabe em um inteiro de 64 bits e é a própria chave.
 * O valor 0 marca posição livre: "00000000000" nunca é um CPF válido
 * (dígitos repetidos são rejeitados na validação).
 */
typedef struct {
    uint64_t *chaves;
    size_t capacidade; /* sempre potência de 2 */
    size_t usados;
} ConjuntoCpf;

#define CONJUNTO_CAPACIDADE_INICIAL 1024

static uint64_t cpf_para_numero(const char *cpf)
{
    uint64_t numero = 0;
    for (; *cpf != '\0'; cpf++) {
        numero = numero * 10u + (uint64_t)(*cpf - '0');
    }
    return numero;
}

/** Posição inicial da chave: multiplicação de Fibonacci espalha bem os bits. */
static size_t conjunto_posicao(uint64_t chave, size_t capacidade)
{
    return (size_t)((chave * UINT64_C(11400714819323198485)) >> 32) & (capacidade - 1);
}

static int conjunto_iniciar(ConjuntoCpf *conjunto, size_t capacidade)
{
    conjunto->chaves = calloc(capacidade, sizeof *conjunto->chaves);
    conjunto->capacidade = capacidade;
    conjunto->usados = 0;
    return conjunto->chaves != NULL;
}

static void conjunto_liberar(ConjuntoCpf *conjunto)
{
    free(conjunto->chaves);
    conjunto->chaves = NULL;
    conjunto->capacidade = 0;
    conjunto->usados = 0;
}

/** Coloca a chave na primeira posição livre (sondagem linear), sem checar repetição. */
static void conjunto_colocar(uint64_t *chaves, size_t capacidade, uint64_t chave)
{
    size_t i = conjunto_posicao(chave, capacidade);
    while (chaves[i] != 0) {
        i = (i + 1) & (capacidade - 1);
    }
    chaves[i] = chave;
}

/** Dobra a capacidade e redistribui as chaves. @return 1 ok, 0 sem memória. */
static int conjunto_crescer(ConjuntoCpf *conjunto)
{
    size_t nova = conjunto->capacidade * 2;
    uint64_t *novas = calloc(nova, sizeof *novas);
    size_t i;

    if (novas == NULL) {
        return 0;
    }
    for (i = 0; i < conjunto->capacidade; i++) {
        if (conjunto->chaves[i] != 0) {
            conjunto_colocar(novas, nova, conjunto->chaves[i]);
        }
    }
    free(conjunto->chaves);
    conjunto->chaves = novas;
    conjunto->capacidade = nova;
    return 1;
}

/**
 * Insere o CPF se ainda não estiver no conjunto.
 * @return 1 se inseriu, 0 se já existia, -1 se faltou memória.
 */
static int conjunto_inserir(ConjuntoCpf *conjunto, const char *cpf)
{
    uint64_t chave = cpf_para_numero(cpf);
    size_t i;

    /* Mantém a ocupação abaixo de 70% para as sondagens continuarem curtas. */
    if ((conjunto->usados + 1) * 10 > conjunto->capacidade * 7 && !conjunto_crescer(conjunto)) {
        return -1;
    }
    for (i = conjunto_posicao(chave, conjunto->capacidade); conjunto->chaves[i] != 0;
         i = (i + 1) & (conjunto->capacidade - 1)) {
        if (conjunto->chaves[i] == chave) {
            return 0;
        }
    }
    conjunto->chaves[i] = chave;
    conjunto->usados++;
    return 1;
}

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
    size_t i;
    ResultadoArquivo resultado = ARQUIVO_OK;
    ConjuntoCpf cpfs_lidos;
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

    /* Clientes que já estejam na lista também contam como "já vistos". */
    if (!conjunto_iniciar(&cpfs_lidos, CONJUNTO_CAPACIDADE_INICIAL)) {
        fclose(arquivo);
        return ARQUIVO_ERRO_MEMORIA;
    }
    for (i = 0; i < lista->quantidade; i++) {
        if (conjunto_inserir(&cpfs_lidos, lista->itens[i].cpf) < 0) {
            resultado = ARQUIVO_ERRO_MEMORIA;
            break;
        }
    }

    while (resultado == ARQUIVO_OK && fgets(linha, sizeof linha, arquivo) != NULL) {
        int novo;
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

        if (!linha_para_cliente(conteudo, &cliente)) {
            ignoradas++; /* linha inválida */
            continue;
        }
        novo = conjunto_inserir(&cpfs_lidos, cliente.cpf);
        if (novo == 0) {
            ignoradas++; /* CPF duplicado: vale a primeira ocorrência */
            continue;
        }
        if (novo < 0 || !lista_adicionar(lista, &cliente)) {
            resultado = ARQUIVO_ERRO_MEMORIA;
            break;
        }
    }

    if (resultado == ARQUIVO_OK && ferror(arquivo)) {
        resultado = ARQUIVO_ERRO_LEITURA;
    }
    fclose(arquivo);
    conjunto_liberar(&cpfs_lidos);

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
