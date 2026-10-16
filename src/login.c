/**
 * @file login.c
 * @brief Tela de login do sistema.
 *
 * Simplificação acadêmica (documentada no README): as credenciais ficam no
 * próprio programa ou em variáveis de ambiente, e a comparação é feita em
 * texto puro. Em um sistema real, a senha seria armazenada como hash
 * (ex.: bcrypt/Argon2) e nunca em texto no código-fonte.
 */
#include "login.h"
#include "terminal.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USUARIO_PADRAO "admin"
#define SENHA_PADRAO   "admin"
#define TAM_CREDENCIAL LOGIN_TAM_USUARIO

/**
 * Lê uma credencial de variável de ambiente, usando o valor padrão se ela
 * não estiver definida. Permite trocar usuário/senha sem recompilar.
 */
static const char *obter_credencial(const char *variavel, const char *padrao)
{
    const char *valor = getenv(variavel);
    return (valor != NULL && valor[0] != '\0') ? valor : padrao;
}

int login_autenticar(char *usuario_logado, size_t tamanho)
{
    const char *usuario_esperado = obter_credencial("DOGWALKER_USUARIO", USUARIO_PADRAO);
    const char *senha_esperada = obter_credencial("DOGWALKER_SENHA", SENHA_PADRAO);
    char rotulo_usuario[TAM_ENTRADA];
    char rotulo_senha[TAM_ENTRADA];
    char usuario[TAM_CREDENCIAL];
    char senha[TAM_CREDENCIAL];
    int tentativa;

    snprintf(rotulo_usuario, sizeof rotulo_usuario, "  %sUsuário:%s ",
             ui_codigo(UI_COR_ROTULO), ui_codigo(UI_COR_NORMAL));
    snprintf(rotulo_senha, sizeof rotulo_senha, "  %sSenha:%s   ",
             ui_codigo(UI_COR_ROTULO), ui_codigo(UI_COR_NORMAL));

    ui_cabecalho("TELA DE LOGIN");
    printf("\n");
    ui_info("Informe suas credenciais para acessar o sistema.");
    printf("\n");

    /* O original repetia o login para sempre. Limitar as tentativas é uma
     * medida básica contra tentativa e erro (força bruta). */
    for (tentativa = 1; tentativa <= LOGIN_MAX_TENTATIVAS; tentativa++) {
        ResultadoEntrada leitura_usuario;
        ResultadoEntrada leitura_senha;
        int restantes;

        leitura_usuario = ler_linha(rotulo_usuario, usuario, sizeof usuario);
        if (leitura_usuario == ENTRADA_FIM) {
            return 0;
        }
        leitura_senha = ler_senha(rotulo_senha, senha, sizeof senha);
        if (leitura_senha == ENTRADA_FIM) {
            return 0;
        }

        if (leitura_usuario == ENTRADA_OK && leitura_senha == ENTRADA_OK
            && strcmp(usuario, usuario_esperado) == 0
            && strcmp(senha, senha_esperada) == 0) {
            printf("\n");
            ui_sucesso("Login efetuado com sucesso! Bem-vindo(a), %s.", usuario);
            snprintf(usuario_logado, tamanho, "%s", usuario);
            return 1;
        }

        /* Mensagem genérica de propósito: não revelamos se o erro foi no
         * usuário ou na senha, para não ajudar quem está tentando adivinhar. */
        restantes = LOGIN_MAX_TENTATIVAS - tentativa;
        if (restantes > 0) {
            printf("\n");
            ui_erro("Usuário ou senha inválidos. Tentativas restantes: %d", restantes);
            printf("\n");
        }
    }

    printf("\n");
    ui_erro("Usuário ou senha inválidos. Número máximo de tentativas atingido.");
    return 0;
}
