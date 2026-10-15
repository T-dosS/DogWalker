/**
 * @file validacao.c
 * @brief Regras de validação e padronização dos dados do cliente.
 *
 * Observação sobre <ctype.h>: funções como isdigit/isspace recebem int e
 * têm comportamento indefinido para valores negativos. Como `char` pode
 * ser negativo (ex.: bytes de letras acentuadas em UTF-8), sempre
 * convertemos para `unsigned char` antes de chamá-las.
 */
#include "validacao.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIGITOS_CPF 11
#define DIGITOS_CEP 8

/* As 27 unidades federativas do Brasil. */
static const char *const UFS_VALIDAS[] = {
    "AC", "AL", "AP", "AM", "BA", "CE", "DF", "ES", "GO",
    "MA", "MT", "MS", "MG", "PA", "PB", "PR", "PE", "PI",
    "RJ", "RN", "RS", "RO", "RR", "SC", "SP", "SE", "TO"
};

/* ------------------------------------------------------------------ */
/* Funções auxiliares internas                                         */
/* ------------------------------------------------------------------ */

/**
 * Calcula onde começa e quanto mede o texto sem os espaços das pontas,
 * sem modificar a string original (ela é const).
 */
static void limites_sem_espacos(const char *texto, size_t *inicio, size_t *comprimento)
{
    size_t ini = 0;
    size_t fim = strlen(texto);

    while (texto[ini] != '\0' && isspace((unsigned char)texto[ini])) {
        ini++;
    }
    while (fim > ini && isspace((unsigned char)texto[fim - 1])) {
        fim--;
    }
    *inicio = ini;
    *comprimento = fim - ini;
}

/**
 * Copia `comprimento` bytes e termina com '\0', se couber no destino.
 * @return 1 se copiou, 0 se não coube.
 */
static int copiar_se_couber(const char *origem, size_t comprimento,
                            char *destino, size_t tamanho)
{
    if (comprimento >= tamanho) {
        return 0;
    }
    memcpy(destino, origem, comprimento);
    destino[comprimento] = '\0';
    return 1;
}

/**
 * Caracteres que não podem aparecer em nenhum campo: ';' quebraria o
 * formato CSV, e caracteres de controle (quebra de linha, tab, DEL...)
 * quebrariam a estrutura "um cliente por linha".
 */
static int caractere_proibido(unsigned char c)
{
    return c < 32 || c == 127 || c == ';';
}

/**
 * Extrai exatamente `quantidade` dígitos de `entrada`, aceitando os
 * separadores ' ', '.', '-' e '/' (máscaras comuns de CPF e CEP).
 * Qualquer outro caractere torna a entrada inválida.
 *
 * @param digitos buffer com pelo menos quantidade + 1 posições.
 * @return 1 se encontrou exatamente `quantidade` dígitos, 0 caso contrário.
 */
static int extrair_digitos(const char *entrada, char *digitos, size_t quantidade)
{
    size_t encontrados = 0;
    const char *p;

    for (p = entrada; *p != '\0'; p++) {
        unsigned char c = (unsigned char)*p;
        if (isdigit(c)) {
            if (encontrados == quantidade) {
                return 0; /* dígitos demais */
            }
            digitos[encontrados++] = (char)c;
        } else if (c != ' ' && c != '.' && c != '-' && c != '/') {
            return 0;
        }
    }
    digitos[encontrados] = '\0';
    return encontrados == quantidade;
}

/* ------------------------------------------------------------------ */
/* Funções públicas                                                    */
/* ------------------------------------------------------------------ */

void aparar_texto(char *texto)
{
    size_t inicio;
    size_t comprimento;

    limites_sem_espacos(texto, &inicio, &comprimento);
    memmove(texto, texto + inicio, comprimento);
    texto[comprimento] = '\0';
}

int normalizar_texto(const char *entrada, char *saida, size_t tamanho)
{
    size_t inicio;
    size_t comprimento;
    size_t i;

    limites_sem_espacos(entrada, &inicio, &comprimento);
    if (comprimento == 0) {
        return 0;
    }
    for (i = inicio; i < inicio + comprimento; i++) {
        if (caractere_proibido((unsigned char)entrada[i])) {
            return 0;
        }
    }
    return copiar_se_couber(entrada + inicio, comprimento, saida, tamanho);
}

int normalizar_email(const char *entrada, char *saida, size_t tamanho)
{
    char email[256];
    size_t inicio;
    size_t comprimento;
    size_t i;
    const char *arroba;
    const char *dominio;
    const char *ultimo_ponto;

    limites_sem_espacos(entrada, &inicio, &comprimento);
    if (comprimento == 0 || comprimento >= tamanho || comprimento >= sizeof email) {
        return 0;
    }

    /* Trabalha em uma cópia local para só escrever em `saida` se tudo
     * estiver correto (contrato das funções normalizar_*). */
    for (i = 0; i < comprimento; i++) {
        unsigned char c = (unsigned char)entrada[inicio + i];
        if (c == ' ' || caractere_proibido(c)) {
            return 0;
        }
        email[i] = (char)tolower(c);
    }
    email[comprimento] = '\0';

    arroba = strchr(email, '@');
    if (arroba == NULL || arroba == email || strchr(arroba + 1, '@') != NULL) {
        return 0; /* precisa de exatamente um '@', com algo antes dele */
    }

    dominio = arroba + 1;
    ultimo_ponto = strrchr(dominio, '.');
    if (ultimo_ponto == NULL           /* domínio sem ponto: "ana@gmail"  */
        || dominio[0] == '.'           /* "ana@.com"                      */
        || ultimo_ponto[1] == '\0'     /* "ana@gmail."                    */
        || strstr(email, "..") != NULL) {
        return 0;
    }

    memcpy(saida, email, comprimento + 1);
    return 1;
}

int cpf_digitos_validos(const char *cpf)
{
    int soma;
    int resto;
    int i;
    int todos_iguais = 1;

    for (i = 0; i < DIGITOS_CPF; i++) {
        if (!isdigit((unsigned char)cpf[i])) {
            return 0;
        }
        if (cpf[i] != cpf[0]) {
            todos_iguais = 0;
        }
    }
    if (cpf[DIGITOS_CPF] != '\0' || todos_iguais) {
        return 0; /* "111.111.111-11" passa na conta, mas não é um CPF real */
    }

    /* Primeiro dígito verificador: pesos 10, 9, ..., 2 sobre os 9 primeiros. */
    soma = 0;
    for (i = 0; i < 9; i++) {
        soma += (cpf[i] - '0') * (10 - i);
    }
    resto = (soma * 10) % 11;
    if (resto == 10) {
        resto = 0;
    }
    if (resto != cpf[9] - '0') {
        return 0;
    }

    /* Segundo dígito verificador: pesos 11, 10, ..., 2 sobre os 10 primeiros. */
    soma = 0;
    for (i = 0; i < 10; i++) {
        soma += (cpf[i] - '0') * (11 - i);
    }
    resto = (soma * 10) % 11;
    if (resto == 10) {
        resto = 0;
    }
    return resto == cpf[10] - '0';
}

int normalizar_cpf(const char *entrada, char *saida, size_t tamanho)
{
    char digitos[DIGITOS_CPF + 1];

    if (tamanho < sizeof digitos
        || !extrair_digitos(entrada, digitos, DIGITOS_CPF)
        || !cpf_digitos_validos(digitos)) {
        return 0;
    }
    memcpy(saida, digitos, sizeof digitos);
    return 1;
}

int normalizar_cep(const char *entrada, char *saida, size_t tamanho)
{
    char digitos[DIGITOS_CEP + 1];

    if (tamanho < sizeof digitos || !extrair_digitos(entrada, digitos, DIGITOS_CEP)) {
        return 0;
    }
    memcpy(saida, digitos, sizeof digitos);
    return 1;
}

int normalizar_uf(const char *entrada, char *saida, size_t tamanho)
{
    char sigla[3];
    size_t inicio;
    size_t comprimento;
    size_t i;

    limites_sem_espacos(entrada, &inicio, &comprimento);
    if (comprimento != 2 || tamanho < sizeof sigla) {
        return 0;
    }
    sigla[0] = (char)toupper((unsigned char)entrada[inicio]);
    sigla[1] = (char)toupper((unsigned char)entrada[inicio + 1]);
    sigla[2] = '\0';

    for (i = 0; i < sizeof UFS_VALIDAS / sizeof UFS_VALIDAS[0]; i++) {
        if (strcmp(sigla, UFS_VALIDAS[i]) == 0) {
            memcpy(saida, sigla, sizeof sigla);
            return 1;
        }
    }
    return 0;
}

int converter_inteiro(const char *texto, int *valor)
{
    char *fim;
    long numero;

    errno = 0;
    numero = strtol(texto, &fim, 10);

    /* fim == texto: nada foi convertido ("abc" ou "").
     * *fim != '\0': sobrou texto depois do número ("3abc"). */
    if (fim == texto || *fim != '\0' || errno == ERANGE
        || numero < INT_MIN || numero > INT_MAX) {
        return 0;
    }
    *valor = (int)numero;
    return 1;
}

void formatar_cpf(const char *cpf, char *saida, size_t tamanho)
{
    snprintf(saida, tamanho, "%.3s.%.3s.%.3s-%.2s", cpf, cpf + 3, cpf + 6, cpf + 9);
}

void formatar_cep(const char *cep, char *saida, size_t tamanho)
{
    snprintf(saida, tamanho, "%.5s-%.3s", cep, cep + 5);
}
