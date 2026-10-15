/**
 * @file main.c
 * @brief Ponto de entrada do DogWalker - Gerenciador de Clientes.
 *
 * Uso:
 *   dogwalker                     (usa data/clientes.csv)
 *   dogwalker caminho/arquivo.csv (usa outro arquivo de dados)
 *
 * O main apenas organiza o fluxo: configura o terminal, exige o login e
 * entrega o controle ao menu. As regras ficam nos módulos específicos.
 */
#include "arquivo.h"
#include "login.h"
#include "menu.h"
#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    const char *caminho_dados = (argc > 1) ? argv[1] : ARQUIVO_PADRAO;

    terminal_configurar();

    if (!login_autenticar()) {
        printf("\n  Acesso negado. Encerrando o programa.\n");
        return EXIT_FAILURE;
    }

    menu_executar(caminho_dados);
    return EXIT_SUCCESS;
}
