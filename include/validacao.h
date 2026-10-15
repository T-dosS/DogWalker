/**
 * @file validacao.h
 * @brief Validação e normalização dos dados digitados pelo usuário.
 *
 * Todas as funções "normalizar_*" seguem o mesmo contrato:
 *
 *   int normalizar_xxx(const char *entrada, char *saida, size_t tamanho);
 *
 * - Recebem o texto bruto (como o usuário digitou ou como veio do arquivo).
 * - Se o valor for válido, gravam em `saida` a versão padronizada
 *   (sem espaços nas pontas, sem máscara, em maiúsculas/minúsculas etc.)
 *   e retornam 1.
 * - Se for inválido, retornam 0 e NÃO alteram `saida`. Isso é importante
 *   na alteração de cadastro: um valor inválido nunca apaga o valor atual.
 *
 * Como todas têm a mesma assinatura, podem ser passadas como ponteiro de
 * função (tipo Normalizador), o que evita repetir o laço de leitura para
 * cada campo do cadastro.
 */
#ifndef VALIDACAO_H
#define VALIDACAO_H

#include <stddef.h>

/** Ponteiro para qualquer função normalizar_*. */
typedef int (*Normalizador)(const char *entrada, char *saida, size_t tamanho);

/**
 * Texto livre obrigatório (nome, rua, número, bairro, cidade).
 * Regras: não pode ficar vazio, precisa caber em `tamanho`, não pode ter
 * ';' (separador do arquivo CSV) nem caracteres de controle.
 * Acentos (UTF-8) são permitidos.
 */
int normalizar_texto(const char *entrada, char *saida, size_t tamanho);

/**
 * E-mail. Validação simples e intencionalmente didática:
 * exatamente um '@', algo antes dele, domínio com '.', sem espaços,
 * sem ".." e sem ';'. O resultado é gravado em minúsculas.
 */
int normalizar_email(const char *entrada, char *saida, size_t tamanho);

/**
 * CPF com ou sem máscara ("529.982.247-25" ou "52998224725").
 * Confere os dois dígitos verificadores e rejeita sequências repetidas
 * como "111.111.111-11". Grava somente os 11 dígitos.
 */
int normalizar_cpf(const char *entrada, char *saida, size_t tamanho);

/** CEP com ou sem hífen ("01001-000" ou "01001000"). Grava 8 dígitos. */
int normalizar_cep(const char *entrada, char *saida, size_t tamanho);

/** Sigla de UF brasileira ("sp", "SP"). Grava em maiúsculas. */
int normalizar_uf(const char *entrada, char *saida, size_t tamanho);

/**
 * Verifica os dígitos verificadores de um CPF.
 *
 * @param cpf string com exatamente 11 dígitos, sem máscara.
 * @return 1 se o CPF for válido, 0 caso contrário.
 */
int cpf_digitos_validos(const char *cpf);

/**
 * Converte um texto inteiro (ex.: "3") para int de forma segura.
 * Diferente de atoi/scanf, rejeita "3abc", "", "abc" e números fora do
 * intervalo de int.
 *
 * @return 1 em caso de sucesso (valor gravado em *valor), 0 caso contrário.
 */
int converter_inteiro(const char *texto, int *valor);

/** Remove espaços (e '\r', '\t', '\n') do início e do fim do texto. */
void aparar_texto(char *texto);

/** Formata 11 dígitos como 000.000.000-00. */
void formatar_cpf(const char *cpf, char *saida, size_t tamanho);

/** Formata 8 dígitos como 00000-000. */
void formatar_cep(const char *cep, char *saida, size_t tamanho);

#endif /* VALIDACAO_H */
