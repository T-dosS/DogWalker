/**
 * @file cliente.c
 * @brief Operações sobre a lista dinâmica de clientes.
 */
#include "cliente.h"
#include "validacao.h"

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

void cliente_exibir(const Cliente *cliente)
{
    char cpf[TAM_CPF_FORMATADO];
    char cep[TAM_CEP_FORMATADO];

    formatar_cpf(cliente->cpf, cpf, sizeof cpf);
    formatar_cep(cliente->cep, cep, sizeof cep);

    printf("  Nome ......: %s\n", cliente->nome);
    printf("  CPF .......: %s\n", cpf);
    printf("  E-mail ....: %s\n", cliente->email);
    printf("  Endereço ..: %s, %s - %s\n", cliente->rua, cliente->numero,
           cliente->bairro);
    printf("  Cidade/UF .: %s/%s\n", cliente->cidade, cliente->estado);
    printf("  CEP .......: %s\n", cep);
}
