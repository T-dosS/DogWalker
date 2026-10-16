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

/**
 * Procura `termo` dentro de `texto` sem diferenciar maiúsculas/minúsculas.
 * Compara byte a byte com tolower(), o que cobre letras sem acento
 * ("ana" encontra "Ana"); letras acentuadas precisam coincidir exatamente.
 */
static int contem_ignorando_caixa(const char *texto, const char *termo)
{
    size_t tamanho_termo = strlen(termo);
    size_t i;
    size_t j;

    for (i = 0; texto[i] != '\0'; i++) {
        for (j = 0; j < tamanho_termo && texto[i + j] != '\0'; j++) {
            if (tolower((unsigned char)texto[i + j]) != tolower((unsigned char)termo[j])) {
                break;
            }
        }
        if (j == tamanho_termo) {
            return 1;
        }
    }
    return 0;
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
    return contem_ignorando_caixa(cliente->nome, termo)
        || contem_ignorando_caixa(cliente->email, termo)
        || contem_ignorando_caixa(cliente->cidade, termo)
        || contem_ignorando_caixa(cliente->estado, termo)
        || corresponde_cpf(cliente->cpf, termo);
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
