/**
 * @file ui.h
 * @brief Camada visual do terminal: cores, bordas, mensagens e menus.
 *
 * Usa apenas sequências ANSI e caracteres Unicode, que terminais modernos
 * entendem nativamente (Windows 10+, Windows Terminal, VS Code, Linux, macOS).
 * Nenhuma biblioteca externa é necessária.
 *
 * O módulo trabalha em dois modos, escolhidos automaticamente:
 *
 * - **Rico:** a saída é um terminal com suporte a ANSI. Há cores, bordas
 *   Unicode, tela limpa a cada tela e menu navegável com as setas.
 * - **Simples:** a saída é um arquivo/pipe (ex.: testes automatizados), o
 *   terminal é antigo, ou o usuário definiu NO_COLOR ou DOGWALKER_SIMPLES=1.
 *   Há texto puro com bordas ASCII, e o menu é lido como uma linha.
 *
 * Assim a aparência melhora sem quebrar a execução por script nem a
 * compatibilidade com terminais limitados.
 */
#ifndef UI_H
#define UI_H

#include <stddef.h>

#if defined(__GNUC__)
#define UI_FORMATO_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define UI_FORMATO_PRINTF(fmt, args)
#endif

/** Largura (em colunas) das caixas e tabelas; cabe em terminais de 80 colunas. */
#define UI_LARGURA 76

/** Papéis de cor. As cores reais ficam centralizadas em ui.c. */
typedef enum {
    UI_COR_NORMAL,
    UI_COR_MARCA,     /**< nome do sistema no cabeçalho */
    UI_COR_TITULO,    /**< título da tela                */
    UI_COR_DESTAQUE,  /**< item selecionado no menu      */
    UI_COR_ROTULO,    /**< rótulos de campos             */
    UI_COR_SUAVE,     /**< textos secundários e dicas    */
    UI_COR_SUCESSO,
    UI_COR_ERRO,
    UI_COR_AVISO
} UiCor;

/** Uma opção do menu: tecla de atalho, texto e descrição curta. */
typedef struct {
    char numero;           /**< tecla numérica, ex.: '1'         */
    char letra;            /**< atalho por letra, ex.: 'C'        */
    const char *texto;     /**< ex.: "Cadastrar cliente"          */
    const char *descricao; /**< exibida para o item selecionado  */
} UiItemMenu;

/** Retornos especiais de ui_menu(). */
#define UI_MENU_FIM  (-1) /**< a entrada terminou (Ctrl+Z / Ctrl+D)   */
#define UI_MENU_SAIR (-2) /**< o usuário apertou Esc                  */

/** Detecta o modo (rico ou simples). Chamar uma vez, no início do programa. */
void ui_iniciar(void);

/** @return 1 se cores e bordas Unicode estão ativas, 0 no modo simples. */
int ui_modo_rico(void);

/** @return a sequência ANSI da cor, ou "" no modo simples. */
const char *ui_codigo(UiCor cor);

/** Limpa a tela (somente no modo rico; no simples não faz nada). */
void ui_limpar_tela(void);

/** Limpa a tela e desenha o cabeçalho com a marca e o título da tela. */
void ui_cabecalho(const char *titulo);

/* Mensagens padronizadas com ícone e cor. Aceitam formato de printf. */
void ui_sucesso(const char *formato, ...) UI_FORMATO_PRINTF(1, 2);
void ui_erro(const char *formato, ...) UI_FORMATO_PRINTF(1, 2);
void ui_aviso(const char *formato, ...) UI_FORMATO_PRINTF(1, 2);
void ui_info(const char *formato, ...) UI_FORMATO_PRINTF(1, 2);

/**
 * Quantas colunas o texto ocupa na tela. Em UTF-8, "ç" ocupa 2 bytes mas
 * 1 coluna, então strlen() não serve para alinhar textos acentuados.
 */
size_t ui_largura_texto(const char *texto);

/**
 * Copia no máximo `largura` colunas de `texto` para `saida`, sem cortar
 * um caractere UTF-8 ao meio. Se o texto não couber, termina com "...".
 */
void ui_truncar(const char *texto, size_t largura, char *saida, size_t tamanho);

/* Tabelas: desenham bordas e células alinhadas às larguras informadas. */
void ui_tabela_topo(const size_t larguras[], size_t colunas);
void ui_tabela_divisoria(const size_t larguras[], size_t colunas);
void ui_tabela_base(const size_t larguras[], size_t colunas);
void ui_tabela_linha(const char *const celulas[], const size_t larguras[],
                     size_t colunas, UiCor cor);

/**
 * Mostra um menu e devolve o índice da opção escolhida.
 *
 * - Modo rico com teclado: setas ↑↓ + Enter, tecla numérica ou letra de
 *   atalho (seleção imediata) e Esc para sair.
 * - Modo simples: lista numerada e leitura de uma linha ("1" ou "C").
 *
 * @param status       linha de status exibida abaixo do cabeçalho (ou NULL).
 * @param selecionado  posição inicial do destaque; recebe a última posição.
 * @return índice da opção, UI_MENU_SAIR (Esc) ou UI_MENU_FIM (fim da entrada).
 */
int ui_menu(const char *titulo, const char *status, const UiItemMenu itens[],
            int quantidade, int *selecionado);

/**
 * Pergunta de sim/não, repetida até uma resposta válida.
 * @return 1 para sim, 0 para não e -1 se a entrada terminar.
 */
int ui_confirmar(const char *pergunta);

/** Espera o usuário apertar ENTER antes de voltar ao menu. */
void ui_pausar(void);

#endif /* UI_H */
