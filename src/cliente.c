/**
 * @file cliente.c
 * @brief Operações sobre a lista dinâmica de clientes.
 */
#include "cliente.h"
#include "ui.h"
#include "validacao.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Capacidade do primeiro bloco alocado. A cada vez que o vetor enche, a
 * capacidade dobra: isso mantém o número de realloc() pequeno mesmo com
 * muitos clientes (crescimento geométrico). */
#define CAPACIDADE_INICIAL 8

void lista_iniciar(ListaClientes *lista)
{
    lista->itens = NULL;
    lista->quantidade = 0;
    lista->capacidade = 0;
}

void lista_liberar(ListaClientes *lista)
{
    free(lista->itens);
    lista_iniciar(lista);
}

int lista_adicionar(ListaClientes *lista, const Cliente *cliente)
{
    if (lista->quantidade == lista->capacidade) {
        size_t nova_capacidade = (lista->capacidade == 0)
                                     ? CAPACIDADE_INICIAL
                                     : lista->capacidade * 2;

        /* O resultado vai para um ponteiro auxiliar: se realloc falhar ele
         * devolve NULL, mas o bloco antigo continua válido. Atribuir direto
         * em lista->itens perderia o endereço antigo (vazamento de memória). */
        Cliente *novos_itens = realloc(lista->itens,
                                       nova_capacidade * sizeof *novos_itens);
        if (novos_itens == NULL) {
            return 0;
        }
        lista->itens = novos_itens;
        lista->capacidade = nova_capacidade;
    }

    lista->itens[lista->quantidade] = *cliente; /* cópia da struct inteira */
    lista->quantidade++;
    return 1;
}

int lista_buscar_cpf(const ListaClientes *lista, const char *cpf)
{
    /* Busca linear: suficiente para o volume de um sistema acadêmico.
     * Como o CPF é sempre guardado só com dígitos, basta strcmp. */
    size_t i;
    for (i = 0; i < lista->quantidade; i++) {
        if (strcmp(lista->itens[i].cpf, cpf) == 0) {
            return (int)i;
        }
    }
    return -1;
}

int lista_remover(ListaClientes *lista, size_t indice)
{
    size_t posteriores;

    if (indice >= lista->quantidade) {
        return 0;
    }

    /* Desloca os clientes seguintes uma posição para trás, sobrescrevendo
     * o removido. memmove (e não memcpy) é obrigatório porque as regiões
     * de origem e destino se sobrepõem. */
    posteriores = lista->quantidade - indice - 1;
    memmove(&lista->itens[indice], &lista->itens[indice + 1],
            posteriores * sizeof lista->itens[0]);
    lista->quantidade--;
    return 1;
}

int clientes_iguais(const Cliente *a, const Cliente *b)
{
    /* Compara campo a campo com strcmp em vez de memcmp na struct inteira:
     * bytes "lixo" depois do '\0' de cada campo não devem influenciar. */
    return strcmp(a->cpf, b->cpf) == 0
        && strcmp(a->nome, b->nome) == 0
        && strcmp(a->email, b->email) == 0
        && strcmp(a->rua, b->rua) == 0
        && strcmp(a->numero, b->numero) == 0
        && strcmp(a->bairro, b->bairro) == 0
        && strcmp(a->cidade, b->cidade) == 0
        && strcmp(a->estado, b->estado) == 0
        && strcmp(a->cep, b->cep) == 0;
}

/* Cabe qualquer campo de texto do cliente (o maior tem 100 bytes). Um termo
 * de busca maior que isso não pode estar contido em nenhum campo. */
#define TAM_COMPARACAO 128

/**
 * Procura `termo_comparavel` (já em minúsculas e sem acentos) dentro de
 * `texto`, convertido da mesma forma: "sao" encontra "São Paulo" e
 * "SÃO" encontra "sao".
 */
static int contem_para_busca(const char *texto, const char *termo_comparavel)
{
    char comparavel[TAM_COMPARACAO];

    texto_para_comparacao(texto, comparavel, sizeof comparavel);
    return strstr(comparavel, termo_comparavel) != NULL;
}

/**
 * Se o termo for um pedaço de CPF (só dígitos e pontuação), procura os
 * dígitos no CPF do cliente, que é guardado sem máscara.
 */
static int corresponde_cpf(const char *cpf, const char *termo)
{
    char digitos[TAM_CPF];
    size_t quantidade = 0;
    const char *p;

    for (p = termo; *p != '\0'; p++) {
        if (isdigit((unsigned char)*p)) {
            if (quantidade == sizeof digitos - 1) {
                return 0; /* mais de 11 dígitos: não é CPF */
            }
            digitos[quantidade++] = *p;
        } else if (*p != '.' && *p != '-' && *p != ' ') {
            return 0; /* tem letras: o termo não é um CPF */
        }
    }
    digitos[quantidade] = '\0';
    return quantidade > 0 && strstr(cpf, digitos) != NULL;
}

int cliente_corresponde(const Cliente *cliente, const char *termo)
{
    if (termo == NULL || termo[0] == '\0') {
        return 1;
    }
    if (strlen(termo) < TAM_COMPARACAO) {
        char termo_comparavel[TAM_COMPARACAO];

        texto_para_comparacao(termo, termo_comparavel, sizeof termo_comparavel);
        if (contem_para_busca(cliente->nome, termo_comparavel)
            || contem_para_busca(cliente->email, termo_comparavel)
            || contem_para_busca(cliente->cidade, termo_comparavel)
            || contem_para_busca(cliente->estado, termo_comparavel)) {
            return 1;
        }
    }
    return corresponde_cpf(cliente->cpf, termo);
}

/* qsort() não recebe um parâmetro extra para o comparador, então a ordem
 * pedida fica nesta variável durante a ordenação (o programa não usa threads). */
static OrdemListagem ordem_atual = ORDEM_CADASTRO;

/** Compara dois textos ignorando maiúsculas e acentos. */
static int comparar_textos(const char *a, const char *b)
{
    char comparavel_a[TAM_COMPARACAO];
    char comparavel_b[TAM_COMPARACAO];

    texto_para_comparacao(a, comparavel_a, sizeof comparavel_a);
    texto_para_comparacao(b, comparavel_b, sizeof comparavel_b);
    return strcmp(comparavel_a, comparavel_b);
}

static int comparar_clientes(const void *pa, const void *pb)
{
    const Cliente *a = *(const Cliente *const *)pa;
    const Cliente *b = *(const Cliente *const *)pb;
    int resultado = 0;

    if (ordem_atual == ORDEM_CIDADE) {
        resultado = comparar_textos(a->cidade, b->cidade);
        if (resultado == 0) {
            resultado = strcmp(a->estado, b->estado);
        }
    }
    if (resultado == 0 && ordem_atual != ORDEM_CADASTRO) {
        resultado = comparar_textos(a->nome, b->nome);
    }
    if (resultado == 0) {
        /* Desempate pela posição no vetor: qsort não é estável, e assim
         * clientes com o mesmo nome continuam na ordem de cadastro. */
        resultado = (a > b) - (a < b);
    }
    return resultado;
}

const Cliente **lista_criar_visao(const ListaClientes *lista, OrdemListagem ordem)
{
    const Cliente **visao;
    size_t i;

    if (lista->quantidade == 0) {
        return NULL;
    }
    visao = malloc(lista->quantidade * sizeof *visao);
    if (visao == NULL) {
        return NULL;
    }
    for (i = 0; i < lista->quantidade; i++) {
        visao[i] = &lista->itens[i];
    }
    if (ordem != ORDEM_CADASTRO) {
        ordem_atual = ordem;
        qsort(visao, lista->quantidade, sizeof *visao, comparar_clientes);
        ordem_atual = ORDEM_CADASTRO;
    }
    return visao;
}

void cliente_exibir(const Cliente *cliente)
{
    const char *rotulo = ui_codigo(UI_COR_ROTULO);
    const char *normal = ui_codigo(UI_COR_NORMAL);
    char cpf[TAM_CPF_FORMATADO];
    char cep[TAM_CEP_FORMATADO];

    formatar_cpf(cliente->cpf, cpf, sizeof cpf);
    formatar_cep(cliente->cep, cep, sizeof cep);

    printf("  %sNome ......:%s %s\n", rotulo, normal, cliente->nome);
    printf("  %sCPF .......:%s %s\n", rotulo, normal, cpf);
    printf("  %sE-mail ....:%s %s\n", rotulo, normal, cliente->email);
    printf("  %sEndereço ..:%s %s, %s - %s\n", rotulo, normal, cliente->rua,
           cliente->numero, cliente->bairro);
    printf("  %sCidade/UF .:%s %s/%s\n", rotulo, normal, cliente->cidade, cliente->estado);
    printf("  %sCEP .......:%s %s\n", rotulo, normal, cep);
}
