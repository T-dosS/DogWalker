# Auditoria técnica: versão 1.2 → 2.0

Auditoria completa feita antes do upgrade 2.0, com a classificação usada para
priorizar o trabalho e o status de cada item.

## 1. Retrato do projeto auditado (v1.2 + PR #1)

| Aspecto | Situação encontrada |
|---------|---------------------|
| Linguagem / padrão | C11, biblioteca padrão; APIs de sistema isoladas por `#ifdef _WIN32` |
| Frameworks e dependências | Nenhum. Sem bibliotecas externas (ponto forte: nada a atualizar ou auditar) |
| Arquitetura | 7 módulos: `main`, `menu` (telas), `cliente` (modelo/lista), `validacao`, `arquivo` (CSV), `terminal` (teclado), `ui` (visual) |
| Persistência | CSV com `;`, gravação atômica via `.tmp`; sem banco de dados |
| Integrações / APIs | Nenhuma (aplicação local de linha de comando) |
| Autenticação | Usuário/senha fixos (`admin`/`admin`) ou por variáveis de ambiente; 3 tentativas |
| Testes | 1273 verificações unitárias + 36 cenários ponta a ponta, todos passando |
| Build | Makefile (`make`, `run`, `test`, `cenarios`, `clean`) e tarefas do VS Code |
| Qualidade estática | Compilação sem avisos com `-Wall -Wextra -Wpedantic -Wconversion -fanalyzer`; cppcheck nunca rodado |
| Repositório | README, CHANGELOG, ROADMAP e documentação técnica; sem CI; PR #1 aberto |

**Linha de base medida** (Windows, gcc 16.2, `-O2`):

| Medida | Valor |
|--------|-------|
| Executável | 103.790 bytes |
| Carregar 1.000 / 10.000 / 50.000 clientes | 0,002 s / 0,081 s / **3,096 s** |
| Gravar 1.000 / 10.000 / 50.000 clientes | 0,002 s / 0,007 s / 0,031 s |
| cppcheck (warning, style, performance, portability) | 7 achados |

## 2. Problemas e oportunidades encontrados

Prioridade: 🔴 alta · 🟠 média · 🟢 baixa. Impacto e complexidade: alto/médio/baixo.

| # | Área | Achado | Prioridade | Impacto | Complexidade | Status 2.0 |
|---|------|--------|-----------|---------|--------------|------------|
| 1 | Desempenho | Carga do CSV é **quadrática**: cada linha procura CPF duplicado percorrendo a lista inteira. 50 mil clientes = 3,1 s, e *toda* tela carrega o arquivo | 🔴 | Alto | Média | ✅ Tabela hash: 0,042 s |
| 2 | Qualidade | Análise estática nunca rodada; 7 achados de estilo (escopo, campo sem uso, falsos positivos sem documentação) | 🟠 | Médio | Baixa | ✅ Corrigidos; `make lint` |
| 3 | Automação | Não há um comando único de validação antes de publicar | 🟠 | Médio | Baixa | ✅ `make check` |
| 4 | Automação | Sem integração contínua; o caminho Linux/macOS (`termios`) nunca foi executado | 🔴 | Alto | Baixa | ⏸️ Workflow pronto em `docs/ci/`; publicar exige escopo `workflow` |
| 5 | Funcionalidade | Listagem sem paginação: com muitos clientes a tabela rola para fora da tela | 🟠 | Médio | Baixa | ✅ 15 por página |
| 6 | Funcionalidade | Não há como ver a ficha completa a partir da listagem (só alterando/removendo) | 🟠 | Médio | Baixa | ✅ `#N` |
| 7 | Funcionalidade | Não há como tirar os dados do programa (relatório, planilha) | 🟠 | Alto | Média | ✅ `/e` HTML e `/x` planilha |
| 8 | Segurança | Abrir o CSV no Excel executa fórmulas em campos iniciados por `=`, `+`, `-`, `@` | 🟠 | Médio | Baixa | ✅ Exportação de planilha protegida (o arquivo de dados não muda) |
| 9 | UX | Sem ajuda dentro do programa; atalhos só no README | 🟢 | Médio | Baixa | ✅ `?` no menu e na listagem |
| 10 | UX/Visual | Ficha do cliente como linhas soltas; endereço longo quebra a leitura | 🟢 | Baixo | Baixa | ✅ Cartão com borda e quebra por palavra |
| 11 | UX | Remoção não deixa claro que é definitiva | 🟢 | Médio | Baixa | ✅ Aviso destacado |
| 12 | Segurança | Sem política de segurança nem modelo de ameaça documentados | 🟢 | Baixo | Baixa | ✅ `SECURITY.md` |
| 13 | Desempenho | O menu relê o arquivo a cada volta só para contar clientes | 🟢 | Baixo | Média | ✅ Com o item 1 a releitura custa ~1 ms (10 mil clientes); mantida por ser sempre exata |
| 14 | Segurança | Credenciais padrão `admin`/`admin` e senha comparada em texto | 🟠 | Médio | Média | 🟡 Decisão pendente (cadastro de usuários + hash) |
| 15 | Dados | CSV sem criptografia | 🟢 | Médio | Alta | 🟡 Risco aceito, documentado |
| 16 | Domínio | Não há cadastro dos cães nem telefone do tutor | 🟠 | Alto | Média | 🟡 Muda o modelo de dados: decisão pendente |
| 17 | Concorrência | Duas instâncias gravando o mesmo arquivo podem perder alterações | 🟢 | Baixo | Média | 🟡 Risco aceito (uso individual) |

## 3. Pontos fortes preservados

- Nenhuma dependência externa: zero vulnerabilidades de terceiros, build em segundos.
- Validação centralizada (`normalizar_*`) aplicada também ao que vem do arquivo.
- Gravação atômica, que não corrompe os dados em caso de falha.
- Separação clara entre visual (`ui`), teclado (`terminal`) e regras (`cliente`, `validacao`, `arquivo`).
- Modo simples automático, que mantém testes e scripts funcionando sem terminal.

## 4. Resultado (2.0)

| Medida | 1.2 | 2.0 |
|--------|-----|-----|
| Carregar 50.000 clientes | 3,096 s | **0,042 s** |
| Carregar 10.000 clientes | 0,081 s | 0,009 s |
| Verificações automatizadas | 1273 | **1337** |
| Cenários ponta a ponta | 36 | **49** |
| Achados do cppcheck | 7 | **0** |
| Avisos de compilação (rigoroso, `-Werror`) | 0 | 0 |
| Módulos | 7 | 8 (`exportar`) |
| Dependências externas | 0 | 0 |
