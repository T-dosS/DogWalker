# Roadmap

Backlog priorizado do projeto. Cada ciclo de manutenção escolhe itens daqui,
move os concluídos para o [CHANGELOG](../CHANGELOG.md) e registra novos achados.

Legenda: 🟢 pode ser feito · 🟡 aguardando decisão do mantenedor · 🔴 bloqueado

## Próximos passos

| # | Item | Tipo | Status | Observação |
|---|------|------|--------|------------|
| 1 | Ordenar a listagem por nome ou cidade (tecla na tela da lista) | Melhoria | 🟢 | Só ordena a exibição; o CSV não muda |
| 2 | Paginação da tabela quando houver muitos clientes | Melhoria | 🟢 | Ex.: 15 por página, `n`/`p` para navegar |
| 3 | Filtro ignorando acentos ("sao" encontra "São") | Melhoria | 🟢 | Hoje acentos precisam coincidir |
| 4 | Tela de detalhes: escolher o `#` na tabela e ver a ficha completa | Melhoria | 🟢 | Reaproveita `cliente_exibir` |
| 5 | Exportar relatório HTML estático (`data/relatorio.html`) | Funcionalidade | 🟢 | Alternativa D de `docs/INTERFACE.md` |
| 6 | Integração contínua (GitHub Actions) em Linux e Windows | Qualidade | 🔴 | O token do `gh` desta máquina não tem o escopo `workflow`; requer `gh auth refresh -s workflow` |
| 7 | Cadastro de cães por cliente (nome, raça, porte) | Funcionalidade | 🟡 | Muda o modelo de dados (novo arquivo CSV); decidir formato |
| 8 | Telefone do cliente | Funcionalidade | 🟡 | Muda o formato do CSV; precisa de migração dos dados existentes |
| 9 | Senha com hash (ex.: SHA-256 implementado em C) e arquivo de usuários | Segurança | 🟡 | Decidir se haverá cadastro de usuários |

## Limitações conhecidas

- O caminho de teclado do Linux/macOS (`termios`, sequência `ESC [ A/B`) ainda não foi executado:
  não há ambiente Linux disponível sem a CI (item 6).
- `tests/cenarios.ps1` roda só no Windows (PowerShell).
- Busca linear por CPF (O(n)); suficiente para o volume esperado.
