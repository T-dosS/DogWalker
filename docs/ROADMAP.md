# Roadmap

Backlog priorizado do projeto. Cada ciclo move os itens concluídos para o
[CHANGELOG](../CHANGELOG.md) e registra novos achados.

Legenda: 🟢 pode ser feito · 🟡 aguardando decisão do mantenedor · 🔴 bloqueado

## Próximos passos

| # | Item | Tipo | Status | Observação |
|---|------|------|--------|------------|
| 1 | Integração contínua (GitHub Actions) em Linux e Windows | Qualidade | 🔴 | Workflow pronto em `docs/ci/ci.yml`. Falta o escopo `workflow` no token: `gh auth refresh -h github.com -s workflow`, depois copiar o arquivo para `.github/workflows/` |
| 2 | Cadastro de cães por cliente (nome, raça, porte, observações) | Funcionalidade | 🟡 | Muda o modelo de dados (relação 1:N); decidir entre um segundo CSV ou migrar para SQLite (ver `AVALIACAO_TECNOLOGIAS.md`) |
| 3 | Telefone/WhatsApp do tutor | Funcionalidade | 🟡 | Muda o formato do CSV; precisa de migração dos dados existentes (coluna nova com valor vazio) |
| 4 | Senha com *hash* e arquivo de usuários | Segurança | 🟡 | Decidir se haverá cadastro de usuários e perfis; sem dependência externa seria SHA-256 + sal implementado em C |
| 5 | Agenda de passeios (data, horário, passeador, cão) | Funcionalidade | 🟡 | Depende do item 2 |
| 6 | Completar `tests/cenarios.sh` com todos os cenários da versão PowerShell | Qualidade | 🟢 | Hoje cobre os fluxos principais (27 de 56) |

## Concluídos na 2.0

Paginação, ficha do cliente, relatório HTML, planilha protegida contra fórmulas (resolveu o item
"Proteção contra fórmulas" com a opção de arquivo separado, sem mudar o que o cadastro aceita),
ajuda, carga linear do CSV, análise estática, política de segurança, cenários em shell POSIX,
memória de filtro/ordem na sessão e desfazer a última remoção. Detalhes no CHANGELOG.

## Limitações conhecidas

- O caminho de teclado do Linux/macOS (`termios`, sequência `ESC [ A/B`) ainda não foi executado:
  não há ambiente Linux sem a CI (item 1).
- `tests/cenarios.sh` (27 cenários) cobre os fluxos principais, mas ainda não todos os 56 da versão
  PowerShell (ex.: ordenação por cidade em detalhe, ajuda na listagem, texto longo).
- Busca por CPF nas telas de alteração/remoção é linear (O(n)); com a carga linear, isso é
  imperceptível até dezenas de milhares de clientes.
- O filtro e a ordenação sem acentos cobrem as letras do Latin-1 (todas as do português);
  letras de outros alfabetos são comparadas como estão.
- Arquivos exportados são sobrescritos a cada exportação.
