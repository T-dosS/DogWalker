/**
 * @file login.h
 * @brief Autenticação do operador antes de acessar o menu.
 */
#ifndef LOGIN_H
#define LOGIN_H

#include <stddef.h>

/** Quantidade de tentativas antes de o programa encerrar. */
#define LOGIN_MAX_TENTATIVAS 3

/** Tamanho de buffer suficiente para o nome de usuário. */
#define LOGIN_TAM_USUARIO 65

/**
 * Pede usuário e senha até LOGIN_MAX_TENTATIVAS vezes.
 *
 * As credenciais padrão são "admin"/"admin" (mantidas do trabalho
 * original). Elas podem ser trocadas sem recompilar, definindo as
 * variáveis de ambiente DOGWALKER_USUARIO e DOGWALKER_SENHA.
 *
 * @param usuario_logado recebe o nome do usuário autenticado.
 * @param tamanho        tamanho do buffer usuario_logado.
 * @return 1 se o login foi efetuado e 0 caso contrário.
 */
int login_autenticar(char *usuario_logado, size_t tamanho);

#endif /* LOGIN_H */
