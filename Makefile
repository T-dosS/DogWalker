# ---------------------------------------------------------------------------
# DogWalker - Gerenciador de Clientes
#
#   make           compila o programa
#   make run       compila (se necessário) e executa
#   make test      compila e executa os testes automatizados
#   make cenarios  testes de uso ponta a ponta (PowerShell no Windows,
#                  sh no Linux/macOS)
#   make lint      análise estática com cppcheck
#   make bench     mede o tempo de gravar e carregar muitos clientes
#   make check     tudo antes de publicar: build rigoroso, testes e lint
#   make clean     apaga os executáveis gerados
#
# Funciona com GCC no Linux/macOS e com MinGW-w64 no Windows
# (no Windows, use "mingw32-make" no Prompt de Comando ou PowerShell).
# ---------------------------------------------------------------------------

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -O2 -Iinclude
# Avisos extras usados no "make check": conversões implícitas e o analisador
# estático do próprio GCC (caminhos com vazamento, ponteiro nulo etc.).
STRICT  = -Wconversion -Wsign-conversion -fanalyzer

SRC       = $(wildcard src/*.c)
HEADERS   = $(wildcard include/*.h)
# Os testes usam todos os módulos, exceto o main.c do programa.
LIB_SRC   = $(filter-out src/main.c,$(SRC))
TEST_SRC  = tests/test_dogwalker.c
BENCH_SRC = tests/benchmark.c

CPPCHECK = cppcheck --enable=warning,style,performance,portability --std=c11 \
           --inline-suppr --error-exitcode=1 --quiet -Iinclude

ifeq ($(OS),Windows_NT)
    EXE      = .exe
    RM       = del /Q /F
    RUN      = .\\
    MKDATA   = if not exist data mkdir data
    CENARIOS = powershell -NoProfile -ExecutionPolicy Bypass -File tests/cenarios.ps1
else
    EXE      =
    RM       = rm -f
    RUN      = ./
    MKDATA   = mkdir -p data
    CENARIOS = sh tests/cenarios.sh ./$(BIN)
endif

BIN       = dogwalker$(EXE)
TEST_BIN  = test_dogwalker$(EXE)
BENCH_BIN = benchmark$(EXE)

.PHONY: all run test cenarios lint bench strict check clean

all: $(BIN)

$(BIN): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $@

run: $(BIN)
	$(MKDATA)
	$(RUN)$(BIN)

test: $(TEST_BIN)
	$(RUN)$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(LIB_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(TEST_SRC) $(LIB_SRC) -o $@

# Simula um usuário digitando e confere mensagens e o CSV.
cenarios: $(BIN)
	$(CENARIOS)

lint:
	$(CPPCHECK) src tests

bench: $(BENCH_BIN)
	$(RUN)$(BENCH_BIN)

$(BENCH_BIN): $(BENCH_SRC) $(LIB_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(BENCH_SRC) $(LIB_SRC) -o $@

# Compila com os avisos extras; qualquer aviso interrompe (-Werror).
strict:
	$(CC) $(CFLAGS) $(STRICT) -Werror $(SRC) -o $(BIN)

check: strict test lint cenarios

clean:
	-$(RM) $(BIN) $(TEST_BIN) $(BENCH_BIN)
