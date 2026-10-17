# Histórico de alterações

Todas as mudanças relevantes do projeto ficam registradas aqui.
Formato baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/),
com versionamento [SemVer](https://semver.org/lang/pt-BR/).

Categorias usadas: **Adicionado**, **Alterado**, **Corrigido**, **Segurança**, **Removido**.

## [Não publicado]

Mudanças prontas que ainda não viraram versão.

### Adicionado
- Ordenação da listagem: `/n` ordena por nome, `/c` por cidade (depois UF e nome) e `/o`
  volta à ordem de cadastro. Só a exibição muda; o CSV e a coluna `#` continuam na ordem de cadastro.
- `texto_para_comparacao`: forma minúscula e sem acentos de um texto UTF-8, usada no filtro
  e na ordenação.
- 8 cenários e 29 verificações automatizadas novas (36 cenários e 1273 verificações no total).

### Alterado
- O filtro da listagem ignora acentos: "sao" encontra "São Paulo" e "SÃO" encontra "sao".
- A listagem avisa quando o texto digitado é longo demais ou o comando de ordenação não existe,
  em vez de ignorar a entrada em silêncio.

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
