# Decisões técnicas

## 1. Estrutura de módulos

```
main.c ─► login.c ─┐
   │               ├─► ui.c ─► terminal.c
   └────► menu.c ──┘
              │
              ├─► cliente.c ─► validacao.c
              ├─► arquivo.c
              └─► exportar.c (relatório HTML e planilha)
```

| Módulo | Responsabilidade | Por que separado |
|--------|------------------|------------------|
| `main.c` | Configura o terminal, exige login e chama o menu | Fica com ~20 linhas, e o fluxo do programa é lido em segundos |
| `login.c` | Autenticação | Regra de acesso isolada; trocar por hash/arquivo de usuários não afeta o resto |
| `menu.c` | Telas (cadastrar, alterar, listar com filtro, remover) | Decide **o que** mostrar; não contém códigos de cor |
| `cliente.c` | `struct Cliente` e a lista dinâmica | Não usa teclado nem arquivo, então pode ser testado sozinho |
| `validacao.c` | Regras de cada campo (CPF, e-mail, UF, CEP, texto) | Regras de negócio puras, 100% testáveis |
| `arquivo.c` | Ler/gravar CSV | Trocar o formato (ex.: binário/SQLite) só altera este arquivo |
| `exportar.c` | Relatório HTML5/CSS3 e planilha CSV | Funções puras de escape e formatação, testáveis sem a tela; o arquivo de dados nunca é tocado |
| `terminal.c` | Leitura segura do teclado (linhas, senha, setas/Esc) | Concentra o `#ifdef _WIN32` do teclado e substitui `scanf`/`fflush(stdin)` |
| `ui.c` | Cores ANSI, bordas Unicode, mensagens, tabelas e menu | Toda a aparência em um ponto; modo simples automático. Veja [INTERFACE.md](INTERFACE.md) |

**Por que não menos arquivos?** Com um único `.c` não seria possível testar
as regras sem o menu interativo. **Por que não mais?** Separar cada tela em um
arquivo ou criar camadas "repository/service" seria exagero para um projeto
deste tamanho.

## 2. Armazenamento: CSV com `;`

| Opção | Prós | Contras |
|-------|------|---------|
| TXT livre (original) | Simples de escrever | Impossível ler de volta com confiança: não dá para alterar nem remover |
| **CSV `;`** ✅ | Legível, abre no Excel/LibreOffice, fácil de depurar, diff no Git, portátil entre sistemas | Precisa proibir `;` e quebras de linha nos campos |
| Binário (`fwrite` da struct) | Leitura/gravação triviais, registros de tamanho fixo | Ilegível, depende do compilador/arquitetura (padding, endianness), um byte corrompido estraga tudo |
| SQLite | Consultas, transações | Dependência externa e complexidade desnecessária |

O separador é `;` (e não `,`) porque endereços brasileiros usam vírgula com frequência
e o Excel em português usa `;` como padrão. Em vez de implementar aspas/escape
do CSV completo (RFC 4180), o sistema **proíbe `;`** nos campos: é uma regra simples,
validada na digitação e na leitura.

CPF e CEP são gravados só com dígitos (`52998224725`), e a máscara é aplicada apenas na tela.
Isso garante que "529.982.247-25" e "52998224725" sejam reconhecidos como o mesmo cliente.

## 3. Estratégia de persistência: "carregar → operar → regravar"

Toda operação segue o mesmo ciclo (`executar_operacao` em `menu.c`):

1. `arquivo_carregar()` lê o CSV para uma `ListaClientes` (vetor dinâmico com `realloc`);
2. a operação trabalha na lista em memória;
3. se algo mudou, `arquivo_salvar()` regrava **o arquivo inteiro**;
4. `lista_liberar()` devolve a memória.

**Por que regravar tudo?** Em arquivo texto, os registros têm tamanhos diferentes:
não é possível "editar a linha 3 no lugar" se o novo nome for maior.
Regravar é simples, correto e rápido para milhares de registros.

**Carga em tempo linear (2.0):** na leitura, cada CPF precisa ser comparado aos já lidos
para descartar duplicados. Com busca linear isso custava O(n²): 50 mil clientes levavam 3,1 s,
e toda tela carrega o arquivo. Agora uma **tabela hash de endereçamento aberto** guarda os CPFs
já vistos. Ela é simples de explicar: o CPF de 11 dígitos cabe num inteiro de 64 bits e é a
própria chave, há sondagem linear e a tabela dobra de tamanho com 70% de ocupação. Cada
verificação custa O(1) em média, e 50 mil clientes carregam em 0,04 s. Uma estrutura feita à mão,
em vez de biblioteca, mantém o projeto sem dependências (medição: `make bench`).

**Gravação segura:** os dados são escritos em `clientes.csv.tmp`. Só se *todas* as
escritas e o `fclose` tiverem sucesso o `.tmp` substitui o original
(`rename` no Linux, `MoveFileExA(..., MOVEFILE_REPLACE_EXISTING)` no Windows, onde
`rename` falha se o destino existir). Se faltar espaço em disco no meio da gravação,
o arquivo original continua intacto.

## 4. Como funciona cada operação

### Cadastro
1. Lê o CPF primeiro, valida os dígitos verificadores e **recusa CPF já cadastrado**
   (assim o usuário não digita 8 campos para descobrir no final que o CPF é duplicado).
2. Lê os demais campos com `ler_campo()`, que repete a pergunta até o valor ser válido.
3. Mostra um resumo e pede confirmação.
4. `lista_adicionar()` + `arquivo_salvar()`.

### Busca por CPF
`normalizar_cpf()` remove a máscara e valida → `lista_buscar_cpf()` percorre a
lista comparando com `strcmp` (busca linear, O(n)). Para uma busca por tela, isso é
instantâneo mesmo com dezenas de milhares de clientes. O gargalo real era a carga do arquivo,
resolvida com o índice hash descrito na seção 3.

### Exportação (2.0)
A listagem monta uma única **seleção** (filtro + ordem) que alimenta a tabela, a paginação e as
exportações, então o que se exporta é exatamente o que se vê. O relatório HTML não tem JavaScript
nem recursos externos: tudo é escapado e o CSS vai embutido. A planilha é um arquivo **separado**
com proteção contra fórmulas. Essa escolha resolveu o risco de injeção de fórmulas sem mudar o que
o cadastro aceita e sem descartar dados já gravados.

### Alteração
1. `localizar_cliente()`: lê o CPF, busca e exibe os dados atuais.
2. Copia o cliente para uma variável `editado` (**a lista não é tocada ainda**).
3. Cada campo mostra o valor atual entre colchetes: ENTER mantém o valor, e um valor
   inválido é recusado sem apagar o atual (contrato das funções `normalizar_*`).
4. Se nada mudou, avisa e não grava. Senão, exibe os novos dados e pede confirmação.
5. Só então `lista->itens[indice] = editado` e o arquivo é regravado.
   Os outros clientes permanecem byte a byte iguais.

O **CPF não é editável** porque é a chave do cliente. Para corrigir um CPF, remove-se e
cadastra-se novamente (regra comum em sistemas reais).

### Remoção
1. `localizar_cliente()` (o mesmo da alteração, sem duplicar código).
2. Pede confirmação explícita.
3. `lista_remover()` usa `memmove` para puxar os clientes seguintes uma posição, mantendo a ordem.
4. Regrava o arquivo e informa sucesso ou erro.

## 5. Tratamento de erros

| Situação | Comportamento |
|----------|---------------|
| Arquivo não existe | Tratado como lista vazia (primeiro uso) |
| Sem permissão de leitura | Mensagem de erro e volta ao menu |
| Pasta `data/` inexistente ou sem permissão de escrita | Mensagem clara, e nada é perdido |
| Linha do CSV inválida/duplicada/gigante | Ignorada com aviso; o restante carrega normalmente |
| Falta de memória (`realloc` = NULL) | Operação abortada sem vazar memória (o ponteiro antigo é preservado) |
| Entrada maior que o campo | Rejeitada; o excesso é descartado para não contaminar a próxima pergunta |
| Fim da entrada (Ctrl+Z/Ctrl+D, pipe) | Programa encerra de forma limpa, sem laço infinito |
| Opção de menu inválida (`abc`, `3x`, `99`) | `converter_inteiro()` com `strtol` rejeita e o menu reaparece |

Os erros de arquivo usam o `enum ResultadoArquivo`, que é mais expressivo que retornar
`-1`/`0` e permite mensagens específicas via `arquivo_descrever_erro()`.

## 6. Outras escolhas

* **Sem variáveis globais mutáveis:** tudo é passado por parâmetro, então cada função deixa claro o que lê e o que altera.
* **Ponteiros de função** (`Normalizador`, `Operacao`): evitam repetir o mesmo laço de leitura 8 vezes e o mesmo "carregar/liberar" em 4 telas.
* **`unsigned char` antes de `isdigit`/`toupper`:** evita comportamento indefinido com bytes de acentos UTF-8.
* **Sem limpar a tela:** `system("cls")` foi removido por portabilidade e segurança; a saída também pode ser redirecionada para arquivo, o que facilita testes.
* **Login com `getenv`:** permite trocar a senha padrão sem recompilar. Hash de senha fica como melhoria futura, para não depender de biblioteca de criptografia.
