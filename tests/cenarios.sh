#!/bin/sh
# ---------------------------------------------------------------------------
# Testes de cenário (integração) do DogWalker - Linux, macOS e Git Bash
#
# Versão POSIX dos cenários de tests/cenarios.ps1: executa o programa
# compilado com entradas de teclado redirecionadas de arquivo e confere as
# mensagens e o conteúdo dos arquivos gerados. Como a entrada é
# redirecionada, a interface roda no modo simples.
#
# Uso (na pasta do projeto, depois de compilar):
#   sh tests/cenarios.sh            ou   make cenarios   (Linux/macOS)
#   sh tests/cenarios.sh ./dogwalker.exe                 (Git Bash no Windows)
#
# Retorna 0 se todos os cenários passarem e 1 caso contrário.
# ---------------------------------------------------------------------------

EXE=${1:-./dogwalker}
if [ ! -x "$EXE" ]; then
    echo "Executável não encontrado: $EXE (compile com make)"
    exit 1
fi

TMP=$(mktemp -d 2>/dev/null || mktemp -d -t dogwalker)
DADOS="$TMP/clientes.csv"
ENTRADA="$TMP/entrada.txt"
SAIDA="$TMP/saida.txt"
EXEMPLO="data/clientes_exemplo.csv"
TOTAL=0
FALHAS=0
trap 'rm -rf "$TMP"' EXIT

# Credenciais padrão, mesmo que o ambiente tenha outras.
unset DOGWALKER_USUARIO DOGWALKER_SENHA

# entrada linha1 linha2 ... : grava as linhas digitadas (uma por argumento).
entrada() {
    : > "$ENTRADA"
    for linha in "$@"; do
        printf '%s\n' "$linha" >> "$ENTRADA"
    done
}

# executar [arquivo_de_dados]: roda o programa com a entrada gravada.
executar() {
    "$EXE" "${1:-$DADOS}" < "$ENTRADA" > "$SAIDA" 2>&1
}

# contem "texto"...: todos os textos aparecem na última saída.
contem() {
    for esperado in "$@"; do
        if ! grep -qF -- "$esperado" "$SAIDA"; then
            echo "    faltou: '$esperado'"
            return 1
        fi
    done
    return 0
}

# nao_contem "texto": o texto NÃO aparece na última saída.
nao_contem() {
    if grep -qF -- "$1" "$SAIDA"; then
        echo "    não deveria ter: '$1'"
        return 1
    fi
    return 0
}

# resultado "nome" status: contabiliza e imprime o resultado de um cenário.
resultado() {
    TOTAL=$((TOTAL + 1))
    if [ "$2" -eq 0 ]; then
        echo "[ OK ] $1"
    else
        FALHAS=$((FALHAS + 1))
        echo "[FALHA] $1"
    fi
}

# arquivo_contem arquivo "texto"...: os textos aparecem no arquivo.
arquivo_contem() {
    arq=$1
    shift
    [ -f "$arq" ] || { echo "    arquivo não gerado: $arq"; return 1; }
    for esperado in "$@"; do
        if ! grep -qF -- "$esperado" "$arq"; then
            echo "    faltou em $(basename "$arq"): '$esperado'"
            return 1
        fi
    done
    return 0
}

rm -f "$DADOS"

# --- Login e menu ----------------------------------------------------------
entrada admin admin 5; executar
contem "Login efetuado com sucesso" "Fim da execução"; resultado "Login correto" $?

entrada admin x root admin a b; executar
{ contem "Tentativas restantes: 2" "Tentativas restantes: 1" "máximo de tentativas" "Acesso negado" \
  && nao_contem "Login efetuado"; }; resultado "Login incorreto 3x" $?

entrada admin admin 3 "" 5; executar
contem "Nenhum cliente cadastrado até o momento"; resultado "Arquivo inexistente: listagem vazia" $?

entrada admin admin abc 9 1abc "" -1 5; executar
contem "Opção inválida" "Fim da execução"; resultado "Menu: entradas inesperadas" $?

entrada admin admin "?" "" 5; executar
contem "AJUDA" "/+ /-" "#3" "Pressione ENTER para voltar ao menu"; resultado "Ajuda pelo menu" $?

# --- Cadastro --------------------------------------------------------------
entrada admin admin 1 \
    abc 123.456.789-00 529.982.247-25 \
    "" "Ana;Souza" "José D'Ávila & Cia." \
    ana@gmail Ana@Example.COM \
    "Rua das Flores" 120 Centro Campinas \
    "São Paulo" sp \
    1234 13010-000 \
    talvez s "" 5
executar
contem "CPF inválido" "Campo obrigatório" "E-mail inválido" "Estado inválido" "CEP inválido" \
       "Responda com" "Cliente cadastrado com sucesso" "529.982.247-25" "ana@example.com"
resultado "Cadastro com validações" $?

entrada admin admin 1 52998224725 "" "" 5; executar
contem "Já existe um cliente cadastrado com este CPF" "Cadastro cancelado"
resultado "Cadastro: CPF duplicado e cancelar" $?

entrada admin admin \
    1 11144477735 "Bruno Lima" bruno@example.com "Avenida Brasil" 1500 Jardim "Rio de Janeiro" RJ 20040002 s "" \
    1 12345678909 "Cláudia Conceição" claudia@example.com "Rua São João" S/N Savassi "Belo Horizonte" mg 30140071 s "" \
    3 "" 5
executar
contem "Total: 3 cliente(s)" "Belo Horizonte/MG"; resultado "Cadastro de mais 2 clientes" $?

# --- Listagem --------------------------------------------------------------
entrada admin admin 3 bruno "" 5; executar
contem 'Filtro: "bruno"' "Exibindo 1 de 3" "| 2   | Bruno Lima"; resultado "Filtro por nome" $?

entrada admin admin 3 conceicao "" 5; executar
contem "Exibindo 1 de 3" "| 3   | Cláudia Conceição"; resultado "Filtro sem acentos" $?

entrada admin admin 3 /n "" 5; executar
contem "Ordenado por nome"; resultado "Ordenação por nome" $?

entrada admin admin 3 "#2" "" "" 5; executar
contem "FICHA DO CLIENTE" "Cliente nº 2 de 3" "Bruno Lima" "111.444.777-35"; resultado "Ficha do cliente (#2)" $?

entrada admin admin 3 "#99" "" 5; executar
contem "Número de cliente inválido"; resultado "Ficha: número inválido" $?

# --- Exportação ------------------------------------------------------------
entrada admin admin 3 /e "" 5; executar
{ contem "Relatório gerado com 3 cliente(s)" \
  && arquivo_contem "$TMP/relatorio.html" "<!DOCTYPE html>" "José D&#39;Ávila &amp; Cia." "***.982.247-**" \
  && ! grep -qF "529.982.247-25" "$TMP/relatorio.html"; }
resultado "Relatório HTML: escape e CPF mascarado" $?

entrada admin admin 3 /n /x "" 5; executar
{ contem "Planilha gerada com 3 cliente(s)" \
  && arquivo_contem "$TMP/exportacao.csv" "Nome;CPF;" "Bruno Lima;111.444.777-35;"; }
resultado "Planilha CSV" $?

# --- Alteração e remoção (confere o arquivo) -------------------------------
cp "$DADOS" "$TMP/antes.csv"
entrada admin admin 2 111.444.777-35 "" bruno.novo@example.com "" "" "" "" "" "" s "" 5; executar
{ contem "Dados alterados com sucesso" \
  && [ "$(diff "$TMP/antes.csv" "$DADOS" | grep -c '^[<>]')" -eq 2 ] \
  && grep -qF "bruno.novo@example.com" "$DADOS"; }
resultado "Alteração muda só o cliente escolhido" $?

entrada admin admin 2 98765432100 "" 5; executar
contem "Nenhum cliente encontrado com o CPF 987.654.321-00"; resultado "Alteração: cliente inexistente" $?

cp "$DADOS" "$TMP/antes.csv"
entrada admin admin 4 529.982.247-25 s "" 5; executar
{ contem "A remoção é definitiva" "Cliente removido com sucesso" \
  && ! grep -qF "52998224725" "$DADOS" \
  && [ "$(wc -l < "$DADOS")" -eq 3 ]; }
resultado "Remoção apaga só o cliente escolhido" $?

# --- Arquivo e ambiente ----------------------------------------------------
: > "$DADOS"
entrada admin admin 3 "" 5; executar
contem "Nenhum cliente cadastrado até o momento"; resultado "Arquivo vazio" $?

cp "$EXEMPLO" "$DADOS"; printf 'lixo;lixo\n' >> "$DADOS"
entrada admin admin 3 "" 5; executar
contem "1 linha(s) inválida(s)" "Total: 3 cliente(s)"; resultado "Arquivo com linha corrompida" $?

entrada admin admin 1 52998224725 Ana a@example.com R 1 B C SP 13010000 s "" 5
executar "$TMP/pasta_inexistente/c.csv"
{ contem "não foi possível gravar" && nao_contem "cadastrado com sucesso"; }; resultado "Pasta inexistente" $?

entrada admin admin 1 52998224725 Ana; executar
contem "Entrada encerrada"; resultado "Fim da entrada no meio do cadastro" $?

entrada maria "s3nh@" 5
DOGWALKER_USUARIO=maria DOGWALKER_SENHA='s3nh@' "$EXE" "$DADOS" < "$ENTRADA" > "$SAIDA" 2>&1
contem "Bem-vindo(a), maria"; resultado "Credenciais por variável de ambiente" $?

# --- Paginação (20 clientes gerados) ---------------------------------------
# Gera CPFs válidos com a mesma regra dos testes unitários.
gerar_cpf() {
    base=$((100000000 + $1 * 7919))
    s=0; i=0
    for d in $(echo "$base" | sed 's/./& /g'); do s=$((s + d * (10 - i))); i=$((i + 1)); done
    d1=$(( (s * 10) % 11 % 10 ))
    s=0; i=0
    for d in $(echo "$base$d1" | sed 's/./& /g'); do s=$((s + d * (11 - i))); i=$((i + 1)); done
    d2=$(( (s * 10) % 11 % 10 ))
    echo "$base$d1$d2"
}
PAGINAS="$TMP/paginas.csv"
echo "cpf;nome;email;rua;numero;bairro;cidade;estado;cep" > "$PAGINAS"
n=1
while [ "$n" -le 20 ]; do
    echo "$(gerar_cpf "$n");Cliente $n;c$n@example.com;Rua A;$n;Centro;Campinas;SP;13010000" >> "$PAGINAS"
    n=$((n + 1))
done
entrada admin admin 3 /+ /+ /- "" 5; executar "$PAGINAS"
contem "Página 1 de 2" "Página 2 de 2" "| 20  | Cliente 20" "Esta já é a última página" "Exibindo 20 de 20"
resultado "Paginação" $?

echo
echo "$TOTAL cenários, $FALHAS falha(s)."
[ "$FALHAS" -eq 0 ]
