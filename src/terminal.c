/**
 * @file terminal.c
 * @brief Leitura segura do teclado e utilitários de tela.
 *
 * Único módulo (junto com a troca de arquivo em arquivo.c) que contém
 * código específico de sistema operacional, isolado por #ifdef _WIN32.
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L /* habilita termios/unistd com -std=c11 */
#endif

#include "terminal.h"
#include "validacao.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h> /* _getch: lê uma tecla sem esperar ENTER e sem eco */
#else
#include <termios.h>
#include <unistd.h>
#endif

#define LARGURA_TITULO 50

/* Códigos de tecla tratados na digitação da senha. */
#define TECLA_CTRL_C     3
#define TECLA_CTRL_D     4   /* fim de entrada no Linux/macOS */
#define TECLA_BACKSPACE  8   /* Windows */
#define TECLA_CTRL_Z     26  /* fim de entrada no Windows */
#define TECLA_DEL        127 /* Backspace no Linux/macOS */

/* ------------------------------------------------------------------ */
/* Código dependente de plataforma                                     */
/* ------------------------------------------------------------------ */

void terminal_configurar(void)
{
#ifdef _WIN32
    /* Os textos do programa estão em UTF-8. Sem isso, o console do
     * Windows (que usa a página de código 850/1252) mostraria "Ã§" no
     * lugar de "ç". Substitui o setlocale(LC_ALL, "Portuguese") original,
     * que só funcionava no Windows e não resolvia a codificação. */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

/*
 * Modo "tecla a tecla" usado na senha: cada tecla chega ao programa na
 * hora (sem esperar ENTER) e sem aparecer na tela, para que possamos
 * exibir um '*' no lugar de cada caractere.
 *
 * iniciar_modo_senha() retorna 0 quando a entrada não é um teclado
 * (ex.: redirecionada de um arquivo nos testes). Nesse caso a senha é
 * lida como uma linha comum.
 */
#ifdef _WIN32
static int iniciar_modo_senha(void)
{
    DWORD modo;
    /* _getch já lê tecla a tecla e sem eco; basta conferir se é um console. */
    return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &modo) != 0;
}

static void encerrar_modo_senha(void)
{
}

static int ler_tecla(void)
{
    return _getch();
}
#else
static struct termios config_original;

static int iniciar_modo_senha(void)
{
    struct termios config;

    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &config_original) != 0) {
        return 0;
    }
    config = config_original;
    /* ICANON: sem esperar ENTER; ECHO: sem eco; ISIG: Ctrl+C chega como
     * tecla (código 3) em vez de matar o programa com o terminal desconfigurado. */
    config.c_lflag &= ~(tcflag_t)(ICANON | ECHO | ISIG);
    config.c_cc[VMIN] = 1;
    config.c_cc[VTIME] = 0;
    return tcsetattr(STDIN_FILENO, TCSANOW, &config) == 0;
}

static void encerrar_modo_senha(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &config_original);
}

static int ler_tecla(void)
{
    return getchar();
}
#endif

/* ------------------------------------------------------------------ */
/* Leitura                                                             */
/* ------------------------------------------------------------------ */

/** Consome o que sobrou da linha atual no teclado. */
static void descartar_resto_da_linha(void)
{
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

ResultadoEntrada ler_linha(const char *rotulo, char *destino, size_t tamanho)
{
    char linha[TAM_ENTRADA];
    char *quebra;
    size_t comprimento;

    if (rotulo != NULL) {
        printf("%s", rotulo);
        /* fflush(stdout) é o uso CORRETO de fflush: garante que a pergunta
         * apareça antes de o programa parar para ler. (fflush(stdin), usado
         * no código original, é comportamento indefinido em C.) */
        fflush(stdout);
    }

    if (fgets(linha, sizeof linha, stdin) == NULL) {
        return ENTRADA_FIM;
    }

    quebra = strchr(linha, '\n');
    if (quebra == NULL && !feof(stdin)) {
        /* Não achou '\n': a linha não coube no buffer. Descartamos o resto
         * para que ele não seja lido como resposta da próxima pergunta. */
        descartar_resto_da_linha();
        return ENTRADA_LONGA;
    }
    if (quebra != NULL) {
        *quebra = '\0';
    }

    aparar_texto(linha); /* também remove o '\r' de entradas do Windows */
    if (linha[0] == '\0') {
        return ENTRADA_VAZIA;
    }

    comprimento = strlen(linha);
    if (comprimento >= tamanho) {
        return ENTRADA_LONGA;
    }
    memcpy(destino, linha, comprimento + 1);
    return ENTRADA_OK;
}

ResultadoEntrada ler_senha(const char *rotulo, char *destino, size_t tamanho)
{
    char senha[TAM_ENTRADA];
    size_t comprimento = 0;
    int excedeu = 0;

    if (!iniciar_modo_senha()) {
        return ler_linha(rotulo, destino, tamanho);
    }

    printf("%s", rotulo);
    fflush(stdout);

    for (;;) {
        int tecla = ler_tecla();

        if (tecla == '\r' || tecla == '\n') {
            break;
        }
        if (tecla == EOF || tecla == TECLA_CTRL_C || tecla == TECLA_CTRL_D
            || tecla == TECLA_CTRL_Z) {
            encerrar_modo_senha();
            printf("\n");
            return ENTRADA_FIM;
        }
#ifdef _WIN32
        if (tecla == 0 || tecla == 0xE0) {
            (void)ler_tecla(); /* setas e teclas F enviam dois códigos: ignora */
            continue;
        }
#endif
        if (tecla == TECLA_BACKSPACE || tecla == TECLA_DEL) {
            if (comprimento > 0) {
                comprimento--;
                printf("\b \b"); /* volta, apaga o '*' e volta de novo */
                fflush(stdout);
            }
            continue;
        }
        if (tecla < 32) {
            continue; /* outras teclas de controle (Tab, Esc...) */
        }

        if (comprimento < sizeof senha - 1) {
            senha[comprimento++] = (char)tecla;
            putchar('*');
            fflush(stdout);
        } else {
            excedeu = 1;
        }
    }

    encerrar_modo_senha();
    printf("\n");

    senha[comprimento] = '\0';
    if (comprimento == 0) {
        return ENTRADA_VAZIA;
    }
    if (excedeu || comprimento >= tamanho) {
        return ENTRADA_LONGA;
    }
    memcpy(destino, senha, comprimento + 1);
    return ENTRADA_OK;
}

int confirmar(const char *pergunta)
{
    char rotulo[TAM_ENTRADA];
    char resposta[TAM_ENTRADA];
    size_t i;

    snprintf(rotulo, sizeof rotulo, "%s [s/n]: ", pergunta);

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
        printf("    ! Responda com 's' para sim ou 'n' para não.\n");
    }
}

void pausar(void)
{
    char descarte[TAM_ENTRADA];

    if (!entrada_encerrada()) {
        (void)ler_linha("\n  Pressione ENTER para voltar ao menu...",
                        descarte, sizeof descarte);
    }
}

int entrada_encerrada(void)
{
    return feof(stdin) || ferror(stdin);
}

/* ------------------------------------------------------------------ */
/* Exibição                                                            */
/* ------------------------------------------------------------------ */

static void imprimir_linha(char caractere)
{
    int i;
    for (i = 0; i < LARGURA_TITULO; i++) {
        putchar(caractere);
    }
    putchar('\n');
}

void imprimir_titulo(const char *titulo)
{
    /* No lugar de system("cls"), que só existe no Windows e executa um
     * processo externo, separamos as telas com títulos bem visíveis. */
    printf("\n");
    imprimir_linha('=');
    printf("  %s\n", titulo);
    imprimir_linha('=');
}
