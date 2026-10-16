/**
 * @file cliente.h
 * @brief Estrutura de dados do cliente e da lista dinâmica de clientes.
 *
 * Este módulo não faz entrada/saída de arquivo nem leitura do teclado:
 * ele apenas representa os dados e oferece operações sobre a lista em
 * memória (adicionar, buscar por CPF, remover). Isso permite testá-lo
 * de forma isolada (veja tests/test_dogwalker.c).
 */
#ifndef CLIENTE_H
#define CLIENTE_H

#include <stddef.h>

/*
 * Tamanhos máximos dos campos, já contando o '\0' final.
 * Ex.: TAM_NOME 101 => o nome pode ter até 100 caracteres (bytes).
 */
#define TAM_CPF      12  /* 11 dígitos, armazenado SEM máscara       */
#define TAM_NOME    101
#define TAM_EMAIL   101
#define TAM_RUA     101
#define TAM_NUMERO   11  /* texto: aceita "123", "12A", "S/N"        */
#define TAM_BAIRRO   61
#define TAM_CIDADE   61
#define TAM_ESTADO    3  /* sigla da UF, ex.: "SP"                    */
#define TAM_CEP       9  /* 8 dígitos, armazenado SEM máscara         */

/** Tamanho de um buffer capaz de guardar o CPF formatado (000.000.000-00). */
#define TAM_CPF_FORMATADO 15
/** Tamanho de um buffer capaz de guardar o CEP formatado (00000-000). */
#define TAM_CEP_FORMATADO 10

/**
 * Representa um cliente da DogWalker.
 *
 * Regra de negócio: o CPF é o identificador único do cliente. Por isso
 * ele é usado na busca, na alteração e na remoção, e não pode ser
 * alterado depois do cadastro.
 *
 * CPF e CEP são guardados apenas com dígitos; a máscara é aplicada só na
 * exibição. Assim "529.982.247-25" e "52998224725" são o mesmo cliente.
 */
typedef struct {
    char cpf[TAM_CPF];
    char nome[TAM_NOME];
    char email[TAM_EMAIL];
    char rua[TAM_RUA];
    char numero[TAM_NUMERO];
    char bairro[TAM_BAIRRO];
    char cidade[TAM_CIDADE];
    char estado[TAM_ESTADO];
    char cep[TAM_CEP];
} Cliente;

/**
 * Lista de clientes em memória, implementada como um vetor dinâmico.
 *
 * - itens:      vetor alocado com malloc/realloc (NULL quando vazio).
 * - quantidade: quantos clientes estão realmente armazenados.
 * - capacidade: quantos clientes cabem no vetor sem realocar.
 *
 * Sempre inicie com lista_iniciar() e libere com lista_liberar().
 */
typedef struct {
    Cliente *itens;
    size_t quantidade;
    size_t capacidade;
} ListaClientes;

/** Deixa a lista vazia e pronta para uso (não aloca memória). */
void lista_iniciar(ListaClientes *lista);

/** Libera a memória da lista e a deixa vazia novamente. */
void lista_liberar(ListaClientes *lista);

/**
 * Adiciona uma cópia do cliente ao final da lista.
 *
 * @return 1 em caso de sucesso e 0 se faltar memória
 *         (nesse caso a lista continua exatamente como estava).
 */
int lista_adicionar(ListaClientes *lista, const Cliente *cliente);

/**
 * Procura um cliente pelo CPF (somente dígitos).
 *
 * @return o índice do cliente na lista, ou -1 se não existir.
 */
int lista_buscar_cpf(const ListaClientes *lista, const char *cpf);

/**
 * Remove o cliente da posição indicada, preservando a ordem dos demais.
 *
 * @return 1 em caso de sucesso e 0 se o índice for inválido.
 */
int lista_remover(ListaClientes *lista, size_t indice);

/** @return 1 se todos os campos dos dois clientes forem iguais, 0 caso contrário. */
int clientes_iguais(const Cliente *a, const Cliente *b);

/**
 * Verifica se o cliente corresponde a um termo de busca (filtro da listagem).
 *
 * O termo é procurado, sem diferenciar maiúsculas de minúsculas, no nome,
 * e-mail, cidade e UF. Se o termo for só dígitos (com ou sem pontuação),
 * também é procurado no CPF, para que "529.982" encontre "52998224725".
 *
 * @return 1 se corresponde (ou se o termo for vazio/NULL), 0 caso contrário.
 */
int cliente_corresponde(const Cliente *cliente, const char *termo);

/** Mostra os dados do cliente no terminal, com CPF e CEP formatados. */
void cliente_exibir(const Cliente *cliente);

#endif /* CLIENTE_H */
