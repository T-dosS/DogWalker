# Interface: alternativas avaliadas e decisão

Objetivo: deixar a interface mais moderna, organizada e interativa **sem** comprometer o
funcionamento, a compatibilidade (Windows/Linux/macOS), os testes automatizados nem a
proposta do projeto (um portfólio de **linguagem C**) e sem adicionar complexidade desnecessária.

## 1. Ponto de partida

| Aspecto | Situação antes |
|---------|----------------|
| Arquitetura | `menu.c` misturava navegação e telas; o único recurso visual era um título entre linhas `=====` |
| Entrada | Tudo por linha (`fgets`). A leitura tecla a tecla já existia só para a senha (`*`) |
| Ambiente | Windows 11 e terminais modernos (Windows Terminal, VS Code, Linux, macOS) entendem sequências ANSI e Unicode |
| Restrição | 36 cenários e os testes automatizados rodam com entrada/saída **redirecionadas** (sem terminal real) |

## 2. Alternativas

| # | Alternativa | Vantagens | Limitações | Esforço | Veredito |
|---|-------------|-----------|------------|---------|----------|
| A | **TUI própria em C com ANSI + Unicode** | Sem dependências; mesma linguagem; cores, bordas, setas, atalhos; fácil de explicar; isolada em um módulo | Cada recurso é implementado à mão; teclas especiais diferem entre Windows e Linux | Baixo/médio (~1 dia) | ✅ **Escolhida** |
| B | ncurses / PDCurses | Janelas, campos, redimensionamento prontos | Dependência externa; no Windows exige instalar PDCurses; muda o build e a forma de testar | Médio (+ configuração) | ❌ Complexidade e dependência sem ganho proporcional |
| C | Interface web (HTML/CSS/JS) com servidor HTTP em C | Visual rico, mouse, responsivo | Exige sockets (Winsock × POSIX), HTTP, JSON, segurança (CSRF, XSS); dobra o tamanho do projeto e desvia o foco do C | Alto (4–7 dias) | ❌ Desproporcional para um CRUD de portfólio |
| D | Relatório HTML estático gerado pelo programa | HTML/CSS úteis sem servidor; bonito para imprimir/compartilhar | Só leitura (não substitui o menu) | Baixo (2–4 h) | ⏭️ Boa **melhoria futura**, complementar |
| E | GUI nativa (Win32, GTK, raylib) | Janelas e botões de verdade | Código específico por plataforma ou biblioteca grande; difícil de testar | Alto | ❌ Quebra a portabilidade simples |
| F | Front-end em outra linguagem (Python/Textual, Electron) chamando o C | Muito visual | Deixa de ser um projeto em C; integração entre processos | Alto | ❌ Contraria a proposta |

## 3. Por que a alternativa A

* **Compatível:** códigos ANSI funcionam nativamente no Windows 10+ (ativados com
  `ENABLE_VIRTUAL_TERMINAL_PROCESSING`), no Windows Terminal, no terminal do VS Code, no Linux e no macOS.
* **Sustentável:** toda a aparência vive em **um módulo** (`ui.c`). As regras de negócio
  (`cliente.c`, `validacao.c`, `arquivo.c`) não mudaram de comportamento.
* **Degrada com segurança:** sem terminal compatível, ou com saída redirecionada, `NO_COLOR` ou
  `DOGWALKER_SIMPLES`, o programa entra no **modo simples** (texto puro, bordas ASCII, menu por linha).
  Por isso os testes e scripts continuam funcionando.
* **Didática:** mostra C de verdade, como funções variádicas, UTF-8, `termios`, API de console
  do Windows e ponteiros de função, sem esconder nada atrás de uma biblioteca.

## 4. O que foi implementado

| Recurso | Detalhe | Onde |
|---------|---------|------|
| Cabeçalho | Caixa com bordas arredondadas, marca e título da tela | `ui_cabecalho` |
| Menu navegável | `↑`/`↓` (circular), `Enter`, números `1–5`, letras `C A L R S`, `Esc` para sair | `ui_menu`, `ler_tecla_navegacao` |
| Item destacado | Seletor `❯` e fundo colorido, com a descrição da opção embaixo | `desenhar_menu` |
| Barra de status | Usuário logado, quantidade de clientes e arquivo de dados | `montar_status` |
| Mensagens | `✔` sucesso (verde), `✖` erro (vermelho), `⚠` aviso (amarelo), `●` informação (ciano) | `ui_sucesso`, `ui_erro`... |
| Tabela | Colunas alinhadas mesmo com acentos (conta colunas, não bytes) e truncamento com `…` | `ui_tabela_*`, `ui_truncar` |
| Filtro | Busca na listagem por nome, cidade, UF, e-mail ou CPF (com ou sem máscara) | `cliente_corresponde` |
| Formulários | Rótulos coloridos e valor atual entre colchetes na alteração | `ler_campo` |
| Limpeza de tela | `ESC[H ESC[2J` no lugar do `system("cls")` original | `ui_limpar_tela` |
| Restauração | Cor e cursor são restaurados ao sair (`atexit`) | `restaurar_terminal` |

## 5. Arquitetura

```
main.c ─► login.c ─┐
   │               ├─► ui.c ─► terminal.c   (teclado: linhas, senha, setas/Esc)
   └────► menu.c ──┘     ▲
              │          │ (cores dos rótulos)
              ├─► cliente.c ─► validacao.c
              └─► arquivo.c
```

* `terminal.c`: **entrada** (e o que depende do sistema operacional no teclado).
* `ui.c`: **saída visual** e interações simples (menu, confirmação, pausa).
* `menu.c`: decide **o que** mostrar; não contém nenhum código ANSI.

## 6. Paleta

As cores são definidas em um único vetor (`CODIGOS_COR` em `ui.c`). Trocar a identidade visual
é alterar essa tabela. O item selecionado usa preto sobre ciano claro **sem negrito**: no console
do Windows, "negrito + preto" vira cinza e perde contraste (detectado nas capturas de teste).

## 7. Como foi validado

* Compilação sem warnings com `-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -Wsign-conversion -fanalyzer`.
* 1273 verificações automatizadas, incluindo filtro sem acentos, ordenação e truncamento UTF-8.
* 36 cenários de uso com entrada redirecionada, no modo simples.
* Console real do Windows: teclas simuladas (setas, Esc, atalhos) e telas capturadas **com cores**.
  As imagens em `docs/img/` são dessas capturas.
* O código específico do Linux/macOS (sequência `ESC [ A/B`) segue o padrão POSIX `termios`, mas
  não pôde ser executado neste ambiente Windows. A validação ideal é uma pipeline de CI em Linux.
