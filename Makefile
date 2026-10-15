# ---------------------------------------------------------------------------
# DogWalker - Gerenciador de Clientes
#
#   make        compila o programa
#   make run    compila (se necessário) e executa
#   make test   compila e executa os testes automatizados
#   make clean  apaga os executáveis gerados
#
# Funciona com GCC no Linux/macOS e com MinGW-w64 no Windows
# (no Windows, use "mingw32-make" no Prompt de Comando ou PowerShell).
# ---------------------------------------------------------------------------

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -O2 -Iinclude

SRC      = $(wildcard src/*.c)
HEADERS  = $(wildcard include/*.h)
# Os testes usam todos os módulos, exceto o main.c do programa.
LIB_SRC  = $(filter-out src/main.c,$(SRC))
TEST_SRC = tests/test_dogwalker.c

ifeq ($(OS),Windows_NT)
    EXE      = .exe
    RM       = del /Q /F
    RUN      = .\\
    MKDATA   = if not exist data mkdir data
else
    EXE      =
    RM       = rm -f
    RUN      = ./
    MKDATA   = mkdir -p data
endif

BIN      = dogwalker$(EXE)
TEST_BIN = test_dogwalker$(EXE)

.PHONY: all run test clean

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

clean:
	-$(RM) $(BIN) $(TEST_BIN)
