/**
 * @file ui.c
 * @brief Camada visual: cores ANSI, bordas Unicode, mensagens, tabelas e menu.
 *
 * Toda a aparência do programa passa por aqui. As outras partes do código
 * pedem "mostre um erro" ou "desenhe uma tabela" e não sabem se o terminal
 * tem cores. Isso mantém a regra de negócio livre de detalhes visuais e
 * permite desligar todo o estilo em um único ponto (modo simples).
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L /* isatty/STDOUT_FILENO com -std=c11 */
#endif

#include "ui.h"
#include "terminal.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

/* Começa no modo simples: assim, quem usa o módulo sem chamar ui_iniciar()
 * (ex.: os testes automatizados) recebe sempre texto puro e previsível. */
static int modo_rico = 0;

/* ------------------------------------------------------------------ */
/* Paleta e símbolos                                                   */
/* ------------------------------------------------------------------ */

/* Códigos ANSI na mesma ordem do enum UiCor. Mudar a identidade visual do
 * programa inteiro é só mudar esta tabela. */
static const char *const CODIGOS_COR[] = {
    "\033[0m",        /* NORMAL   */
    "\033[1;36m",     /* MARCA    ciano forte    */
    "\033[1;97m",     /* TITULO   branco forte   */
    "\033[30;106m",   /* DESTAQUE preto em ciano claro (sem negrito: no
                       * Windows, negrito + preto vira cinza e perde contraste) */
    "\033[36m",       /* ROTULO   ciano          */
    "\033[90m",       /* SUAVE    cinza          */
    "\033[1;32m",     /* SUCESSO  verde          */
    "\033[1;31m",     /* ERRO     vermelho       */
    "\033[1;33m"      /* AVISO    amarelo        */
};

/** Caracteres de borda: Unicode no modo rico, ASCII no simples. */
typedef struct {
    const char *horizontal, *vertical;
    const char *sup_esq, *sup_dir, *inf_esq, *inf_dir;
    const char *t_esq, *t_dir, *t_cima, *t_baixo, *cruz;
} Bordas;

static const Bordas BORDAS_RICAS = {
    "─", "│", "╭", "╮", "╰", "╯", "├", "┤", "┬", "┴", "┼"
};
static const Bordas BORDAS_SIMPLES = {
    "-", "|", "+", "+", "+", "+", "+", "+", "+", "+", "+"
};

/** Ícones das mensagens e do menu. */
typedef struct {
    const char *sucesso, *erro, *aviso, *info, *seletor, *reticencias, *separador;
} Icones;

static const Icones ICONES_RICOS = { "✔", "✖", "⚠", "●", "❯", "…", " • " };
static const Icones ICONES_SIMPLES = { "[OK]", "[ERRO]", "[AVISO]", "[i]", ">", "...", " | " };

static const Bordas *bordas(void)
{
    return modo_rico ? &BORDAS_RICAS : &BORDAS_SIMPLES;
}

static const Icones *icones(void)
{
    return modo_rico ? &ICONES_RICOS : &ICONES_SIMPLES;
}

/* ------------------------------------------------------------------ */
/* Inicialização                                                       */
/* ------------------------------------------------------------------ */

/** Garante que o terminal volte ao normal ao sair (cor padrão, cursor visível). */
static void restaurar_terminal(void)
{
    fputs("\033[0m\033[?25h", stdout);
    fflush(stdout);
}

/**
 * Verifica se a saída é um terminal capaz de interpretar códigos ANSI.
 * No Windows, o suporte existe desde o Windows 10, mas precisa ser ligado
 * com ENABLE_VIRTUAL_TERMINAL_PROCESSING; se falhar, o console é antigo.
 */
static int detectar_suporte_ansi(void)
{
#ifdef _WIN32
    HANDLE saida = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo;

    if (!GetConsoleMode(saida, &modo)) {
        return 0; /* saída redirecionada para arquivo/pipe */
    }
    return (modo & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        || SetConsoleMode(saida, modo | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    const char *term = getenv("TERM");
    return isatty(STDOUT_FILENO) && term != NULL && strcmp(term, "dumb") != 0;
#endif
}

void ui_iniciar(void)
{
    /* NO_COLOR é uma convenção adotada por várias ferramentas de terminal
     * (no-color.org). DOGWALKER_SIMPLES=1 força o modo texto puro. */
    const char *sem_cor = getenv("NO_COLOR");
    const char *simples = getenv("DOGWALKER_SIMPLES");

    if ((sem_cor != NULL && sem_cor[0] != '\0')
        || (simples != NULL && simples[0] != '\0' && strcmp(simples, "0") != 0)) {
        modo_rico = 0;
        return;
    }
    modo_rico = detectar_suporte_ansi();
    if (modo_rico) {
        atexit(restaurar_terminal);
    }
}

int ui_modo_rico(void)
{
    return modo_rico;
}

const char *ui_codigo(UiCor cor)
{
    return modo_rico ? CODIGOS_COR[cor] : "";
}

void ui_limpar_tela(void)
{
    if (modo_rico) {
        /* cursor para o topo + apaga a tela: substitui o system("cls") original */
        fputs("\033[H\033[2J", stdout);
    }
}

/* ------------------------------------------------------------------ */
/* Texto e alinhamento                                                 */
/* ------------------------------------------------------------------ */

/* Em UTF-8, bytes no formato 10xxxxxx continuam um caractere já iniciado. */
static int byte_de_continuacao(char c)
{
    return ((unsigned char)c & 0xC0) == 0x80;
}

size_t ui_largura_texto(const char *texto)
{
    size_t colunas = 0;
    for (; *texto != '\0'; texto++) {
        if (!byte_de_continuacao(*texto)) {
            colunas++;
        }
    }
    return colunas;
}

void ui_truncar(const char *texto, size_t largura, char *saida, size_t tamanho)
{
    const char *reticencias = icones()->reticencias;
    size_t largura_ret = ui_largura_texto(reticencias);
    size_t limite;
    size_t colunas = 0;
    size_t bytes = 0;

    if (ui_largura_texto(texto) <= largura) {
        snprintf(saida, tamanho, "%s", texto);
        return;
    }

    /* Avança caractere por caractere (não byte por byte) até sobrar espaço
     * apenas para as reticências. */
    limite = (largura > largura_ret) ? largura - largura_ret : 0;
    while (texto[bytes] != '\0' && colunas < limite) {
        bytes++;
        while (byte_de_continuacao(texto[bytes])) {
            bytes++;
        }
        colunas++;
    }
    snprintf(saida, tamanho, "%.*s%s", (int)bytes, texto, reticencias);
}

static void repetir(const char *trecho, size_t vezes)
{
    size_t i;
    for (i = 0; i < vezes; i++) {
        fputs(trecho, stdout);
    }
}

/** Escreve o texto e completa com espaços até ocupar `largura` colunas. */
static void escrever_alinhado(const char *texto, size_t largura)
{
    size_t ocupado = ui_largura_texto(texto);
    fputs(texto, stdout);
    if (ocupado < largura) {
        repetir(" ", largura - ocupado);
    }
}

/* ------------------------------------------------------------------ */
/* Cabeçalho e mensagens                                               */
/* ------------------------------------------------------------------ */

/** Uma linha dentro da caixa: │ <texto colorido> <espaços> │ */
static void linha_de_caixa(UiCor cor, const char *texto, const char *extra, UiCor cor_extra)
{
    const Bordas *b = bordas();
    size_t interno = UI_LARGURA - 4; /* bordas + um espaço de cada lado */
    size_t ocupado = ui_largura_texto(texto) + (extra ? ui_largura_texto(extra) : 0);

    printf("  %s %s%s%s", b->vertical, ui_codigo(cor), texto, ui_codigo(UI_COR_NORMAL));
    if (extra != NULL) {
        printf("%s%s%s", ui_codigo(cor_extra), extra, ui_codigo(UI_COR_NORMAL));
    }
    repetir(" ", ocupado < interno ? interno - ocupado : 0);
    printf(" %s\n", b->vertical);
}

void ui_cabecalho(const char *titulo)
{
    const Bordas *b = bordas();

    ui_limpar_tela();
    printf("\n  %s", b->sup_esq);
    repetir(b->horizontal, UI_LARGURA - 2);
    printf("%s\n", b->sup_dir);

    linha_de_caixa(UI_COR_MARCA, "DOGWALKER",
                   modo_rico ? " · Gerenciador de Clientes" : " - Gerenciador de Clientes",
                   UI_COR_SUAVE);
    linha_de_caixa(UI_COR_TITULO, titulo, NULL, UI_COR_NORMAL);

    printf("  %s", b->inf_esq);
    repetir(b->horizontal, UI_LARGURA - 2);
    printf("%s\n", b->inf_dir);
}

/** Corpo comum das mensagens: ícone colorido + texto formatado. */
static void mensagem(UiCor cor, const char *icone, const char *formato, va_list args)
    UI_FORMATO_PRINTF(3, 0);

static void mensagem(UiCor cor, const char *icone, const char *formato, va_list args)
{
    printf("  %s%s ", ui_codigo(cor), icone);
    vprintf(formato, args);
    printf("%s\n", ui_codigo(UI_COR_NORMAL));
}

void ui_sucesso(const char *formato, ...)
{
    va_list args;
    va_start(args, formato);
    mensagem(UI_COR_SUCESSO, icones()->sucesso, formato, args);
    va_end(args);
}

void ui_erro(const char *formato, ...)
{
    va_list args;
    va_start(args, formato);
    mensagem(UI_COR_ERRO, icones()->erro, formato, args);
    va_end(args);
}

void ui_aviso(const char *formato, ...)
{
    va_list args;
    va_start(args, formato);
    mensagem(UI_COR_AVISO, icones()->aviso, formato, args);
    va_end(args);
}

void ui_info(const char *formato, ...)
{
    va_list args;
    va_start(args, formato);
    mensagem(UI_COR_ROTULO, icones()->info, formato, args);
    va_end(args);
}

/* ------------------------------------------------------------------ */
/* Tabelas                                                             */
/* ------------------------------------------------------------------ */

/** Desenha uma borda horizontal da tabela com os conectores indicados. */
static void borda_tabela(const size_t larguras[], size_t colunas,
                         const char *esquerda, const char *meio, const char *direita)
{
    size_t i;

    printf("  %s", esquerda);
    for (i = 0; i < colunas; i++) {
        repetir(bordas()->horizontal, larguras[i] + 2);
        fputs(i + 1 < colunas ? meio : direita, stdout);
    }
    putchar('\n');
}

void ui_tabela_topo(const size_t larguras[], size_t colunas)
{
    const Bordas *b = bordas();
    borda_tabela(larguras, colunas, b->sup_esq, b->t_cima, b->sup_dir);
}

void ui_tabela_divisoria(const size_t larguras[], size_t colunas)
{
    const Bordas *b = bordas();
    borda_tabela(larguras, colunas, b->t_esq, b->cruz, b->t_dir);
}

void ui_tabela_base(const size_t larguras[], size_t colunas)
{
    const Bordas *b = bordas();
    borda_tabela(larguras, colunas, b->inf_esq, b->t_baixo, b->inf_dir);
}

void ui_tabela_linha(const char *const celulas[], const size_t larguras[],
                     size_t colunas, UiCor cor)
{
    char celula[TAM_ENTRADA];
    size_t i;

    printf("  %s", bordas()->vertical);
    for (i = 0; i < colunas; i++) {
        ui_truncar(celulas[i], larguras[i], celula, sizeof celula);
        printf(" %s", ui_codigo(cor));
        escrever_alinhado(celula, larguras[i]);
        printf("%s %s", ui_codigo(UI_COR_NORMAL), bordas()->vertical);
    }
    putchar('\n');
}

/* ------------------------------------------------------------------ */
/* Menu                                                                */
/* ------------------------------------------------------------------ */

/** @return o índice do item cuja tecla (número ou letra) foi digitada, ou -1. */
static int item_por_tecla(const UiItemMenu itens[], int quantidade, int tecla)
{
    int i;
    int maiuscula = toupper((unsigned char)tecla);

    for (i = 0; i < quantidade; i++) {
        if (tecla == itens[i].numero || maiuscula == itens[i].letra) {
            return i;
        }
    }
    return -1;
}

static void exibir_status(const char *status)
{
    if (status != NULL) {
        printf("\n  %s%s%s\n", ui_codigo(UI_COR_SUAVE), status, ui_codigo(UI_COR_NORMAL));
    }
}

/** Desenha o menu navegável, com o item `atual` destacado. */
static void desenhar_menu(const char *titulo, const char *status,
                          const UiItemMenu itens[], int quantidade, int atual)
{
    const size_t largura_item = 30;
    int i;

    ui_cabecalho(titulo);
    exibir_status(status);
    printf("\n");

    for (i = 0; i < quantidade; i++) {
        if (i == atual) {
            printf("  %s %s %c  ", ui_codigo(UI_COR_DESTAQUE), icones()->seletor, itens[i].numero);
            escrever_alinhado(itens[i].texto, largura_item);
            printf(" %c %s\n", itens[i].letra, ui_codigo(UI_COR_NORMAL));
        } else {
            printf("     %s%c%s  ", ui_codigo(UI_COR_ROTULO), itens[i].numero,
                   ui_codigo(UI_COR_NORMAL));
            escrever_alinhado(itens[i].texto, largura_item);
            printf(" %s%c%s\n", ui_codigo(UI_COR_SUAVE), itens[i].letra,
                   ui_codigo(UI_COR_NORMAL));
        }
    }

    printf("\n  %s%s %s%s\n", ui_codigo(UI_COR_ROTULO), icones()->info,
           itens[atual].descricao, ui_codigo(UI_COR_NORMAL));
    printf("\n  %s", ui_codigo(UI_COR_SUAVE));
    repetir(bordas()->horizontal, UI_LARGURA - 2);
    printf("\n  ↑↓ navegar   Enter selecionar   1-%d ou letra: atalho   Esc sair%s\n",
           quantidade, ui_codigo(UI_COR_NORMAL));
}

/** Menu do modo rico: responde a cada tecla, sem precisar de ENTER. */
static int menu_interativo(const char *titulo, const char *status,
                           const UiItemMenu itens[], int quantidade, int *selecionado)
{
    int atual = (*selecionado >= 0 && *selecionado < quantidade) ? *selecionado : 0;
    int escolha = UI_MENU_FIM;
    int decidido = 0;

    fputs("\033[?25l", stdout); /* esconde o cursor enquanto navega */
    while (!decidido) {
        int caractere;
        int indice;

        desenhar_menu(titulo, status, itens, quantidade, atual);
        switch (ler_tecla_navegacao(&caractere)) {
        case TECLA_NAV_CIMA:
            atual = (atual + quantidade - 1) % quantidade; /* circular */
            break;
        case TECLA_NAV_BAIXO:
            atual = (atual + 1) % quantidade;
            break;
        case TECLA_NAV_ENTER:
            escolha = atual;
            decidido = 1;
            break;
        case TECLA_NAV_ESC:
            escolha = UI_MENU_SAIR;
            decidido = 1;
            break;
        case TECLA_NAV_FIM:
            escolha = UI_MENU_FIM;
            decidido = 1;
            break;
        case TECLA_NAV_CARACTERE:
            indice = item_por_tecla(itens, quantidade, caractere);
            if (indice >= 0) {
                atual = indice;
                escolha = indice;
                decidido = 1;
            }
            break;
        case TECLA_NAV_OUTRA:
            break;
        }
    }
    fputs("\033[?25h", stdout);
    *selecionado = atual;
    return escolha;
}

/** Menu do modo simples: lista numerada e leitura de uma linha. */
static int menu_por_linha(const char *titulo, const char *status,
                          const UiItemMenu itens[], int quantidade)
{
    char entrada[TAM_ENTRADA];
    int i;

    ui_cabecalho(titulo);
    exibir_status(status);
    printf("\n");
    for (i = 0; i < quantidade; i++) {
        printf("  [%c] ", itens[i].numero);
        escrever_alinhado(itens[i].texto, 30);
        printf(" (%c)\n", itens[i].letra);
    }

    for (;;) {
        ResultadoEntrada leitura =
            ler_linha("\n  Escolha a opção desejada: ", entrada, sizeof entrada);

        if (leitura == ENTRADA_FIM) {
            return UI_MENU_FIM;
        }
        if (leitura == ENTRADA_OK && strlen(entrada) == 1) {
            int indice = item_por_tecla(itens, quantidade, entrada[0]);
            if (indice >= 0) {
                return indice;
            }
        }
        ui_erro("Opção inválida. Digite um número de 1 a %d ou a letra do atalho.",
                quantidade);
    }
}

int ui_menu(const char *titulo, const char *status, const UiItemMenu itens[],
            int quantidade, int *selecionado)
{
    int escolha;

    /* Setas exigem saída com ANSI (para redesenhar) e entrada pelo teclado. */
    if (modo_rico && terminal_interativo()) {
        return menu_interativo(titulo, status, itens, quantidade, selecionado);
    }
    escolha = menu_por_linha(titulo, status, itens, quantidade);
    if (escolha >= 0) {
        *selecionado = escolha;
    }
    return escolha;
}

/* ------------------------------------------------------------------ */
/* Interações simples                                                  */
/* ------------------------------------------------------------------ */

int ui_confirmar(const char *pergunta)
{
    char rotulo[TAM_ENTRADA];
    char resposta[TAM_ENTRADA];
    size_t i;

    snprintf(rotulo, sizeof rotulo, "%s%s%s %s[s/n]%s: ", ui_codigo(UI_COR_TITULO), pergunta,
             ui_codigo(UI_COR_NORMAL), ui_codigo(UI_COR_SUAVE), ui_codigo(UI_COR_NORMAL));

    for (;;) {
        ResultadoEntrada resultado = ler_linha(rotulo, resposta, sizeof resposta);
        if (resultado == ENTRADA_FIM) {
            return -1;
        }
        if (resultado == ENTRADA_OK) {
            for (i = 0; resposta[i] != '\0'; i++) {
                resposta[i] = (char)tolower((unsigned char)resposta[i]);
            }
            if (strcmp(resposta, "s") == 0 || strcmp(resposta, "sim") == 0) {
                return 1;
            }
            if (strcmp(resposta, "n") == 0 || strcmp(resposta, "nao") == 0
                || strcmp(resposta, "não") == 0) {
                return 0;
            }
        }
        ui_erro("Responda com 's' para sim ou 'n' para não.");
    }
}

void ui_pausar(void)
{
    char rotulo[TAM_ENTRADA];
    char descarte[TAM_ENTRADA];

    if (entrada_encerrada()) {
        return;
    }
    snprintf(rotulo, sizeof rotulo, "\n  %sPressione ENTER para voltar ao menu...%s",
             ui_codigo(UI_COR_SUAVE), ui_codigo(UI_COR_NORMAL));
    (void)ler_linha(rotulo, descarte, sizeof descarte);
}
