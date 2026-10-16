/**
 * @file terminal.c
 * @brief Leitura segura do teclado: linhas, senha e teclas de navegação.
 *
 * Concentra o código de teclado específico de cada sistema operacional,
 * isolado por #ifdef _WIN32. A parte visual (cores, bordas) fica em ui.c.
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L /* habilita termios/unistd com -std=c11 */
#endif

#include "terminal.h"
#include "validacao.h"

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

/* Códigos de tecla tratados na senha e nos menus. */
#define TECLA_CTRL_C     3
#define TECLA_CTRL_D     4   /* fim de entrada no Linux/macOS */
#define TECLA_BACKSPACE  8   /* Windows */
#define TECLA_CTRL_Z     26  /* fim de entrada no Windows */
#define TECLA_ESC        27
#define TECLA_DEL        127 /* Backspace no Linux/macOS */

/* No Windows, _getch devolve as setas como dois códigos: 0xE0 (ou 0) e
 * depois 72 (cima) ou 80 (baixo). */
#define WIN_PREFIXO_ESTENDIDA 0xE0
#define WIN_SETA_CIMA         72
#define WIN_SETA_BAIXO        80

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

int terminal_interativo(void)
{
#ifdef _WIN32
    DWORD modo;
    return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &modo) != 0;
#else
    return isatty(STDIN_FILENO);
#endif
}

/*
 * Modo "tecla a tecla", usado na senha e nos menus: cada tecla chega ao
 * programa na hora (sem esperar ENTER) e sem aparecer na tela.
 *
 * iniciar_modo_tecla() retorna 0 quando a entrada não é um teclado
 * (ex.: redirecionada de um arquivo nos testes); quem chama deve então
 * usar a leitura por linha.
 */
#ifdef _WIN32
static int iniciar_modo_tecla(void)
{
    /* _getch já lê tecla a tecla e sem eco; basta conferir se é um console. */
    return terminal_interativo();
}

static void encerrar_modo_tecla(void)
{
}

static int ler_tecla(void)
{
    return _getch();
}
#else
static struct termios config_original;

static int iniciar_modo_tecla(void)
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

static void encerrar_modo_tecla(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &config_original);
}

static int ler_tecla(void)
{
    return getchar();
}

/**
 * Lê uma tecla esperando no máximo 0,1 s. No Linux, Esc sozinho e as setas
 * (ESC [ A) começam com o mesmo byte; se nada chegar logo depois do ESC,
 * foi o Esc mesmo.
 */
static int ler_tecla_com_espera_curta(void)
{
    struct termios config;
    int tecla;

    tcgetattr(STDIN_FILENO, &config);
    config.c_cc[VMIN] = 0;
    config.c_cc[VTIME] = 1; /* décimos de segundo */
    tcsetattr(STDIN_FILENO, TCSANOW, &config);

    tecla = getchar();
    if (tecla == EOF) {
        clearerr(stdin); /* tempo esgotado não é fim de entrada */
    }

    config.c_cc[VMIN] = 1;
    config.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &config);
    return tecla;
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

    if (!iniciar_modo_tecla()) {
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
            encerrar_modo_tecla();
            printf("\n");
            return ENTRADA_FIM;
        }
#ifdef _WIN32
        if (tecla == 0 || tecla == WIN_PREFIXO_ESTENDIDA) {
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

    encerrar_modo_tecla();
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

TeclaNavegacao ler_tecla_navegacao(int *caractere)
{
    TeclaNavegacao resultado;
    int tecla;

    *caractere = 0;
    if (!iniciar_modo_tecla()) {
        return TECLA_NAV_FIM;
    }
    fflush(stdout);
    tecla = ler_tecla();

#ifdef _WIN32
    if (tecla == 0 || tecla == WIN_PREFIXO_ESTENDIDA) {
        int codigo = ler_tecla();
        encerrar_modo_tecla();
        if (codigo == WIN_SETA_CIMA) {
            return TECLA_NAV_CIMA;
        }
        return (codigo == WIN_SETA_BAIXO) ? TECLA_NAV_BAIXO : TECLA_NAV_OUTRA;
    }
#else
    if (tecla == TECLA_ESC) {
        int seguinte = ler_tecla_com_espera_curta();
        if (seguinte == '[' || seguinte == 'O') {
            int codigo = ler_tecla_com_espera_curta();
            encerrar_modo_tecla();
            if (codigo == 'A') {
                return TECLA_NAV_CIMA;
            }
            return (codigo == 'B') ? TECLA_NAV_BAIXO : TECLA_NAV_OUTRA;
        }
        /* nada (ou outra tecla) logo após o ESC: foi o Esc sozinho */
    }
#endif

    if (tecla == '\r' || tecla == '\n') {
        resultado = TECLA_NAV_ENTER;
    } else if (tecla == TECLA_ESC) {
        resultado = TECLA_NAV_ESC;
    } else if (tecla == EOF || tecla == TECLA_CTRL_C || tecla == TECLA_CTRL_D
               || tecla == TECLA_CTRL_Z) {
        resultado = TECLA_NAV_FIM;
    } else if (tecla >= 32 && tecla < 127) {
        *caractere = tecla;
        resultado = TECLA_NAV_CARACTERE;
    } else {
        resultado = TECLA_NAV_OUTRA;
    }

    encerrar_modo_tecla();
    return resultado;
}

int entrada_encerrada(void)
{
    return feof(stdin) || ferror(stdin);
}
