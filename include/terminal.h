/**
 * @file terminal.h
 * @brief Leitura segura do teclado e utilitários de exibição no terminal.
 *
 * Toda leitura do programa passa por ler_linha(), que usa fgets() com
 * limite de tamanho. Isso substitui o scanf("%s") e o fflush(stdin) do
 * código original, que causavam estouro de buffer e comportamento
 * indefinido.
 *
 * Também concentra o pouco código específico de sistema operacional
 * (Windows x Linux), para que o resto do projeto seja C padrão.
 */
#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>

/** Tamanho máximo de uma linha digitada (linhas maiores são rejeitadas). */
#define TAM_ENTRADA 256

/** Resultado de uma leitura do teclado. */
typedef enum {
    ENTRADA_OK,     /**< Texto lido com sucesso.                          */
    ENTRADA_VAZIA,  /**< Usuário apertou ENTER sem digitar nada.          */
    ENTRADA_LONGA,  /**< Texto maior que o permitido (foi descartado).    */
    ENTRADA_FIM     /**< Fim da entrada (Ctrl+Z / Ctrl+D ou fim do pipe). */
} ResultadoEntrada;

/**
 * Prepara o terminal: no Windows ativa UTF-8 para que acentos apareçam
 * corretamente. No Linux/macOS não é necessário fazer nada.
 * (Cores e bordas ficam a cargo do módulo ui.)
 */
void terminal_configurar(void);

/**
 * Mostra o rótulo e lê uma linha inteira do teclado.
 *
 * Remove a quebra de linha e os espaços das pontas. Se o texto não couber
 * em `tamanho` (incluindo o '\0'), retorna ENTRADA_LONGA e descarta o
 * restante da linha, para que ele não "vaze" para a próxima pergunta.
 *
 * @param rotulo   texto exibido antes da leitura (pode ser NULL).
 * @param destino  buffer que recebe o texto (só é alterado em ENTRADA_OK).
 * @param tamanho  tamanho do buffer destino.
 */
ResultadoEntrada ler_linha(const char *rotulo, char *destino, size_t tamanho);

/**
 * Lê uma senha mostrando '*' no lugar de cada caractere digitado.
 * Backspace apaga o último caractere; Ctrl+C/Ctrl+Z/Ctrl+D retornam
 * ENTRADA_FIM. Se a entrada não for um teclado (ex.: redirecionada de um
 * arquivo), a senha é lida como uma linha comum.
 */
ResultadoEntrada ler_senha(const char *rotulo, char *destino, size_t tamanho);

/** @return 1 se a entrada padrão terminou (EOF ou erro), 0 caso contrário. */
int entrada_encerrada(void);

/**
 * @return 1 se a entrada vem de um teclado (console/terminal), 0 se foi
 *         redirecionada de um arquivo ou pipe (ex.: testes automatizados).
 */
int terminal_interativo(void);

/** Teclas reconhecidas na navegação dos menus. */
typedef enum {
    TECLA_NAV_CIMA,
    TECLA_NAV_BAIXO,
    TECLA_NAV_ENTER,
    TECLA_NAV_ESC,
    TECLA_NAV_CARACTERE, /**< tecla imprimível, devolvida em *caractere */
    TECLA_NAV_OUTRA,     /**< tecla sem uso no menu (F1, Tab...)       */
    TECLA_NAV_FIM        /**< Ctrl+C / Ctrl+Z / Ctrl+D ou sem teclado   */
} TeclaNavegacao;

/**
 * Lê UMA tecla, sem esperar ENTER e sem eco, traduzindo as setas.
 * As setas chegam como sequências diferentes no Windows (0xE0 + código)
 * e no Linux/macOS (ESC [ A), e esta função esconde essa diferença.
 *
 * @param caractere recebe a tecla quando o retorno é TECLA_NAV_CARACTERE.
 */
TeclaNavegacao ler_tecla_navegacao(int *caractere);

#endif /* TERMINAL_H */
