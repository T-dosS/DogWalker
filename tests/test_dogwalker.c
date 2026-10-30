/**
 * @file test_dogwalker.c
 * @brief Testes automatizados das regras de negócio e da persistência.
 *
 * Não usa nenhuma biblioteca externa: a macro VERIFICAR conta quantas
 * verificações passaram e mostra o arquivo/linha das que falharam.
 * O programa retorna 0 se tudo passou e 1 caso contrário, para que
 * `make test` indique falha corretamente.
 *
 * As telas interativas (menu.c, login.c) são testadas manualmente;
 * o roteiro está no README.
 */
#include "arquivo.h"
#include "cliente.h"
#include "exportar.h"
#include "terminal.h"
#include "ui.h"
#include "validacao.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int total_verificacoes = 0;
static int total_falhas = 0;

#define VERIFICAR(condicao)                                                  \
    do {                                                                     \
        total_verificacoes++;                                                \
        if (!(condicao)) {                                                   \
            total_falhas++;                                                  \
            printf("    FALHOU (%s:%d): %s\n", __FILE__, __LINE__, #condicao); \
        }                                                                    \
    } while (0)

/* Arquivos temporários criados no diretório atual e apagados ao final. */
#define ARQ_TESTE      "teste_tmp_clientes.csv"
#define ARQ_INEXISTENTE "teste_tmp_nao_existe.csv"

/* CPFs válidos usados apenas como exemplo em testes. */
#define CPF_A "52998224725"
#define CPF_B "11144477735"
#define CPF_C "12345678909"

/* ------------------------------------------------------------------ */
/* Auxiliares                                                          */
/* ------------------------------------------------------------------ */

static Cliente criar_cliente(const char *cpf, const char *nome)
{
    Cliente c;
    memset(&c, 0, sizeof c);
    snprintf(c.cpf, sizeof c.cpf, "%s", cpf);
    snprintf(c.nome, sizeof c.nome, "%s", nome);
    snprintf(c.email, sizeof c.email, "%s", "cliente@example.com");
    snprintf(c.rua, sizeof c.rua, "%s", "Rua das Flores");
    snprintf(c.numero, sizeof c.numero, "%s", "120");
    snprintf(c.bairro, sizeof c.bairro, "%s", "Centro");
    snprintf(c.cidade, sizeof c.cidade, "%s", "Campinas");
    snprintf(c.estado, sizeof c.estado, "%s", "SP");
    snprintf(c.cep, sizeof c.cep, "%s", "13010000");
    return c;
}

/** Gera um CPF válido a partir de um número (implementação independente). */
static void gerar_cpf(int semente, char cpf[TAM_CPF])
{
    int soma;
    int resto;
    int i;

    snprintf(cpf, TAM_CPF, "%09d", 100000000 + semente * 7919);
    soma = 0;
    for (i = 0; i < 9; i++) {
        soma += (cpf[i] - '0') * (10 - i);
    }
    resto = (soma * 10) % 11 % 10;
    cpf[9] = (char)('0' + resto);
    soma = 0;
    for (i = 0; i < 10; i++) {
        soma += (cpf[i] - '0') * (11 - i);
    }
    resto = (soma * 10) % 11 % 10;
    cpf[10] = (char)('0' + resto);
    cpf[11] = '\0';
}

static void escrever_arquivo(const char *caminho, const char *conteudo)
{
    FILE *arquivo = fopen(caminho, "wb");
    if (arquivo != NULL) {
        fputs(conteudo, arquivo);
        fclose(arquivo);
    }
}

/* ------------------------------------------------------------------ */
/* Validação                                                           */
/* ------------------------------------------------------------------ */

static void testar_cpf(void)
{
    char saida[TAM_CPF] = "inalterado";

    VERIFICAR(normalizar_cpf("52998224725", saida, sizeof saida));
    VERIFICAR(strcmp(saida, CPF_A) == 0);
    VERIFICAR(normalizar_cpf(" 529.982.247-25 ", saida, sizeof saida));
    VERIFICAR(strcmp(saida, CPF_A) == 0);
    VERIFICAR(normalizar_cpf("111.444.777-35", saida, sizeof saida));
    VERIFICAR(normalizar_cpf("123.456.789-09", saida, sizeof saida));

    VERIFICAR(!normalizar_cpf("529.982.247-26", saida, sizeof saida)); /* dígito errado */
    VERIFICAR(!normalizar_cpf("111.111.111-11", saida, sizeof saida)); /* repetido      */
    VERIFICAR(!normalizar_cpf("00000000000", saida, sizeof saida));
    VERIFICAR(!normalizar_cpf("1234567890", saida, sizeof saida));     /* 10 dígitos    */
    VERIFICAR(!normalizar_cpf("529982247250", saida, sizeof saida));   /* 12 dígitos    */
    VERIFICAR(!normalizar_cpf("5299822472a", saida, sizeof saida));    /* letra         */
    VERIFICAR(!normalizar_cpf("", saida, sizeof saida));
    VERIFICAR(!normalizar_cpf("abc", saida, sizeof saida));

    /* contrato: entrada inválida não altera a saída */
    snprintf(saida, sizeof saida, "%s", CPF_B);
    VERIFICAR(!normalizar_cpf("123", saida, sizeof saida));
    VERIFICAR(strcmp(saida, CPF_B) == 0);
}

static void testar_email(void)
{
    char saida[TAM_EMAIL];

    VERIFICAR(normalizar_email("ana@example.com", saida, sizeof saida));
    VERIFICAR(normalizar_email("  Ana.Souza@Example.COM.br ", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "ana.souza@example.com.br") == 0);

    VERIFICAR(!normalizar_email("", saida, sizeof saida));
    VERIFICAR(!normalizar_email("ana", saida, sizeof saida));
    VERIFICAR(!normalizar_email("ana@", saida, sizeof saida));
    VERIFICAR(!normalizar_email("@example.com", saida, sizeof saida));
    VERIFICAR(!normalizar_email("ana@example", saida, sizeof saida));
    VERIFICAR(!normalizar_email("ana@.com", saida, sizeof saida));
    VERIFICAR(!normalizar_email("ana@example.", saida, sizeof saida));
    VERIFICAR(!normalizar_email("ana@@example.com", saida, sizeof saida));
    VERIFICAR(!normalizar_email("ana souza@example.com", saida, sizeof saida));
    VERIFICAR(!normalizar_email("ana..souza@example.com", saida, sizeof saida));
    VERIFICAR(!normalizar_email("ana;x@example.com", saida, sizeof saida));
}

static void testar_uf_e_cep(void)
{
    char uf[TAM_ESTADO];
    char cep[TAM_CEP];
    char formatado[TAM_CEP_FORMATADO];

    VERIFICAR(normalizar_uf("sp", uf, sizeof uf));
    VERIFICAR(strcmp(uf, "SP") == 0);
    VERIFICAR(normalizar_uf(" Df ", uf, sizeof uf));
    VERIFICAR(strcmp(uf, "DF") == 0);
    VERIFICAR(!normalizar_uf("XX", uf, sizeof uf));
    VERIFICAR(!normalizar_uf("São Paulo", uf, sizeof uf));
    VERIFICAR(!normalizar_uf("", uf, sizeof uf));

    VERIFICAR(normalizar_cep("01001-000", cep, sizeof cep));
    VERIFICAR(strcmp(cep, "01001000") == 0);
    VERIFICAR(normalizar_cep("13010000", cep, sizeof cep));
    VERIFICAR(!normalizar_cep("0100100", cep, sizeof cep));
    VERIFICAR(!normalizar_cep("01001-0000", cep, sizeof cep));
    VERIFICAR(!normalizar_cep("ABCDE-FGH", cep, sizeof cep));

    formatar_cep("01001000", formatado, sizeof formatado);
    VERIFICAR(strcmp(formatado, "01001-000") == 0);
}

static void testar_texto(void)
{
    char saida[TAM_NOME];
    char longo[200];

    VERIFICAR(normalizar_texto("  João da Silva  ", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "João da Silva") == 0);
    VERIFICAR(normalizar_texto("Ação & Cia. (Pet's) #1", saida, sizeof saida));
    VERIFICAR(!normalizar_texto("", saida, sizeof saida));
    VERIFICAR(!normalizar_texto("     ", saida, sizeof saida));
    VERIFICAR(!normalizar_texto("Ana;Souza", saida, sizeof saida));
    VERIFICAR(!normalizar_texto("Ana\tSouza", saida, sizeof saida));

    memset(longo, 'a', 100);
    longo[100] = '\0';
    VERIFICAR(normalizar_texto(longo, saida, sizeof saida)); /* exatamente 100 */
    memset(longo, 'a', 101);
    longo[101] = '\0';
    VERIFICAR(!normalizar_texto(longo, saida, sizeof saida)); /* 101: não cabe */
}

static void testar_conversao_inteiro(void)
{
    int valor = 0;

    VERIFICAR(converter_inteiro("3", &valor) && valor == 3);
    VERIFICAR(converter_inteiro("-7", &valor) && valor == -7);
    VERIFICAR(!converter_inteiro("", &valor));
    VERIFICAR(!converter_inteiro("abc", &valor));
    VERIFICAR(!converter_inteiro("3abc", &valor));
    VERIFICAR(!converter_inteiro("99999999999999999999", &valor));
}

/* ------------------------------------------------------------------ */
/* Lista em memória                                                    */
/* ------------------------------------------------------------------ */

static void testar_lista(void)
{
    ListaClientes lista;
    Cliente a = criar_cliente(CPF_A, "Ana");
    Cliente b = criar_cliente(CPF_B, "Bruno");
    Cliente c = criar_cliente(CPF_C, "Carla");
    int i;

    lista_iniciar(&lista);
    VERIFICAR(lista.quantidade == 0 && lista.itens == NULL);
    VERIFICAR(lista_buscar_cpf(&lista, CPF_A) == -1);

    VERIFICAR(lista_adicionar(&lista, &a));
    VERIFICAR(lista_adicionar(&lista, &b));
    VERIFICAR(lista_adicionar(&lista, &c));
    VERIFICAR(lista.quantidade == 3);
    VERIFICAR(lista_buscar_cpf(&lista, CPF_B) == 1);
    VERIFICAR(lista_buscar_cpf(&lista, "00000000000") == -1);

    /* remover o do meio preserva os outros, na mesma ordem */
    VERIFICAR(lista_remover(&lista, 1));
    VERIFICAR(lista.quantidade == 2);
    VERIFICAR(strcmp(lista.itens[0].nome, "Ana") == 0);
    VERIFICAR(strcmp(lista.itens[1].nome, "Carla") == 0);
    VERIFICAR(lista_buscar_cpf(&lista, CPF_B) == -1);
    VERIFICAR(!lista_remover(&lista, 5)); /* índice inválido */
    VERIFICAR(lista.quantidade == 2);

    VERIFICAR(clientes_iguais(&lista.itens[0], &a));
    VERIFICAR(!clientes_iguais(&a, &b));

    /* crescimento do vetor dinâmico além da capacidade inicial */
    for (i = 0; i < 100; i++) {
        Cliente extra = criar_cliente(CPF_A, "Extra");
        VERIFICAR(lista_adicionar(&lista, &extra));
    }
    VERIFICAR(lista.quantidade == 102);
    VERIFICAR(lista.capacidade >= lista.quantidade);

    lista_liberar(&lista);
    VERIFICAR(lista.quantidade == 0 && lista.capacidade == 0 && lista.itens == NULL);
}

/* ------------------------------------------------------------------ */
/* Arquivo                                                             */
/* ------------------------------------------------------------------ */

static void testar_arquivo_inexistente_e_vazio(void)
{
    ListaClientes lista;
    size_t ignoradas = 99;

    remove(ARQ_INEXISTENTE);
    lista_iniciar(&lista);
    VERIFICAR(arquivo_carregar(ARQ_INEXISTENTE, &lista, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(lista.quantidade == 0 && ignoradas == 0);

    escrever_arquivo(ARQ_TESTE, "");
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lista, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(lista.quantidade == 0 && ignoradas == 0);

    escrever_arquivo(ARQ_TESTE, CABECALHO_CSV "\n\n\n");
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lista, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(lista.quantidade == 0 && ignoradas == 0);

    lista_liberar(&lista);
}

static void testar_salvar_e_carregar(void)
{
    ListaClientes original;
    ListaClientes lida;
    Cliente a = criar_cliente(CPF_A, "Ana Souza");
    Cliente b = criar_cliente(CPF_B, "Bruno Lima");
    Cliente c = criar_cliente(CPF_C, "Cláudia Conceição");
    size_t ignoradas = 99;
    size_t i;

    lista_iniciar(&original);
    lista_iniciar(&lida);
    lista_adicionar(&original, &a);
    lista_adicionar(&original, &b);
    lista_adicionar(&original, &c);

    VERIFICAR(arquivo_salvar(ARQ_TESTE, &original) == ARQUIVO_OK);
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lida, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(ignoradas == 0);
    VERIFICAR(lida.quantidade == 3);
    for (i = 0; i < lida.quantidade && i < original.quantidade; i++) {
        VERIFICAR(clientes_iguais(&lida.itens[i], &original.itens[i]));
    }

    /* alteração: só o cliente escolhido muda */
    snprintf(lida.itens[1].cidade, sizeof lida.itens[1].cidade, "%s", "Santos");
    VERIFICAR(arquivo_salvar(ARQ_TESTE, &lida) == ARQUIVO_OK);
    lista_liberar(&lida);
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lida, NULL) == ARQUIVO_OK);
    VERIFICAR(lida.quantidade == 3);
    VERIFICAR(strcmp(lida.itens[1].cidade, "Santos") == 0);
    VERIFICAR(clientes_iguais(&lida.itens[0], &a));
    VERIFICAR(clientes_iguais(&lida.itens[2], &c));

    /* remoção: só o cliente escolhido sai */
    VERIFICAR(lista_remover(&lida, 0));
    VERIFICAR(arquivo_salvar(ARQ_TESTE, &lida) == ARQUIVO_OK);
    lista_liberar(&lida);
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lida, NULL) == ARQUIVO_OK);
    VERIFICAR(lida.quantidade == 2);
    VERIFICAR(lista_buscar_cpf(&lida, CPF_A) == -1);
    VERIFICAR(lista_buscar_cpf(&lida, CPF_B) == 0);
    VERIFICAR(clientes_iguais(&lida.itens[1], &c));

    /* remover o último cliente deixa um arquivo válido e vazio */
    lista_liberar(&lida);
    VERIFICAR(arquivo_salvar(ARQ_TESTE, &lida) == ARQUIVO_OK);
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lida, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(lida.quantidade == 0 && ignoradas == 0);

    lista_liberar(&original);
    lista_liberar(&lida);
}

static void testar_linhas_invalidas(void)
{
    ListaClientes lista;
    size_t ignoradas = 0;
    char linha_gigante[2000];
    char conteudo[4000];

    memset(linha_gigante, 'x', sizeof linha_gigante - 1);
    linha_gigante[sizeof linha_gigante - 1] = '\0';

    snprintf(conteudo, sizeof conteudo,
             "%s\n"
             "52998224725;Ana;ana@example.com;Rua A;1;Centro;Campinas;SP;13010000\n"
             "52998224726;CPF errado;x@example.com;Rua;1;B;C;SP;13010000\n"
             "11144477735;Campos faltando;b@example.com\n"
             "11144477735;Campo extra;b@example.com;R;1;B;C;SP;13010000;extra\n"
             "52998224725;Duplicado;d@example.com;R;1;B;C;SP;13010000\n"
             "%s\n"
             "12345678909;Carla;carla@example.com;Rua C;3;Bairro;Cidade;RJ;20000000\n",
             CABECALHO_CSV, linha_gigante);
    escrever_arquivo(ARQ_TESTE, conteudo);

    lista_iniciar(&lista);
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lista, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(lista.quantidade == 2);
    VERIFICAR(ignoradas == 5);
    VERIFICAR(lista_buscar_cpf(&lista, CPF_A) == 0);
    VERIFICAR(lista_buscar_cpf(&lista, CPF_C) == 1);
    lista_liberar(&lista);
}

static void testar_crlf_e_bom(void)
{
    ListaClientes lista;
    size_t ignoradas = 99;

    /* arquivo salvo pelo Excel/Bloco de Notas: BOM UTF-8 + quebras CRLF */
    escrever_arquivo(ARQ_TESTE,
                     "\xEF\xBB\xBF" CABECALHO_CSV "\r\n"
                     "52998224725;Ana;ana@example.com;Rua A;1;Centro;Campinas;SP;13010000\r\n");
    lista_iniciar(&lista);
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lista, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(lista.quantidade == 1 && ignoradas == 0);
    VERIFICAR(lista.quantidade == 1 && strcmp(lista.itens[0].cep, "13010000") == 0);
    lista_liberar(&lista);
}

static void testar_pasta_inexistente(void)
{
    ListaClientes lista;
    Cliente a = criar_cliente(CPF_A, "Ana");

    lista_iniciar(&lista);
    lista_adicionar(&lista, &a);
    VERIFICAR(arquivo_salvar("pasta_que_nao_existe_123/clientes.csv", &lista)
              == ARQUIVO_ERRO_ESCRITA);
    lista_liberar(&lista);
}

static void testar_muitos_clientes(void)
{
    ListaClientes lista;
    ListaClientes lida;
    size_t ignoradas = 99;
    int i;

    lista_iniciar(&lista);
    lista_iniciar(&lida);
    for (i = 1; i <= 500; i++) {
        char nome[TAM_NOME];
        Cliente c;
        char cpf[TAM_CPF];

        gerar_cpf(i, cpf);
        snprintf(nome, sizeof nome, "Cliente %d", i);
        c = criar_cliente(cpf, nome);
        VERIFICAR(cpf_digitos_validos(cpf));
        VERIFICAR(lista_adicionar(&lista, &c));
    }
    VERIFICAR(arquivo_salvar(ARQ_TESTE, &lista) == ARQUIVO_OK);
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lida, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(lida.quantidade == 500 && ignoradas == 0);
    VERIFICAR(lida.quantidade == 500
              && strcmp(lida.itens[499].nome, "Cliente 500") == 0);

    lista_liberar(&lista);
    lista_liberar(&lida);
}

/*
 * A detecção de CPF duplicado na carga usa uma tabela hash que cresce
 * conforme a ocupação. Com 3.000 clientes ela cresce algumas vezes; as
 * repetições ficam no fim do arquivo, depois dos crescimentos.
 */
static void testar_duplicados_em_massa(void)
{
    ListaClientes lista;
    ListaClientes lida;
    size_t ignoradas = 0;
    int i;

    lista_iniciar(&lista);
    lista_iniciar(&lida);
    for (i = 1; i <= 3000; i++) {
        char nome[TAM_NOME];
        char cpf[TAM_CPF];
        Cliente c;

        gerar_cpf(i, cpf);
        snprintf(nome, sizeof nome, "Cliente %d", i);
        c = criar_cliente(cpf, nome);
        lista_adicionar(&lista, &c);
    }
    /* 100 repetições dos primeiros CPFs, com outro nome */
    for (i = 1; i <= 100; i++) {
        char cpf[TAM_CPF];
        Cliente c;

        gerar_cpf(i, cpf);
        c = criar_cliente(cpf, "Repetido");
        lista_adicionar(&lista, &c);
    }

    VERIFICAR(arquivo_salvar(ARQ_TESTE, &lista) == ARQUIVO_OK);
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lida, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(lida.quantidade == 3000);
    VERIFICAR(ignoradas == 100);
    /* vale a primeira ocorrência e a ordem do arquivo é mantida */
    VERIFICAR(lida.quantidade == 3000 && strcmp(lida.itens[0].nome, "Cliente 1") == 0);
    VERIFICAR(lida.quantidade == 3000 && strcmp(lida.itens[2999].nome, "Cliente 3000") == 0);
    {
        int repetido_carregado = 0;
        for (i = 0; i < (int)lida.quantidade; i++) {
            if (strcmp(lida.itens[i].nome, "Repetido") == 0) {
                repetido_carregado = 1;
            }
        }
        VERIFICAR(!repetido_carregado);
    }

    /* clientes já presentes na lista também contam como vistos */
    lista_liberar(&lida);
    lista_iniciar(&lida);
    {
        char cpf[TAM_CPF];
        Cliente c;
        gerar_cpf(1, cpf);
        c = criar_cliente(cpf, "Já na lista");
        lista_adicionar(&lida, &c);
    }
    VERIFICAR(arquivo_carregar(ARQ_TESTE, &lida, &ignoradas) == ARQUIVO_OK);
    VERIFICAR(lida.quantidade == 3000);
    VERIFICAR(ignoradas == 101);
    VERIFICAR(strcmp(lida.itens[0].nome, "Já na lista") == 0);

    lista_liberar(&lista);
    lista_liberar(&lida);
}

/* ------------------------------------------------------------------ */
/* Filtro da listagem e alinhamento da interface                       */
/* ------------------------------------------------------------------ */

static void testar_filtro(void)
{
    Cliente c = criar_cliente(CPF_A, "Ana Souza");
    snprintf(c.cidade, sizeof c.cidade, "%s", "São Paulo");
    snprintf(c.email, sizeof c.email, "%s", "ana.souza@example.com");

    VERIFICAR(cliente_corresponde(&c, NULL));
    VERIFICAR(cliente_corresponde(&c, ""));
    VERIFICAR(cliente_corresponde(&c, "ana"));          /* sem diferenciar caixa */
    VERIFICAR(cliente_corresponde(&c, "SOUZA"));
    VERIFICAR(cliente_corresponde(&c, "são"));          /* acento igual          */
    VERIFICAR(cliente_corresponde(&c, "example.com"));  /* e-mail                */
    VERIFICAR(cliente_corresponde(&c, "sp"));           /* UF                    */
    VERIFICAR(cliente_corresponde(&c, "529.982"));      /* CPF com máscara       */
    VERIFICAR(cliente_corresponde(&c, "24725"));        /* fim do CPF            */
    VERIFICAR(!cliente_corresponde(&c, "Bruno"));
    VERIFICAR(!cliente_corresponde(&c, "111.444"));
    VERIFICAR(!cliente_corresponde(&c, "529982247250000")); /* dígitos demais */

    /* Acentos e caixa são ignorados nos dois sentidos. */
    VERIFICAR(cliente_corresponde(&c, "sao"));
    VERIFICAR(cliente_corresponde(&c, "SÃO PAULO"));
    VERIFICAR(cliente_corresponde(&c, "Paulo"));
    VERIFICAR(!cliente_corresponde(&c, "são joão"));
    snprintf(c.nome, sizeof c.nome, "%s", "Cláudia Conceição");
    VERIFICAR(cliente_corresponde(&c, "claudia conceicao"));
    VERIFICAR(cliente_corresponde(&c, "ÇÃO"));
}

static void testar_texto_para_comparacao(void)
{
    char saida[64];
    char pequeno[4];

    texto_para_comparacao("São João", saida, sizeof saida);
    VERIFICAR(strcmp(saida, "sao joao") == 0);
    texto_para_comparacao("ÁÀÂÃÄ áàâãä ÉÊÍÓÔÕÚÜ Çç Ññ", saida, sizeof saida);
    VERIFICAR(strcmp(saida, "aaaaa aaaaa eeiooouu cc nn") == 0);
    texto_para_comparacao("ANA@Example.COM", saida, sizeof saida);
    VERIFICAR(strcmp(saida, "ana@example.com") == 0);
    texto_para_comparacao("", saida, sizeof saida);
    VERIFICAR(strcmp(saida, "") == 0);

    /* Sem letra base: o caractere é mantido como está. */
    texto_para_comparacao("Æ×ß─", saida, sizeof saida);
    VERIFICAR(strcmp(saida, "Æ×ß─") == 0);

    /* Truncamento nunca corta um caractere de vários bytes ao meio. */
    texto_para_comparacao("Ação", pequeno, sizeof pequeno);
    VERIFICAR(strcmp(pequeno, "aca") == 0);
    texto_para_comparacao("ab─", pequeno, sizeof pequeno);
    VERIFICAR(strcmp(pequeno, "ab") == 0);

    /* UTF-8 malformado (byte inicial no fim do texto) não lê além do '\0'. */
    texto_para_comparacao("a\xC3", saida, sizeof saida);
    VERIFICAR(strcmp(saida, "a\xC3") == 0);
}

static void testar_ordenacao(void)
{
    ListaClientes lista;
    const Cliente **visao;
    Cliente c;

    lista_iniciar(&lista);
    VERIFICAR(lista_criar_visao(&lista, ORDEM_NOME) == NULL); /* lista vazia */

    c = criar_cliente(CPF_A, "bruno Lima");
    snprintf(c.cidade, sizeof c.cidade, "%s", "Rio de Janeiro");
    snprintf(c.estado, sizeof c.estado, "%s", "RJ");
    lista_adicionar(&lista, &c);
    c = criar_cliente(CPF_B, "Álvaro Dias");
    snprintf(c.cidade, sizeof c.cidade, "%s", "São Paulo");
    lista_adicionar(&lista, &c);
    c = criar_cliente(CPF_C, "Ana Souza");
    snprintf(c.cidade, sizeof c.cidade, "%s", "Campinas");
    lista_adicionar(&lista, &c);
    c = criar_cliente(CPF_A, "Ana Souza"); /* nome repetido: desempate */
    snprintf(c.cidade, sizeof c.cidade, "%s", "Santos");
    lista_adicionar(&lista, &c);

    visao = lista_criar_visao(&lista, ORDEM_CADASTRO);
    VERIFICAR(visao != NULL);
    if (visao != NULL) {
        VERIFICAR(visao[0] == &lista.itens[0] && visao[3] == &lista.itens[3]);
        free(visao);
    }

    /* "Álvaro" fica entre os "A" e "bruno" minúsculo não vai para o fim. */
    visao = lista_criar_visao(&lista, ORDEM_NOME);
    VERIFICAR(visao != NULL);
    if (visao != NULL) {
        VERIFICAR(visao[0] == &lista.itens[1]); /* Álvaro          */
        VERIFICAR(visao[1] == &lista.itens[2]); /* Ana (cadastrada antes) */
        VERIFICAR(visao[2] == &lista.itens[3]); /* Ana             */
        VERIFICAR(visao[3] == &lista.itens[0]); /* bruno           */
        free(visao);
    }

    /* Cidade: Campinas, Rio de Janeiro, Santos, São Paulo. */
    visao = lista_criar_visao(&lista, ORDEM_CIDADE);
    VERIFICAR(visao != NULL);
    if (visao != NULL) {
        VERIFICAR(visao[0] == &lista.itens[2]);
        VERIFICAR(visao[1] == &lista.itens[0]);
        VERIFICAR(visao[2] == &lista.itens[3]);
        VERIFICAR(visao[3] == &lista.itens[1]);
        free(visao);
    }

    /* A ordenação não altera a lista (nem o que será gravado no CSV). */
    VERIFICAR(strcmp(lista.itens[0].nome, "bruno Lima") == 0);
    VERIFICAR(strcmp(lista.itens[1].nome, "Álvaro Dias") == 0);
    lista_liberar(&lista);
}

static void testar_largura_e_truncamento(void)
{
    char saida[64];

    /* Os testes não chamam ui_iniciar(): a interface fica no modo simples. */
    VERIFICAR(!ui_modo_rico());
    VERIFICAR(strcmp(ui_codigo(UI_COR_ERRO), "") == 0);

    VERIFICAR(ui_largura_texto("") == 0);
    VERIFICAR(ui_largura_texto("Ana") == 3);
    VERIFICAR(ui_largura_texto("Conceição") == 9); /* 11 bytes, 9 colunas */
    VERIFICAR(ui_largura_texto("─│╭") == 3);       /* bordas: 3 bytes cada */

    ui_truncar("Ana", 10, saida, sizeof saida);
    VERIFICAR(strcmp(saida, "Ana") == 0);
    ui_truncar("Cláudia Conceição", 17, saida, sizeof saida);
    VERIFICAR(strcmp(saida, "Cláudia Conceição") == 0); /* cabe exatamente */
    ui_truncar("Cláudia Conceição", 10, saida, sizeof saida);
    VERIFICAR(strcmp(saida, "Cláudia...") == 0);
    VERIFICAR(ui_largura_texto(saida) == 10);
    ui_truncar("ççççç", 4, saida, sizeof saida);     /* não corta "ç" ao meio */
    VERIFICAR(strcmp(saida, "ç...") == 0);
}

/* ------------------------------------------------------------------ */
/* Exportação (relatório HTML e planilha)                              */
/* ------------------------------------------------------------------ */

#define ARQ_HTML "teste_tmp_relatorio.html"
#define ARQ_PLANILHA "teste_tmp_planilha.csv"

/** Lê o arquivo inteiro para `destino` (até tamanho-1 bytes). @return bytes lidos. */
static size_t ler_arquivo(const char *caminho, char *destino, size_t tamanho)
{
    FILE *arquivo = fopen(caminho, "rb");
    size_t lidos = 0;

    destino[0] = '\0';
    if (arquivo != NULL) {
        lidos = fread(destino, 1, tamanho - 1, arquivo);
        destino[lidos] = '\0';
        fclose(arquivo);
    }
    return lidos;
}

static void testar_escapes_de_exportacao(void)
{
    char saida[128];
    char pequeno[8];

    /* HTML: os 5 caracteres especiais viram entidades; o resto não muda */
    VERIFICAR(exportar_escapar_html("Ana & Bia", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "Ana &amp; Bia") == 0);
    VERIFICAR(exportar_escapar_html("<script>alert('x')</script>", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "&lt;script&gt;alert(&#39;x&#39;)&lt;/script&gt;") == 0);
    VERIFICAR(exportar_escapar_html("\"São João\"", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "&quot;São João&quot;") == 0);
    /* sem espaço: para antes da entidade, nunca a corta ao meio */
    VERIFICAR(!exportar_escapar_html("ab&cd", pequeno, 5));
    VERIFICAR(strcmp(pequeno, "ab") == 0);

    /* Planilha: só os inícios perigosos ganham apóstrofo */
    VERIFICAR(exportar_valor_planilha("=HIPERLINK(\"x\")", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "'=HIPERLINK(\"x\")") == 0);
    VERIFICAR(exportar_valor_planilha("+55 19", saida, sizeof saida) && saida[0] == '\'');
    VERIFICAR(exportar_valor_planilha("-1", saida, sizeof saida) && saida[0] == '\'');
    VERIFICAR(exportar_valor_planilha("@soma", saida, sizeof saida) && saida[0] == '\'');
    VERIFICAR(exportar_valor_planilha("Ana Souza", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "Ana Souza") == 0);
    VERIFICAR(exportar_valor_planilha("a=b", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "a=b") == 0); /* só o primeiro caractere importa */

    /* CPF mascarado e caminho do arquivo na mesma pasta dos dados */
    exportar_mascarar_cpf("52998224725", saida, sizeof saida);
    VERIFICAR(strcmp(saida, "***.982.247-**") == 0);
    VERIFICAR(exportar_caminho_vizinho("data/clientes.csv", "relatorio.html", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "data/relatorio.html") == 0);
    VERIFICAR(exportar_caminho_vizinho("C:\\dados\\c.csv", "x.csv", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "C:\\dados\\x.csv") == 0);
    VERIFICAR(exportar_caminho_vizinho("clientes.csv", "relatorio.html", saida, sizeof saida));
    VERIFICAR(strcmp(saida, "relatorio.html") == 0);
    VERIFICAR(!exportar_caminho_vizinho("data/clientes.csv", "relatorio.html", pequeno,
                                        sizeof pequeno));
}

static void testar_arquivos_exportados(void)
{
    static char conteudo[32768];
    ListaClientes lista;
    Cliente a = criar_cliente(CPF_A, "Ana <b>Souza</b>");
    Cliente b = criar_cliente(CPF_B, "=SOMA(A1)");
    Cliente c = criar_cliente(CPF_C, "Cláudia");
    const Cliente *selecao[2];

    lista_iniciar(&lista);
    lista_adicionar(&lista, &a);
    lista_adicionar(&lista, &b);
    lista_adicionar(&lista, &c);
    /* recorte: só o 3º e o 2º cliente, nessa ordem (como um filtro + ordem) */
    selecao[0] = &lista.itens[2];
    selecao[1] = &lista.itens[1];

    /* HTML */
    VERIFICAR(exportar_html(ARQ_HTML, &lista, selecao, 2, "Recorte de <teste>") == ARQUIVO_OK);
    ler_arquivo(ARQ_HTML, conteudo, sizeof conteudo);
    VERIFICAR(strstr(conteudo, "<!DOCTYPE html>") == conteudo);
    VERIFICAR(strstr(conteudo, "lang=\"pt-BR\"") != NULL);
    VERIFICAR(strstr(conteudo, "name=\"viewport\"") != NULL);
    VERIFICAR(strstr(conteudo, "prefers-color-scheme:dark") != NULL);
    VERIFICAR(strstr(conteudo, "<caption>Recorte de &lt;teste&gt;</caption>") != NULL);
    VERIFICAR(strstr(conteudo, "<th scope=\"col\">Nome</th>") != NULL);
    VERIFICAR(strstr(conteudo, "<script") == NULL);     /* nenhum script */
    VERIFICAR(strstr(conteudo, "Ana &lt;b&gt;") == NULL); /* Ana não está no recorte */
    VERIFICAR(strstr(conteudo, ">Cláudia<") != NULL);
    VERIFICAR(strstr(conteudo, "***.456.789-**") != NULL); /* CPF mascarado */
    VERIFICAR(strstr(conteudo, "123.456.789-09") == NULL);  /* nunca completo */
    /* "Nº" é a posição de cadastro (3 e 2), não a ordem no recorte */
    VERIFICAR(strstr(conteudo, "<td data-rotulo=\"Nº\" class=\"num\">3</td>") != NULL);
    VERIFICAR(strstr(conteudo, "<td data-rotulo=\"Nº\" class=\"num\">2</td>") != NULL);
    VERIFICAR(strstr(conteudo, "<strong>2</strong>no relatório") != NULL);
    VERIFICAR(strstr(conteudo, "<strong>3</strong>cadastrados") != NULL);

    /* nome com tags é escapado quando está no recorte */
    selecao[0] = &lista.itens[0];
    VERIFICAR(exportar_html(ARQ_HTML, &lista, selecao, 1, NULL) == ARQUIVO_OK);
    ler_arquivo(ARQ_HTML, conteudo, sizeof conteudo);
    VERIFICAR(strstr(conteudo, "Ana &lt;b&gt;Souza&lt;/b&gt;") != NULL);
    VERIFICAR(strstr(conteudo, "<b>Souza") == NULL);

    /* relatório vazio continua válido */
    VERIFICAR(exportar_html(ARQ_HTML, &lista, selecao, 0, NULL) == ARQUIVO_OK);
    ler_arquivo(ARQ_HTML, conteudo, sizeof conteudo);
    VERIFICAR(strstr(conteudo, "Nenhum cliente neste recorte.") != NULL);

    /* Planilha: BOM, cabeçalho, CPF formatado e fórmula neutralizada */
    selecao[0] = &lista.itens[0];
    selecao[1] = &lista.itens[1];
    VERIFICAR(exportar_csv(ARQ_PLANILHA, selecao, 2) == ARQUIVO_OK);
    ler_arquivo(ARQ_PLANILHA, conteudo, sizeof conteudo);
    VERIFICAR(strncmp(conteudo, "\xEF\xBB\xBF" "Nome;CPF;", 12) == 0);
    VERIFICAR(strstr(conteudo, "\r\n'=SOMA(A1);111.444.777-35;") != NULL);
    VERIFICAR(strstr(conteudo, "\r\nAna <b>Souza</b>;529.982.247-25;") != NULL);
    VERIFICAR(strstr(conteudo, "13010-000\r\n") != NULL); /* CRLF em qualquer sistema */

    /* pasta inexistente: erro, sem travar */
    VERIFICAR(exportar_html("pasta_que_nao_existe_123/r.html", &lista, selecao, 1, NULL)
              == ARQUIVO_ERRO_ESCRITA);
    VERIFICAR(exportar_csv("pasta_que_nao_existe_123/p.csv", selecao, 1)
              == ARQUIVO_ERRO_ESCRITA);

    lista_liberar(&lista);
    remove(ARQ_HTML);
    remove(ARQ_PLANILHA);
}

/* ------------------------------------------------------------------ */

typedef struct {
    const char *nome;
    void (*executar)(void);
} CasoDeTeste;

int main(void)
{
    const CasoDeTeste casos[] = {
        {"CPF: máscara, dígitos verificadores e rejeições", testar_cpf},
        {"E-mail: formatos válidos e inválidos", testar_email},
        {"UF e CEP", testar_uf_e_cep},
        {"Texto: vazio, ';', controle e limite de tamanho", testar_texto},
        {"Conversão segura de inteiros (opção do menu)", testar_conversao_inteiro},
        {"Lista: adicionar, buscar, remover e crescer", testar_lista},
        {"Arquivo inexistente e arquivo vazio", testar_arquivo_inexistente_e_vazio},
        {"Salvar/carregar, alterar e remover persistindo", testar_salvar_e_carregar},
        {"Linhas inválidas, duplicadas e gigantes são ignoradas", testar_linhas_invalidas},
        {"Arquivo com BOM UTF-8 e quebras CRLF", testar_crlf_e_bom},
        {"Gravação em pasta inexistente retorna erro", testar_pasta_inexistente},
        {"500 clientes: gravação e leitura", testar_muitos_clientes},
        {"3.000 clientes com CPFs repetidos: carga descarta só os repetidos", testar_duplicados_em_massa},
        {"Filtro da listagem: nome, cidade, UF, e-mail, CPF e acentos", testar_filtro},
        {"Texto para comparação: minúsculas, sem acentos e UTF-8 seguro", testar_texto_para_comparacao},
        {"Ordenação da listagem por nome e cidade", testar_ordenacao},
        {"Interface: largura UTF-8 e truncamento sem cortar acentos", testar_largura_e_truncamento},
        {"Exportação: escape HTML, proteção contra fórmulas e CPF mascarado", testar_escapes_de_exportacao},
        {"Exportação: conteúdo do relatório HTML e da planilha", testar_arquivos_exportados},
    };
    size_t i;

    terminal_configurar(); /* acentos corretos no console do Windows */
    printf("Executando testes do DogWalker...\n\n");
    for (i = 0; i < sizeof casos / sizeof casos[0]; i++) {
        int falhas_antes = total_falhas;
        casos[i].executar();
        /* casos[i].executar() altera total_falhas (variável global); o
         * cppcheck não acompanha a chamada por ponteiro de função. */
        // cppcheck-suppress knownConditionTrueFalse
        printf("  [%s] %s\n", total_falhas == falhas_antes ? " OK " : "FALHA", casos[i].nome);
    }

    remove(ARQ_TESTE);
    remove(ARQ_INEXISTENTE);

    printf("\n%d verificações, %d falha(s).\n", total_verificacoes, total_falhas);
    return total_falhas == 0 ? 0 : 1;
}
