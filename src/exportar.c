/**
 * @file exportar.c
 * @brief Relatório HTML5/CSS3 e planilha CSV a partir da lista de clientes.
 */
#include "exportar.h"
#include "validacao.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define BOM_UTF8 "\xEF\xBB\xBF"

/* ------------------------------------------------------------------ */
/* Funções de texto (testáveis)                                        */
/* ------------------------------------------------------------------ */

/** @return a entidade HTML do caractere, ou NULL se ele não precisa de escape. */
static const char *entidade_html(char c)
{
    switch (c) {
    case '&':
        return "&amp;";
    case '<':
        return "&lt;";
    case '>':
        return "&gt;";
    case '"':
        return "&quot;";
    case '\'':
        return "&#39;";
    default:
        return NULL;
    }
}

int exportar_escapar_html(const char *texto, char *saida, size_t tamanho)
{
    size_t usado = 0;

    if (tamanho == 0) {
        return 0;
    }
    for (; *texto != '\0'; texto++) {
        const char *entidade = entidade_html(*texto);
        size_t n = (entidade != NULL) ? strlen(entidade) : 1;

        if (usado + n >= tamanho) {
            saida[usado] = '\0';
            return 0; /* não cabe: para antes, sem cortar a entidade */
        }
        if (entidade != NULL) {
            memcpy(saida + usado, entidade, n);
        } else {
            saida[usado] = *texto;
        }
        usado += n;
    }
    saida[usado] = '\0';
    return 1;
}

int exportar_valor_planilha(const char *valor, char *saida, size_t tamanho)
{
    /* Planilhas tratam como fórmula o que começa com estes caracteres;
     * o apóstrofo força o valor a ser lido como texto. */
    int perigoso = valor[0] == '=' || valor[0] == '+' || valor[0] == '-'
                || valor[0] == '@' || valor[0] == '\t' || valor[0] == '\r';
    int escrito = snprintf(saida, tamanho, "%s%s", perigoso ? "'" : "", valor);

    return escrito >= 0 && (size_t)escrito < tamanho;
}

void exportar_mascarar_cpf(const char *cpf, char *saida, size_t tamanho)
{
    /* Padrão comum de mascaramento: mostra só os 6 dígitos do meio. */
    snprintf(saida, tamanho, "***.%.3s.%.3s-**", cpf + 3, cpf + 6);
}

int exportar_caminho_vizinho(const char *caminho_dados, const char *nome,
                             char *saida, size_t tamanho)
{
    const char *p;
    size_t pasta = 0; /* tamanho do trecho até a última barra, inclusive */
    int escrito;

    for (p = caminho_dados; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            pasta = (size_t)(p - caminho_dados) + 1;
        }
    }
    escrito = snprintf(saida, tamanho, "%.*s%s", (int)pasta, caminho_dados, nome);
    return escrito >= 0 && (size_t)escrito < tamanho;
}

/* ------------------------------------------------------------------ */
/* Relatório HTML                                                      */
/* ------------------------------------------------------------------ */

static void escrever_escapado(FILE *arquivo, const char *texto)
{
    for (; *texto != '\0'; texto++) {
        const char *entidade = entidade_html(*texto);
        if (entidade != NULL) {
            fputs(entidade, arquivo);
        } else {
            fputc(*texto, arquivo);
        }
    }
}

/*
 * Estilo embutido (sem CDN, funciona offline). Pontos de design:
 * - variáveis CSS com tema claro e escuro (prefers-color-scheme);
 * - contraste AA nas cores de texto;
 * - em telas estreitas cada linha vira um "card", com o rótulo de cada
 *   coluna vindo do atributo data-rotulo (a tabela continua semântica);
 * - @media print remove fundos e sombras para impressão limpa.
 */
static const char ESTILO[] =
    ":root{--fundo:#f5f7fa;--cartao:#fff;--texto:#1c2330;--suave:#55607a;"
    "--marca:#0b7285;--borda:#d5dbe5;--zebra:#f0f4f8}"
    "@media (prefers-color-scheme:dark){:root{--fundo:#11151c;--cartao:#1a202b;"
    "--texto:#e6eaf2;--suave:#a3adc2;--marca:#5fd3e6;--borda:#2c3545;--zebra:#202837}}"
    "*{box-sizing:border-box}"
    "body{margin:0;background:var(--fundo);color:var(--texto);"
    "font:16px/1.5 system-ui,-apple-system,'Segoe UI',Roboto,sans-serif}"
    "main{max-width:1100px;margin:0 auto;padding:24px 16px}"
    "header{border-left:6px solid var(--marca);padding:4px 0 4px 16px;margin-bottom:20px}"
    "h1{margin:0;font-size:1.6rem;color:var(--marca)}"
    "header p{margin:4px 0 0;color:var(--suave)}"
    ".resumo{display:flex;flex-wrap:wrap;gap:12px;margin:0 0 20px;padding:0;list-style:none}"
    ".resumo li{background:var(--cartao);border:1px solid var(--borda);border-radius:10px;"
    "padding:10px 16px}.resumo strong{display:block;font-size:1.3rem}"
    "table{width:100%;border-collapse:collapse;background:var(--cartao);"
    "border:1px solid var(--borda);border-radius:10px;overflow:hidden}"
    "caption{text-align:left;font-weight:600;padding:0 0 8px}"
    "th,td{padding:10px 12px;text-align:left;vertical-align:top;border-bottom:1px solid var(--borda)}"
    "th{background:var(--zebra);font-size:.85rem;text-transform:uppercase;letter-spacing:.03em}"
    "tbody tr:nth-child(even){background:var(--zebra)}"
    "td.num{color:var(--suave);font-variant-numeric:tabular-nums}"
    "footer{margin-top:20px;color:var(--suave);font-size:.85rem}"
    ".vazio{padding:24px;text-align:center;color:var(--suave)}"
    "@media (max-width:700px){thead{position:absolute;left:-9999px}"
    "table,tbody,tr,td{display:block;width:100%}tr{border-bottom:2px solid var(--borda);padding:8px 0}"
    "td{border:0;padding:4px 12px}td::before{content:attr(data-rotulo);display:block;"
    "font-size:.75rem;text-transform:uppercase;color:var(--suave)}}"
    "@media print{body{background:#fff;color:#000}table,.resumo li{border-color:#999}"
    "tbody tr:nth-child(even){background:#f2f2f2}}";

/** Escreve uma célula com rótulo (usado no layout de cards no celular). */
static void celula(FILE *arquivo, const char *rotulo, const char *valor, const char *classe)
{
    fprintf(arquivo, "<td data-rotulo=\"%s\"%s%s%s>", rotulo,
            classe ? " class=\"" : "", classe ? classe : "", classe ? "\"" : "");
    escrever_escapado(arquivo, valor);
    fputs("</td>", arquivo);
}

static void escrever_linha_html(FILE *arquivo, const Cliente *c, size_t posicao)
{
    char numero[24];
    char cpf[TAM_CPF_FORMATADO];
    char cep[TAM_CEP_FORMATADO];
    char endereco[TAM_RUA + TAM_NUMERO + TAM_BAIRRO + 8];
    char cidade_uf[TAM_CIDADE + TAM_ESTADO + 2];

    snprintf(numero, sizeof numero, "%u", (unsigned)posicao);
    exportar_mascarar_cpf(c->cpf, cpf, sizeof cpf);
    formatar_cep(c->cep, cep, sizeof cep);
    snprintf(endereco, sizeof endereco, "%s, %s - %s", c->rua, c->numero, c->bairro);
    snprintf(cidade_uf, sizeof cidade_uf, "%s/%s", c->cidade, c->estado);

    fputs("<tr>", arquivo);
    celula(arquivo, "Nº", numero, "num");
    celula(arquivo, "Nome", c->nome, NULL);
    celula(arquivo, "CPF", cpf, "num");
    celula(arquivo, "E-mail", c->email, NULL);
    celula(arquivo, "Endereço", endereco, NULL);
    celula(arquivo, "Cidade/UF", cidade_uf, NULL);
    celula(arquivo, "CEP", cep, "num");
    fputs("</tr>\n", arquivo);
}

ResultadoArquivo exportar_html(const char *caminho, const ListaClientes *lista,
                               const Cliente *const clientes[], size_t quantidade,
                               const char *descricao)
{
    char gerado_em[32] = "";
    time_t agora = time(NULL);
    const struct tm *local = localtime(&agora);
    FILE *arquivo;
    size_t i;
    int sucesso;

    if (local != NULL) {
        strftime(gerado_em, sizeof gerado_em, "%d/%m/%Y %H:%M", local);
    }

    arquivo = fopen(caminho, "w");
    if (arquivo == NULL) {
        return ARQUIVO_ERRO_ESCRITA;
    }

    fputs("<!DOCTYPE html>\n<html lang=\"pt-BR\">\n<head>\n<meta charset=\"utf-8\">\n"
          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
          "<meta name=\"color-scheme\" content=\"light dark\">\n"
          "<title>DogWalker - Relatório de clientes</title>\n<style>", arquivo);
    fputs(ESTILO, arquivo);
    fputs("</style>\n</head>\n<body>\n<main>\n<header>\n"
          "<h1>DogWalker</h1>\n<p>Relatório de clientes</p>\n</header>\n", arquivo);

    fprintf(arquivo, "<ul class=\"resumo\">\n<li><strong>%u</strong>no relatório</li>\n"
                     "<li><strong>%u</strong>cadastrados</li>\n",
            (unsigned)quantidade, (unsigned)lista->quantidade);
    if (gerado_em[0] != '\0') {
        fprintf(arquivo, "<li><strong>%s</strong>gerado em</li>\n", gerado_em);
    }
    fputs("</ul>\n", arquivo);

    fputs("<table>\n<caption>", arquivo);
    escrever_escapado(arquivo, (descricao != NULL && descricao[0] != '\0')
                                   ? descricao : "Todos os clientes, em ordem de cadastro");
    fputs("</caption>\n<thead><tr><th scope=\"col\">Nº</th><th scope=\"col\">Nome</th>"
          "<th scope=\"col\">CPF</th><th scope=\"col\">E-mail</th>"
          "<th scope=\"col\">Endereço</th><th scope=\"col\">Cidade/UF</th>"
          "<th scope=\"col\">CEP</th></tr></thead>\n<tbody>\n", arquivo);

    /* Os ponteiros apontam para dentro do vetor da lista; a diferença para
     * o início dá a posição de cadastro, a mesma coluna "#" da tela. */
    for (i = 0; i < quantidade; i++) {
        escrever_linha_html(arquivo, clientes[i], (size_t)(clientes[i] - lista->itens) + 1);
    }
    if (quantidade == 0) {
        fputs("<tr><td class=\"vazio\" colspan=\"7\">Nenhum cliente neste recorte.</td></tr>\n",
              arquivo);
    }

    fputs("</tbody>\n</table>\n<footer>CPFs exibidos de forma mascarada para proteger os "
          "dados pessoais. Gerado pelo DogWalker.</footer>\n</main>\n</body>\n</html>\n",
          arquivo);

    sucesso = !ferror(arquivo);
    if (fclose(arquivo) != 0) {
        sucesso = 0;
    }
    return sucesso ? ARQUIVO_OK : ARQUIVO_ERRO_ESCRITA;
}

/* ------------------------------------------------------------------ */
/* Planilha CSV                                                        */
/* ------------------------------------------------------------------ */

ResultadoArquivo exportar_csv(const char *caminho, const Cliente *const clientes[],
                              size_t quantidade)
{
    /* Modo binário + "\r\n" explícito: o arquivo sai idêntico em qualquer
     * sistema, com o fim de linha CRLF que o padrão CSV (RFC 4180) e o
     * Excel esperam. */
    FILE *arquivo = fopen(caminho, "wb");
    size_t i;
    int sucesso;

    if (arquivo == NULL) {
        return ARQUIVO_ERRO_ESCRITA;
    }

    /* BOM: faz o Excel reconhecer o UTF-8 e mostrar os acentos certos. */
    fputs(BOM_UTF8 "Nome;CPF;E-mail;Rua;Número;Bairro;Cidade;UF;CEP\r\n", arquivo);

    for (i = 0; i < quantidade; i++) {
        const Cliente *c = clientes[i];
        /* CPF e CEP vão formatados: assim a planilha os trata como texto e
         * não descarta zeros à esquerda. */
        char cpf[TAM_CPF_FORMATADO];
        char cep[TAM_CEP_FORMATADO];
        const char *campos[9];
        size_t j;

        formatar_cpf(c->cpf, cpf, sizeof cpf);
        formatar_cep(c->cep, cep, sizeof cep);
        campos[0] = c->nome;
        campos[1] = cpf;
        campos[2] = c->email;
        campos[3] = c->rua;
        campos[4] = c->numero;
        campos[5] = c->bairro;
        campos[6] = c->cidade;
        campos[7] = c->estado;
        campos[8] = cep;

        for (j = 0; j < 9; j++) {
            char seguro[TAM_NOME + 2];
            exportar_valor_planilha(campos[j], seguro, sizeof seguro);
            fprintf(arquivo, "%s%s", seguro, j < 8 ? ";" : "\r\n");
        }
    }

    sucesso = !ferror(arquivo);
    if (fclose(arquivo) != 0) {
        sucesso = 0;
    }
    return sucesso ? ARQUIVO_OK : ARQUIVO_ERRO_ESCRITA;
}
