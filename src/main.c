/**
 * @file main.c
 * @brief Ponto de entrada do DogWalker - Gerenciador de Clientes.
 *
 * Uso:
 *   dogwalker                     (usa data/clientes.csv)
 *   dogwalker caminho/arquivo.csv (usa outro arquivo de dados)
 *
 * Variáveis de ambiente opcionais:
 *   NO_COLOR=1 ou DOGWALKER_SIMPLES=1  desliga cores e bordas Unicode
 *   DOGWALKER_USUARIO / DOGWALKER_SENHA trocam as credenciais padrão
 *
 * O main apenas organiza o fluxo: configura o terminal, exige o login e
 * entrega o controle ao menu. As regras ficam nos módulos específicos.
 */
#include "arquivo.h"
#include "login.h"
#include "menu.h"
#include "terminal.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    const char *caminho_dados = (argc > 1) ? argv[1] : ARQUIVO_PADRAO;
    char usuario[LOGIN_TAM_USUARIO];

    terminal_configurar(); /* UTF-8 no console do Windows      */
    ui_iniciar();          /* decide entre modo rico e simples */

    if (!login_autenticar(usuario, sizeof usuario)) {
        printf("\n  Acesso negado. Encerrando o programa.\n");
        return EXIT_FAILURE;
    }

    menu_executar(caminho_dados, usuario);
    return EXIT_SUCCESS;
}
