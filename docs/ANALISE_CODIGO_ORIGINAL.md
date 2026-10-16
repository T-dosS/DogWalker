# Análise do código original

Este documento lista os problemas encontrados no código original do trabalho
(preservado em [`codigo-original/dogwalker_original.c`](codigo-original/dogwalker_original.c))
e como cada um foi resolvido na versão atual. As explicações foram escritas
para estudantes de Sistemas de Informação.

Gravidade: 🔴 erro/bug grave · 🟠 comportamento incorreto ou risco · 🟡 qualidade/manutenção

---

## 1. Erros de compilação e warnings

| # | Problema | Gravidade | Explicação | Correção |
|---|----------|-----------|------------|----------|
| 1.1 | `return 1;` dentro de funções `void` (`Cadastro`, `Alterar`, `Listar`) | 🔴 | Uma função `void` não devolve valor. Compiladores modernos (GCC 14+) tratam isso como **erro**, e os antigos dão warning. | As operações agora retornam `void` sem valor ou `int` com significado documentado. |
| 1.2 | `sleep(1)` sem `#include <unistd.h>` (e `sleep` nem existe no Windows, onde é `Sleep` em `<windows.h>`) | 🔴 | É uma **declaração implícita de função**, proibida desde o C99. Em alguns ambientes o link falha. | Pausas artificiais removidas. O programa espera o usuário apertar ENTER. |
| 1.3 | `#include <conio.h>` | 🟠 | Cabeçalho que só existe em compiladores Windows antigos (Turbo C, MinGW). No Linux o código **não compila**. Nenhuma função dele era usada. | Removido. |
| 1.4 | `scanf("%s", &cpf)` e `scanf("%s", &Global.Cd)` | 🔴 | `&cpf` é do tipo `char (*)[12]`, e não `char *` (warning de formato). Já `&Global.Cd` aponta para **um único `char`**: `%s` grava a string inteira mais o `'\0'`, ou seja, pelo menos 2 bytes em 1 → **estouro de memória**. | Toda leitura usa `ler_linha()` com `fgets` e limite de tamanho. |
| 1.5 | Linhas `Cadastro;` e `Pessoa;` soltas no início das funções | 🟡 | São expressões sem efeito (o compilador avisa *statement with no effect*). `Cadastro;` nem é válido, porque `Cadastro` é ao mesmo tempo nome de função e de struct. | Removidas. |
| 1.6 | Protótipos `void Cadastro();` sem `void` nos parâmetros | 🟡 | Em C (antes do C23), `()` significa "parâmetros não especificados", e o compilador não verifica as chamadas. | Protótipos completos nos headers: `void menu_executar(const char *caminho_arquivo);`. |
| 1.7 | `int opcao = 0;` global e `argc/argv` nunca usados | 🟡 | Código morto. | Removido. `argv[1]` agora permite escolher o arquivo de dados. |

## 2. Bugs e problemas de lógica

| # | Problema | Gravidade | Explicação | Correção |
|---|----------|-----------|------------|----------|
| 2.1 | **`Alterar()` não altera o cliente informado** | 🔴 | O CPF é lido mas **nunca é procurado no arquivo**. O arquivo é aberto em `"rb+"`, com o cursor na posição 0, e os novos dados são escritos **por cima do início do arquivo**, corrompendo o primeiro cliente (e parte do segundo). Além disso, o nome e o CPF não são regravados. | Reescrita: carrega todos os clientes, busca pelo CPF, edita uma cópia, confirma e regrava o arquivo inteiro de forma segura. |
| 2.2 | `Alterar()` diz "CLIENTE INEXISTENTE" quando o **arquivo** não abre | 🟠 | Confunde "arquivo não abriu" com "cliente não existe". | As duas mensagens agora são distintas: erro de arquivo e "Nenhum cliente encontrado com o CPF ...". |
| 2.3 | `Remover()` vazia | 🔴 | Funcionalidade prometida no menu e não implementada. | Implementada: busca por CPF → exibe → confirma → remove só aquele cliente → grava. |
| 2.4 | **Recursão infinita para navegar** (`mainMenu()` chamando `Cadastro()`, que chama `mainMenu()`...) | 🔴 | Cada volta ao menu empilha uma nova chamada que nunca termina. Com uso prolongado a pilha cresce sem limite (*stack overflow*). Além disso, "Encerrar" só sai do último nível. | Um único laço `for (;;)` em `menu_executar()`. As telas retornam para ele. |
| 2.5 | Em `Cadastro()`, se o usuário responde algo diferente de `s`/`n`, a função chama a si mesma e **perde tudo o que foi digitado** | 🟠 | Além de recursão, obriga a redigitar o cadastro inteiro. | `confirmar()` repete apenas a pergunta s/n até uma resposta válida. |
| 2.6 | Mistura de `scanf` e `fgets` sem consumir o `'\n'` | 🔴 | Depois de `scanf("%d")` ou `scanf("%c")`, o ENTER fica no buffer. O próximo `fgets` lê esse `'\n'` como resposta vazia. Por isso surgiram os `getchar()` e `fflush(stdin)` espalhados. | Só `fgets` (via `ler_linha`), que sempre consome a linha inteira. |
| 2.7 | `fgets` mantém o `'\n'` dentro dos campos | 🟠 | O arquivo ficava com quebras de linha no meio de cada registro ("Nome: Ana\n E-mail: ..."), o que torna impossível achar um cliente por linha. | O `'\n'` é removido, e espaços das pontas também. |
| 2.8 | Entradas maiores que o buffer "vazam" para o campo seguinte | 🟠 | `fgets(Pessoa.Nome, 200, stdin)` lê no máximo 199 caracteres. O restante vira a resposta do E-mail. | `ler_linha` detecta a linha longa, descarta o restante e avisa o usuário. |
| 2.9 | Login com `scanf("%s")` em buffer de 15 | 🔴 | Digitar 15 caracteres ou mais no login **estoura o buffer** (buffer overflow clássico). Também não aceita espaços. | `ler_linha` com tamanho; senha mascarada com `*`; no máximo 3 tentativas. |
| 2.10 | `Listar()`: se o usuário responde `n`, a função simplesmente termina, e como foi chamada por `mainMenu()` → o **programa encerra** | 🟠 | Comportamento inesperado. | A listagem volta ao menu após ENTER. |
| 2.11 | Arquivo inexistente tratado como "Erro no Banco de Dados" | 🟠 | No primeiro uso o arquivo ainda não existe, então Listar e Alterar falhavam. | Arquivo inexistente = lista vazia (não é erro). |
| 2.12 | Nenhuma validação: CPF vazio, duplicado, com letras, e-mail sem `@`, UF "Xyz" | 🟠 | Dados inconsistentes e CPFs repetidos impedem identificar o cliente. | Validação completa (CPF com dígitos verificadores, e-mail, UF, CEP, campos obrigatórios) e CPF único. |

## 3. Segurança e uso inadequado de funções

| # | Problema | Explicação | Correção |
|---|----------|------------|----------|
| 3.1 | `fflush(stdin)` | O padrão C define `fflush` **apenas para streams de saída**. Em entrada o comportamento é **indefinido**: funciona no Windows antigo, não faz nada no Linux. | Removido. Usamos `fflush(stdout)` (uso correto, para mostrar a pergunta antes de ler) e descartamos o resto da linha manualmente quando necessário. |
| 3.2 | `scanf("%s")` sem limite | Porta de entrada para buffer overflow. | `fgets` com tamanho em todo o programa. |
| 3.3 | `system("cls")` | Executa um processo externo (`cmd.exe`) a cada tela. É lento, não funciona no Linux (lá seria `clear`) e é má prática de segurança (depende do `PATH`). | Removido. Telas separadas por títulos. |
| 3.4 | Senha `admin` visível na digitação e no código | Qualquer pessoa ao lado lê a senha. | A senha aparece como `*` (leitura tecla a tecla, isolada em `terminal.c`). Credenciais configuráveis por variável de ambiente. Uso de hash fica documentado como melhoria futura. |
| 3.5 | Login infinito | Permite tentativa e erro ilimitada. | Máximo de 3 tentativas. |
| 3.6 | Dados pessoais (CPF, endereço) em `Dados_Clientes.txt` na pasta do código | Risco de publicar dados reais no GitHub (LGPD). | Dados em `data/`, ignorados pelo `.gitignore`. Apenas um CSV de exemplo **fictício** é versionado. |

## 4. Arquivos

| # | Problema | Correção |
|---|----------|----------|
| 4.1 | `FILE *DB` global, reaberto e às vezes **declarado de novo** dentro de `Alterar()` (`FILE *DB;` local que esconde o global, *shadowing*) | Ponteiros `FILE *` locais, abertos e fechados na mesma função. |
| 4.2 | Arquivo aberto em `"r"` em `Alterar()` e reaberto em `"r+"` sem fechar o primeiro → **vazamento de descritor** | Cada `fopen` tem exatamente um `fclose`. |
| 4.3 | Retorno de `fprintf`/`fclose` nunca verificado: disco cheio ou falta de permissão passam despercebidos | Todos verificados. A gravação é feita em `.tmp` e só substitui o original se tudo deu certo. |
| 4.4 | Formato "livre" (`Nome: ... E-mail: ...` + linhas `-----`) que não pode ser lido de volta de forma confiável | CSV com cabeçalho, um cliente por linha. |

## 5. Portabilidade (Windows × Linux)

* `conio.h`, `system("cls")`, `sleep` sem header e `setlocale(LC_ALL, "Portuguese")`
  (nome de locale que só existe no Windows) impediam a compilação ou execução no Linux.
* A codificação dos acentos dependia da página de código do console.
* **Agora:** C11 padrão. O código específico de cada sistema fica isolado em
  `terminal.c` (UTF-8 no console e eco da senha) e `arquivo.c` (troca atômica do arquivo),
  sempre com `#ifdef _WIN32`.

## 6. Organização e nomenclatura

* **Variáveis globais desnecessárias** (`Global`, `Pessoa`, `DB`, `opcao`): tornavam
  qualquer função capaz de alterar qualquer dado, o que dificulta encontrar bugs. Agora não existe
  nenhuma variável global mutável no programa.
* `struct VarG { int Op; char Cd; }`: nome sem significado, agrupando dados sem relação.
* `struct Cadastro` e a função `Cadastro()` com o mesmo nome; `simounao`, `login1`, `Cd`: nomes pouco descritivos.
* Nomes mistos (`mainMenu` em inglês e camelCase, `Cadastro` em PascalCase): a nova
  versão usa `snake_case` em português de forma consistente.
* Tudo em um único arquivo, com interface, regra de negócio e arquivo misturados → agora são 6 módulos.
* Código duplicado: os blocos de `printf`/`fgets` de cada campo eram repetidos no cadastro e na
  alteração → agora `preencher_dados_cliente()` serve para os dois.
* Tamanhos inconsistentes: `Cpf[20]` na struct e `cpf[12]` em `Alterar`; `Estado[30]` para uma sigla de 2 letras.
