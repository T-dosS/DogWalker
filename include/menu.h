/**
 * @file menu.h
 * @brief Menu principal e telas de cadastro, alteração, listagem e remoção.
 */
#ifndef MENU_H
#define MENU_H

/**
 * Exibe o menu principal em laço até o usuário escolher "Encerrar",
 * apertar Esc ou a entrada terminar.
 *
 * @param caminho_arquivo arquivo CSV onde os clientes são armazenados.
 * @param usuario         usuário logado, exibido na barra de status.
 */
void menu_executar(const char *caminho_arquivo, const char *usuario);

#endif /* MENU_H */
