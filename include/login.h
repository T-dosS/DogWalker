/**
 * @file login.h
 * @brief Autenticação do operador antes de acessar o menu.
 */
#ifndef LOGIN_H
#define LOGIN_H

/** Quantidade de tentativas antes de o programa encerrar. */
#define LOGIN_MAX_TENTATIVAS 3

/**
 * Pede usuário e senha até LOGIN_MAX_TENTATIVAS vezes.
 *
 * As credenciais padrão são "admin"/"admin" (mantidas do trabalho
 * original). Elas podem ser trocadas sem recompilar, definindo as
 * variáveis de ambiente DOGWALKER_USUARIO e DOGWALKER_SENHA.
 *
 * @return 1 se o login foi efetuado e 0 caso contrário.
 */
int login_autenticar(void);

#endif /* LOGIN_H */
