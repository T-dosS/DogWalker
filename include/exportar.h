/**
 * @file exportar.h
 * @brief Exportação dos clientes para relatório HTML e planilha CSV.
 *
 * As exportações recebem a lista JÁ filtrada e ordenada (o que o usuário
 * está vendo na tela) e geram arquivos separados: o arquivo de dados
 * original nunca é alterado.
 *
 * Segurança:
 * - HTML: todo texto é escapado (&, <, >, ", '), então um nome como
 *   "<script>" aparece como texto e não vira código na página.
 * - Planilha: valores que começam com =, +, -, @, tabulação ou CR recebem
 *   um apóstrofo na frente, para o Excel/LibreOffice não os interpretar
 *   como fórmula (injeção de fórmulas em CSV).
 * - Privacidade: o relatório HTML, feito para ser impresso ou compartilhado,
 *   mostra o CPF mascarado (***.982.247-**); a planilha mantém o CPF completo.
 */
#ifndef EXPORTAR_H
#define EXPORTAR_H

#include "arquivo.h"
#include "cliente.h"

#include <stddef.h>

/** Nomes dos arquivos gerados, criados na mesma pasta do arquivo de dados. */
#define EXPORTAR_NOME_HTML "relatorio.html"
#define EXPORTAR_NOME_CSV  "exportacao.csv"

/**
 * Escreve em `saida` o texto com os caracteres especiais do HTML trocados
 * por entidades (&amp; &lt; &gt; &quot; &#39;).
 *
 * @return 1 se coube em `tamanho`, 0 se não coube (saída truncada, mas
 *         sempre terminada em '\0' e sem cortar uma entidade ao meio).
 */
int exportar_escapar_html(const char *texto, char *saida, size_t tamanho);

/**
 * Prepara um valor para ser aberto com segurança em planilhas: se ele
 * começar com =, +, -, @, tabulação ou CR, recebe um apóstrofo na frente.
 *
 * @return 1 se coube em `tamanho`, 0 caso contrário.
 */
int exportar_valor_planilha(const char *valor, char *saida, size_t tamanho);

/** Formata o CPF mascarado para relatórios: "52998224725" -> "***.982.247-**". */
void exportar_mascarar_cpf(const char *cpf, char *saida, size_t tamanho);

/**
 * Monta o caminho de um arquivo na mesma pasta de `caminho_dados`.
 * Ex.: ("data/clientes.csv", "relatorio.html") -> "data/relatorio.html".
 *
 * @return 1 se coube em `tamanho`, 0 caso contrário.
 */
int exportar_caminho_vizinho(const char *caminho_dados, const char *nome,
                             char *saida, size_t tamanho);

/**
 * Gera o relatório HTML5/CSS3 (sem JavaScript e sem recursos externos):
 * responsivo, com tema claro/escuro automático e pronto para impressão.
 *
 * @param lista      lista completa; dá o total e a posição "Nº" de cada
 *                   cliente (a mesma coluna "#" da tela).
 * @param clientes   ponteiros para clientes DE `lista`, na ordem desejada.
 * @param quantidade quantos ponteiros há em `clientes`.
 * @param descricao  linha explicando o recorte (ex.: filtro e ordem) ou NULL.
 */
ResultadoArquivo exportar_html(const char *caminho, const ListaClientes *lista,
                               const Cliente *const clientes[], size_t quantidade,
                               const char *descricao);

/** Gera a planilha CSV (separador ';', UTF-8 com BOM para o Excel). */
ResultadoArquivo exportar_csv(const char *caminho, const Cliente *const clientes[],
                              size_t quantidade);

#endif /* EXPORTAR_H */
