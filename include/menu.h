/**
 * @file menu.h
 * @brief Menu principal e telas de cadastro, alteração, listagem e remoção.
 */
#ifndef MENU_H
#define MENU_H

/**
 * Exibe o menu principal em laço até o usuário escolher "Encerrar"
 * ou a entrada terminar.
 *
 * @param caminho_arquivo arquivo CSV onde os clientes são armazenados.
 */
void menu_executar(const char *caminho_arquivo);

#endif /* MENU_H */
