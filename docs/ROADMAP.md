# Roadmap

Backlog priorizado do projeto. Cada ciclo de manutenção escolhe itens daqui,
move os concluídos para o [CHANGELOG](../CHANGELOG.md) e registra novos achados.

Legenda: 🟢 pode ser feito · 🟡 aguardando decisão do mantenedor · 🔴 bloqueado

## Próximos passos

| # | Item | Tipo | Status | Observação |
|---|------|------|--------|------------|
| 1 | Paginação da tabela quando houver muitos clientes | Melhoria | 🟢 | Ex.: 15 por página; pode seguir o padrão dos comandos com `/` da listagem (`/+` e `/-`) e respeitar filtro e ordem |
| 2 | Tela de detalhes: escolher o `#` na tabela e ver a ficha completa | Melhoria | 🟢 | Reaproveita `cliente_exibir`; o `#` já é a posição de cadastro em qualquer ordem |
| 3 | Exportar relatório HTML estático (`data/relatorio.html`) | Funcionalidade | 🟢 | Alternativa D de `docs/INTERFACE.md` |
| 4 | Proteção contra fórmulas ao abrir o CSV no Excel | Segurança | 🟡 | Um nome como `=HIPERLINK(...)` vira fórmula no Excel. Decidir entre recusar campos que começam com `=`, `+`, `-` ou `@` (muda o que o cadastro aceita e descartaria linhas já gravadas assim) ou gerar um arquivo de exportação separado com os valores escapados |
| 5 | Integração contínua (GitHub Actions) em Linux e Windows | Qualidade | 🔴 | O token do `gh` desta máquina não tem o escopo `workflow`; requer `gh auth refresh -s workflow` |
| 6 | Cadastro de cães por cliente (nome, raça, porte) | Funcionalidade | 🟡 | Muda o modelo de dados (novo arquivo CSV); decidir formato |
| 7 | Telefone do cliente | Funcionalidade | 🟡 | Muda o formato do CSV; precisa de migração dos dados existentes |
| 8 | Senha com hash (ex.: SHA-256 implementado em C) e arquivo de usuários | Segurança | 🟡 | Decidir se haverá cadastro de usuários |

## Limitações conhecidas

- O caminho de teclado do Linux/macOS (`termios`, sequência `ESC [ A/B`) ainda não foi executado:
  não há ambiente Linux disponível sem a CI (item 5).
- `tests/cenarios.ps1` roda só no Windows (PowerShell).
- Busca linear por CPF (O(n)); suficiente para o volume esperado.
- O filtro e a ordenação sem acentos cobrem as letras do Latin-1 (todas as do português);
  letras de outros alfabetos são comparadas como estão.
- A ordem escolhida na listagem não é lembrada: cada vez que a tela é aberta, ela volta à
  ordem de cadastro.
