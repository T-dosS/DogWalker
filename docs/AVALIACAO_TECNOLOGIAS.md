# Avaliação de tecnologias

Pergunta: **vale a pena trazer novas linguagens, frameworks ou banco de dados para o
DogWalker?** Critério único: ganho real em desempenho, segurança, organização,
manutenção, escalabilidade, visual, integração ou redução de bugs. Adicionar
tecnologia só para aumentar a lista não conta.

## Contexto que pesa na decisão

- O DogWalker é um **gerenciador local**, usado por uma pessoa por vez num pequeno negócio de passeios.
- É um **projeto de portfólio em C**. Demonstrar C bem feito é parte do objetivo.
- O núcleo está **estável e testado** (1.337 verificações, 49 cenários) e não tem dependências.
- Volume esperado: centenas a poucos milhares de clientes. Com a carga linear da 2.0,
  50 mil clientes carregam em 0,04 s.

## Avaliação

| Tecnologia | Ganho real hoje? | Análise | Decisão |
|------------|------------------|---------|---------|
| **HTML5 + CSS3** | ✅ Sim | Um relatório que se abre em qualquer navegador, imprime bem e funciona no celular; o programa em C gera o arquivo, sem servidor | **Adotado** (relatório `/e`) |
| **GitHub Actions** | ✅ Sim | Testa Linux e Windows a cada PR; é a única forma prática de validar o código `termios` | **Adotado** (workflow pronto em `docs/ci/`; falta só o escopo do token) |
| **cppcheck** | ✅ Sim | Análise estática gratuita, sem dependência no programa final | **Adotado** (`make lint`) |
| TypeScript / JavaScript | ❌ Não | Não há interface web nem lógica no navegador; o relatório funciona melhor **sem** JS (mais leve, sem risco de XSS) | Não adotado |
| React / Next.js | ❌ Não agora | Exigiria reescrever toda a interface e criar uma API; duplicaria as validações em duas linguagens | Futuro, se virar produto web |
| Node.js / Python (back-end) | ❌ Não agora | Uma API só faz sentido com múltiplos usuários ou acesso remoto, que não é o caso hoje | Futuro (fase 2 abaixo) |
| Tailwind CSS | ❌ Não | Para uma única página gerada, CSS próprio embutido (~3 KB) é menor e não precisa de build | Não adotado |
| PostgreSQL | ❌ Não agora | Exige servidor instalado e configurado; não há concorrência nem volume que justifiquem | Futuro (multiusuário) |
| SQLite | 🟡 Talvez | Seria o próximo passo natural se surgirem consultas complexas ou o cadastro de cães (relacionamento 1:N) | Candidato para a 3.0 |
| ncurses / PDCurses | ❌ Não | A interface ANSI própria já cobre o necessário sem dependência (ver `INTERFACE.md`) | Não adotado |

## Por que não migrar para web agora

| Risco da migração | Consequência |
|-------------------|--------------|
| Reescrita completa | Joga fora um núcleo estável e testado e reintroduz bugs já resolvidos |
| Duas bases de validação (C + TypeScript) | Regras de CPF/e-mail divergindo com o tempo |
| Superfície de ataque nova | Autenticação web, sessões, CSRF, XSS, CORS, hospedagem, HTTPS |
| Custo de operação | Servidor, banco, backups e deploy para um uso que hoje é local e individual |
| Perda de identidade do portfólio | O projeto deixa de demonstrar C, que é o seu diferencial |

## Plano de evolução, se o DogWalker virar um produto multiusuário

Cada fase só começa se a anterior trouxer valor comprovado. Nenhuma está implementada.

1. **Fase 1: dados mais ricos (C + SQLite).** Trocar o CSV por SQLite
   (biblioteca embutida, um arquivo), com tabelas `clientes`, `caes` e `passeios`. O módulo
   `arquivo` já isola a persistência, então só ele muda.
2. **Fase 2: API REST.** Expor as operações por HTTP, por exemplo em C com uma
   biblioteca HTTP leve, ou com um serviço em Node.js/TypeScript ou Python que use o
   mesmo banco. Autenticação com senha em *hash* (Argon2) e tokens.
3. **Fase 3: interface web.** Front-end em React/Next.js + TypeScript consumindo a API, com
   agenda de passeios e acesso pelo celular para os passeadores.
4. **Fase 4: PostgreSQL**, se houver muitos usuários simultâneos, com migração a partir do SQLite.

Até lá, a recomendação é manter o núcleo em C e evoluir o domínio (cães, telefone, agenda),
pois é isso que entrega valor ao usuário real.
