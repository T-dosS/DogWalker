# Segurança

## Como relatar uma vulnerabilidade

Encontrou uma falha de segurança? **Não abra uma issue pública.** Use o recurso
[*Report a vulnerability*](https://github.com/T-dosS/DogWalker/security/advisories/new)
do GitHub (aba **Security** do repositório), descrevendo:

- a versão ou o commit afetado;
- os passos para reproduzir;
- o impacto esperado.

O retorno inicial é dado em até 7 dias.

## Versões com suporte

| Versão | Suporte |
|--------|---------|
| 2.x    | ✅ correções de segurança |
| 1.x    | ❌ atualize para a 2.x |

## Modelo de ameaça

O DogWalker é um programa **local, de um único usuário por vez**, que guarda os
dados em um arquivo CSV no próprio computador. Não há rede, servidor, banco de
dados nem dependências externas.

| Ativo | Ameaça considerada | Proteção atual |
|-------|--------------------|----------------|
| Memória do processo | Estouro de buffer por entrada longa | Toda leitura usa `fgets` com limite; nenhum `gets`, `scanf("%s")`, `strcpy`, `sprintf` ou `system` no código; análise estática (`cppcheck`) e `-fanalyzer` sem achados |
| Terminal | Dado cadastrado com códigos de controle (ANSI) alterando a tela | Validação recusa caracteres de controle (< 32 e 127) na digitação **e** na leitura do CSV |
| Arquivo de dados | Corrupção por falha no meio da gravação | Gravação em `.tmp` e troca atômica (`rename` / `MoveFileEx`) só se tudo foi escrito |
| Arquivo de dados | CSV editado à mão com linhas malformadas ou CPF duplicado | Cada linha passa pelas mesmas validações do cadastro; linhas inválidas são ignoradas e contadas |
| Relatório HTML | Injeção de HTML/JS por meio de nomes e endereços | Todo texto é escapado (`& < > " '`); a página não tem JavaScript nem recursos externos |
| Planilha exportada | Injeção de fórmulas no Excel/LibreOffice | Valores iniciados por `= + - @`, tab ou CR recebem um apóstrofo |
| Dados pessoais (LGPD) | Exposição de CPF em material impresso/compartilhado | CPF mascarado no relatório HTML (`***.982.247-**`); `.gitignore` impede que `data/` vá para o Git |
| Acesso | Tentativa e erro de senha | Login limitado a 3 tentativas, mensagem genérica (não revela se o erro foi no usuário ou na senha), senha mascarada |

## Limitações conhecidas (riscos aceitos)

- **Credenciais padrão `admin`/`admin`**, mantidas do trabalho acadêmico. Podem ser trocadas pelas
  variáveis de ambiente `DOGWALKER_USUARIO` e `DOGWALKER_SENHA`. Senha com *hash* e arquivo de
  usuários estão no [roadmap](docs/ROADMAP.md) como decisão pendente.
- **O arquivo CSV não é criptografado.** Qualquer pessoa com acesso à pasta `data/` lê os dados.
  Proteja a pasta com as permissões do sistema operacional.
- **Sem controle de concorrência:** duas instâncias gravando o mesmo arquivo ao mesmo tempo podem
  sobrescrever as alterações uma da outra. O programa foi feito para um usuário por vez.
- **Arquivos exportados** (`relatorio.html`, `exportacao.csv`) ficam na pasta dos dados e são
  sobrescritos a cada exportação. A planilha contém o CPF completo.

## Boas práticas para quem contribui

- Nunca faça commit de dados reais de clientes, credenciais ou arquivos de `data/`.
- Toda entrada nova deve passar por uma função `normalizar_*` (validação + padronização).
- Formatos de `printf` são sempre literais; dados do usuário vão como argumento (`"%s"`).
- Rode `make check` antes de abrir um Pull Request.
