/**
 * @file arquivo.h
 * @brief Persistência dos clientes em arquivo CSV.
 *
 * Formato (uma linha por cliente, separador ';', codificação UTF-8):
 *
 *   cpf;nome;email;rua;numero;bairro;cidade;estado;cep
 *   52998224725;Ana Souza;ana@example.com;Rua das Flores;120;Centro;Campinas;SP;13010000
 *
 * Estratégia adotada: o programa carrega o arquivo inteiro para uma
 * ListaClientes, faz a operação em memória e regrava o arquivo completo.
 * A regravação é feita primeiro em um arquivo temporário (".tmp"), que só
 * substitui o original se tudo tiver sido escrito sem erro. Assim uma
 * falha no meio da gravação não corrompe os dados existentes.
 */
#ifndef ARQUIVO_H
#define ARQUIVO_H

#include "cliente.h"

/** Caminho usado quando nenhum arquivo é informado na linha de comando. */
#define ARQUIVO_PADRAO "data/clientes.csv"

/** Primeira linha do CSV, com o nome das colunas. */
#define CABECALHO_CSV "cpf;nome;email;rua;numero;bairro;cidade;estado;cep"

/** Resultado das operações de arquivo. */
typedef enum {
    ARQUIVO_OK = 0,
    ARQUIVO_ERRO_LEITURA,
    ARQUIVO_ERRO_ESCRITA,
    ARQUIVO_ERRO_MEMORIA
} ResultadoArquivo;

/**
 * Lê todos os clientes do arquivo e os adiciona à lista.
 *
 * - Arquivo inexistente NÃO é erro: significa "nenhum cliente ainda".
 * - Linhas inválidas (campos faltando, CPF inválido, CPF repetido,
 *   linha longa demais) são ignoradas e contadas em *linhas_ignoradas,
 *   para que um arquivo editado à mão não derrube o programa.
 *
 * @param caminho          caminho do arquivo CSV.
 * @param lista            lista já iniciada com lista_iniciar().
 * @param linhas_ignoradas recebe a quantidade de linhas descartadas
 *                         (pode ser NULL se não interessar).
 * @return ARQUIVO_OK em caso de sucesso; em caso de erro a lista é liberada.
 */
ResultadoArquivo arquivo_carregar(const char *caminho, ListaClientes *lista,
                                  size_t *linhas_ignoradas);

/**
 * Grava a lista inteira no arquivo, substituindo o conteúdo anterior.
 *
 * @return ARQUIVO_OK em caso de sucesso, ARQUIVO_ERRO_ESCRITA caso
 *         contrário (o arquivo original permanece intacto).
 */
ResultadoArquivo arquivo_salvar(const char *caminho, const ListaClientes *lista);

/** @return uma mensagem em português descrevendo o resultado. */
const char *arquivo_descrever_erro(ResultadoArquivo resultado);

#endif /* ARQUIVO_H */
