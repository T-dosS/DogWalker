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

/**
 * Faz uma pergunta de sim/não e repete até receber uma resposta válida.
 * Aceita s/sim e n/nao/não, em maiúsculas ou minúsculas.
 *
 * @return 1 para sim, 0 para não e -1 se a entrada terminar.
 */
int confirmar(const char *pergunta);

/** Espera o usuário apertar ENTER antes de voltar ao menu. */
void pausar(void);

/** @return 1 se a entrada padrão terminou (EOF ou erro), 0 caso contrário. */
int entrada_encerrada(void);

/** Imprime um título de tela entre linhas de separação. */
void imprimir_titulo(const char *titulo);

#endif /* TERMINAL_H */
