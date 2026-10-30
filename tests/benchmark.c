/**
 * @file benchmark.c
 * @brief Mede o tempo de gravar e carregar o CSV com muitos clientes.
 *
 * Uso: make bench
 *
 * Serve para comparar o desempenho antes e depois de uma otimização. Os
 * arquivos são criados no diretório atual e apagados ao final. Os números
 * variam conforme a máquina; compare sempre na mesma máquina.
 */
#include "arquivo.h"
#include "cliente.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define ARQ_BENCH "bench_tmp_clientes.csv"

/** Relógio de parede em segundos (C11, timespec_get). */
static double agora(void)
{
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

/** Gera um CPF válido e distinto para cada número (mesma regra dos testes). */
static void gerar_cpf(long semente, char cpf[TAM_CPF])
{
    int soma;
    int resto;
    int i;

    snprintf(cpf, TAM_CPF, "%09ld", 100000000L + semente * 7919L);
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

static void medir(long quantidade)
{
    ListaClientes lista;
    ListaClientes lida;
    double inicio;
    double t_salvar;
    double t_carregar;
    size_t ignoradas = 0;
    long i;

    lista_iniciar(&lista);
    lista_iniciar(&lida);
    for (i = 1; i <= quantidade; i++) {
        Cliente c;
        memset(&c, 0, sizeof c);
        gerar_cpf(i, c.cpf);
        snprintf(c.nome, sizeof c.nome, "Cliente %ld", i);
        snprintf(c.email, sizeof c.email, "cliente%ld@example.com", i);
        snprintf(c.rua, sizeof c.rua, "%s", "Rua das Flores");
        snprintf(c.numero, sizeof c.numero, "%ld", i % 9999 + 1);
        snprintf(c.bairro, sizeof c.bairro, "%s", "Centro");
        snprintf(c.cidade, sizeof c.cidade, "%s", "Campinas");
        snprintf(c.estado, sizeof c.estado, "%s", "SP");
        snprintf(c.cep, sizeof c.cep, "%s", "13010000");
        if (!lista_adicionar(&lista, &c)) {
            printf("  sem memória com %ld clientes\n", i);
            lista_liberar(&lista);
            return;
        }
    }

    inicio = agora();
    if (arquivo_salvar(ARQ_BENCH, &lista) != ARQUIVO_OK) {
        printf("  erro ao salvar\n");
    }
    t_salvar = agora() - inicio;

    inicio = agora();
    if (arquivo_carregar(ARQ_BENCH, &lida, &ignoradas) != ARQUIVO_OK) {
        printf("  erro ao carregar\n");
    }
    t_carregar = agora() - inicio;

    printf("  %7ld clientes | salvar %8.3f s | carregar %8.3f s | lidos %zu, ignorados %zu\n",
           quantidade, t_salvar, t_carregar, lida.quantidade, ignoradas);

    lista_liberar(&lista);
    lista_liberar(&lida);
    remove(ARQ_BENCH);
}

int main(void)
{
    static const long TAMANHOS[] = {1000, 10000, 50000};
    size_t i;

    printf("Benchmark do DogWalker (gravação e carga do CSV)\n\n");
    for (i = 0; i < sizeof TAMANHOS / sizeof TAMANHOS[0]; i++) {
        medir(TAMANHOS[i]);
    }
    return 0;
}
