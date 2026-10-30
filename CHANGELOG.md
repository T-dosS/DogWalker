# Histórico de alterações

Todas as mudanças relevantes do projeto ficam registradas aqui.
Formato baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/),
com versionamento [SemVer](https://semver.org/lang/pt-BR/).

Categorias usadas: **Adicionado**, **Alterado**, **Corrigido**, **Segurança**, **Removido**.

## [Não publicado]

Mudanças prontas que ainda não viraram versão.

## [2.0.0] - 2026-10-30

Upgrade de desempenho, funcionalidades, qualidade e segurança. Auditoria completa em
[`docs/AUDITORIA.md`](docs/AUDITORIA.md) e avaliação de tecnologias em
[`docs/AVALIACAO_TECNOLOGIAS.md`](docs/AVALIACAO_TECNOLOGIAS.md).

### Adicionado
- **Paginação da listagem:** 15 clientes por página; `/+` e `/-` navegam; filtro e ordem voltam à página 1.
- **Ficha do cliente:** `#N` na listagem abre os dados completos do cliente nº N.
- **Relatório HTML5/CSS3** (`/e`): exporta exatamente o que está na tela (filtro e ordem);
  responsivo (cards no celular), tema claro/escuro automático, pronto para impressão,
  sem JavaScript e sem recursos externos.
- **Planilha CSV** (`/x`): compatível com Excel (BOM UTF-8, CRLF), CPF e CEP formatados como texto.
- **Ajuda** (`?`) no menu e na listagem, com todos os atalhos.
- **Ordenação da listagem:** `/n` por nome, `/c` por cidade (depois UF e nome), `/o` volta à
  ordem de cadastro. Só a exibição muda; o CSV e a coluna `#` continuam na ordem de cadastro.
- Componente de **cartão** (`ui_painel_*`) com quebra de linha por palavra, usado na ficha do cliente.
- `make lint` (cppcheck), `make strict` (avisos extras com `-Werror`), `make bench` e `make check`.
- `tests/benchmark.c`: mede gravação e carga de 1.000, 10.000 e 50.000 clientes.
- `SECURITY.md` com política de relato, modelo de ameaça e riscos aceitos.
- Workflow do GitHub Actions pronto em `docs/ci/ci.yml` (Linux com sanitizers e Windows).
- `docs/AUDITORIA.md` e `docs/AVALIACAO_TECNOLOGIAS.md`; novas capturas de tela no README.
- Testes: 1337 verificações (antes 1244) e 49 cenários ponta a ponta (antes 28).
- `tests/cenarios.sh`: 24 cenários ponta a ponta em shell POSIX, para Linux, macOS, Git Bash e a CI
  (`make cenarios` fora do Windows).

### Alterado
- **Desempenho:** a carga do CSV passou de quadrática para linear (índice de CPFs em tabela hash).
  50.000 clientes: 3,096 s → 0,042 s; 10.000: 0,081 s → 0,009 s.
- O filtro da listagem ignora acentos: "sao" encontra "São Paulo" e "SÃO" encontra "sao".
- A listagem avisa quando o texto é longo demais ou o comando não existe, em vez de ignorar a entrada.
- A ficha do cliente (confirmação de cadastro, alteração, remoção) é exibida em um cartão com borda.
- A remoção mostra um aviso destacado de que a operação é definitiva.
- README reescrito com instalação, configuração, uso, estrutura, testes e qualidade.

### Corrigido
- Achados da análise estática: escopo de variáveis, campo sem uso em `Icones`, verificação de
  faixa de `converter_inteiro` documentada para plataformas em que `long` tem 64 bits.

### Segurança
- Relatório HTML com todo texto escapado (`& < > " '`), impedindo injeção de HTML a partir dos dados.
- Planilha protegida contra injeção de fórmulas (`=`, `+`, `-`, `@`, tab, CR recebem apóstrofo).
- CPF mascarado no relatório HTML (`***.982.247-**`) para proteger dados pessoais (LGPD).

## [1.2.0] - 2026-10-16

### Adicionado
- Interface de terminal com cores ANSI, bordas Unicode e ícones (módulo `ui`), sem bibliotecas externas.
- Menu navegável com setas, Enter, atalhos `1`–`5` e `C`/`A`/`L`/`R`/`S`, e `Esc` para sair.
- Barra de status com usuário logado, total de clientes e arquivo de dados.
- Listagem em tabela com filtro por nome, cidade, UF, e-mail ou CPF (com ou sem máscara).
- Modo simples automático (texto puro) quando a saída é redirecionada, o terminal é antigo
  ou `NO_COLOR`/`DOGWALKER_SIMPLES` estão definidos.
- Testes de cenário ponta a ponta no repositório (`tests/cenarios.ps1`, `make cenarios`).
- `docs/INTERFACE.md` com as alternativas avaliadas, e capturas de tela no README.

### Alterado
- `terminal.c` agora lê teclas de navegação (setas/Esc) no Windows e no Linux/macOS.
- Mensagens padronizadas: sucesso, erro, aviso e informação, cada uma com ícone e cor.

## [1.1.0] - 2026-10-15

### Adicionado
- Senha mascarada com `*` durante a digitação, com suporte a Backspace.
- Configuração do VS Code (compilar, executar, testar e depurar).

### Corrigido
- Gravação falhava de forma intermitente no Windows ("acesso negado") quando o antivírus
  abria o arquivo recém-salvo; agora a troca do arquivo é repetida em erros transitórios.

## [1.0.0] - 2026-10-15

Refatoração completa do trabalho acadêmico original.

### Adicionado
- Função de remoção (antes vazia) com busca por CPF e confirmação.
- Validação de CPF (dígitos verificadores), e-mail, UF, CEP e tamanho dos campos; CPF único.
- Persistência em CSV com gravação segura via arquivo temporário.
- Testes automatizados, Makefile, `.gitignore` protegendo dados pessoais e documentação.

### Corrigido
- Alteração sobrescrevia o início do arquivo em vez do cliente informado.
- Recursão infinita entre menu e telas.
- Estouros de buffer com `scanf("%s")` e comportamento indefinido com `fflush(stdin)`.

### Segurança
- Login limitado a 3 tentativas, com mensagem genérica de erro.
