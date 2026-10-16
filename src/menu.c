/**
 * @file menu.c
 * @brief Menu principal e telas de cadastro, alteração, listagem e remoção.
 *
 * Fluxo de todas as operações (veja executar_operacao):
 *
 *   1. carregar o CSV para uma ListaClientes em memória;
 *   2. executar a operação sobre a lista;
 *   3. se houve mudança, gravar a lista inteira de volta no CSV;
 *   4. liberar a memória da lista.
 *
 * Trabalhar sempre a partir do arquivo mantém a tela e o disco
 * sincronizados e deixa cada operação simples de entender e testar.
 *
 * A aparência (cores, bordas, menu com setas) é responsabilidade de ui.c;
 * este arquivo só decide O QUE mostrar.
 */
#include "menu.h"
#include "arquivo.h"
#include "cliente.h"
#include "terminal.h"
#include "ui.h"
#include "validacao.h"

#include <stdio.h>
#include <string.h>

/* Opções na ordem em que aparecem no menu (índices de ITENS_MENU). */
typedef enum {
    OPCAO_CADASTRAR,
    OPCAO_ALTERAR,
    OPCAO_LISTAR,
    OPCAO_REMOVER,
    OPCAO_ENCERRAR,
    TOTAL_OPCOES
} OpcaoMenu;

/* As teclas numéricas são as mesmas do trabalho original; as letras são
 * atalhos novos (iniciais de cada ação). */
static const UiItemMenu ITENS_MENU[TOTAL_OPCOES] = {
    {'1', 'C', "Cadastrar cliente", "Inclui um novo cliente com validação de CPF, e-mail, UF e CEP."},
    {'2', 'A', "Alterar cliente", "Busca pelo CPF e edita os dados. ENTER mantém o valor atual."},
    {'3', 'L', "Listar clientes", "Tabela com todos os clientes e filtro por nome, cidade, CPF ou e-mail."},
    {'4', 'R', "Remover cliente", "Busca pelo CPF, mostra os dados e pede confirmação antes de remover."},
    {'5', 'S', "Encerrar", "Fecha o programa. Os dados já ficam salvos a cada operação."}
};

/** Assinatura comum das operações executadas sobre a lista de clientes. */
typedef void (*Operacao)(ListaClientes *lista, const char *caminho);

/* Mensagens de ajuda exibidas quando um campo é digitado incorretamente. */
#define DICA_CPF   "CPF inválido. Digite 11 dígitos, com ou sem pontuação (ex.: 529.982.247-25)."
#define DICA_EMAIL "E-mail inválido. Exemplo de formato aceito: nome@dominio.com"
#define DICA_UF    "Estado inválido. Use a sigla da UF com 2 letras (ex.: SP, RJ, MG)."
#define DICA_CEP   "CEP inválido. Digite 8 dígitos, com ou sem hífen (ex.: 01001-000)."

/* Colunas da tabela de clientes: #, Nome, CPF, Cidade/UF. */
#define COLUNAS_TABELA 4
static const size_t LARGURAS_TABELA[COLUNAS_TABELA] = {3, 27, 14, 20};

/* ------------------------------------------------------------------ */
/* Funções auxiliares de leitura                                       */
/* ------------------------------------------------------------------ */

/**
 * Lê um campo do cliente, repetindo a pergunta até o valor ser válido.
 *
 * O parâmetro `normalizar` é um ponteiro de função: o mesmo laço serve
 * para nome, e-mail, UF, CEP etc., mudando apenas a regra de validação.
 *
 * @param nome_campo     nome exibido na pergunta (ex.: "Nome").
 * @param normalizar     função de validação/padronização do campo.
 * @param dica           mensagem de erro específica, ou NULL para a genérica.
 * @param destino        campo da struct Cliente a ser preenchido.
 * @param tamanho        tamanho do campo destino.
 * @param permitir_manter se 1 (alteração), mostra o valor atual entre
 *                       colchetes e ENTER vazio mantém esse valor.
 * @return 1 se o campo foi preenchido (ou mantido), 0 se a entrada terminou.
 */
static int ler_campo(const char *nome_campo, Normalizador normalizar,
                     const char *dica, char *destino, size_t tamanho,
                     int permitir_manter)
{
    char rotulo[TAM_ENTRADA];
    char bruto[TAM_ENTRADA];

    if (permitir_manter) {
        snprintf(rotulo, sizeof rotulo, "  %s%s%s %s[%s]%s: ", ui_codigo(UI_COR_ROTULO),
                 nome_campo, ui_codigo(UI_COR_NORMAL), ui_codigo(UI_COR_SUAVE), destino,
                 ui_codigo(UI_COR_NORMAL));
    } else {
        snprintf(rotulo, sizeof rotulo, "  %s%s%s: ", ui_codigo(UI_COR_ROTULO), nome_campo,
                 ui_codigo(UI_COR_NORMAL));
    }

    for (;;) {
        switch (ler_linha(rotulo, bruto, sizeof bruto)) {
        case ENTRADA_FIM:
            return 0;
        case ENTRADA_VAZIA:
            if (permitir_manter) {
                return 1; /* mantém o valor atual */
            }
            ui_erro("Campo obrigatório.");
            continue;
        case ENTRADA_LONGA:
            break; /* mostra a dica abaixo */
        case ENTRADA_OK:
            if (normalizar(bruto, destino, tamanho)) {
                return 1;
            }
            break;
        }

        if (dica != NULL) {
            ui_erro("%s", dica);
        } else {
            ui_erro("Valor inválido. Use até %u caracteres e não utilize ';'.",
                    (unsigned)(tamanho - 1));
        }
    }
}

/**
 * Lê todos os campos do cliente, exceto o CPF.
 *
 * O operador && faz a leitura parar no primeiro campo que retornar 0
 * (fim da entrada), sem precisar de um if para cada campo.
 *
 * @param alterando 1 na alteração (ENTER mantém o valor atual), 0 no cadastro.
 * @return 1 se todos os campos foram lidos, 0 se a entrada terminou.
 */
static int preencher_dados_cliente(Cliente *c, int alterando)
{
    return ler_campo("Nome", normalizar_texto, NULL, c->nome, sizeof c->nome, alterando)
        && ler_campo("E-mail", normalizar_email, DICA_EMAIL, c->email, sizeof c->email, alterando)
        && ler_campo("Rua", normalizar_texto, NULL, c->rua, sizeof c->rua, alterando)
        && ler_campo("Número", normalizar_texto, NULL, c->numero, sizeof c->numero, alterando)
        && ler_campo("Bairro", normalizar_texto, NULL, c->bairro, sizeof c->bairro, alterando)
        && ler_campo("Cidade", normalizar_texto, NULL, c->cidade, sizeof c->cidade, alterando)
        && ler_campo("Estado (UF)", normalizar_uf, DICA_UF, c->estado, sizeof c->estado, alterando)
        && ler_campo("CEP", normalizar_cep, DICA_CEP, c->cep, sizeof c->cep, alterando);
}

/**
 * Lê um CPF válido. ENTER vazio cancela a operação.
 *
 * @return 1 se leu um CPF, 0 se o usuário cancelou, -1 se a entrada terminou.
 */
static int ler_cpf(char *cpf, size_t tamanho)
{
    char rotulo[TAM_ENTRADA];
    char bruto[TAM_ENTRADA];

    snprintf(rotulo, sizeof rotulo, "  %sCPF%s %s(ENTER vazio para cancelar)%s: ",
             ui_codigo(UI_COR_ROTULO), ui_codigo(UI_COR_NORMAL), ui_codigo(UI_COR_SUAVE),
             ui_codigo(UI_COR_NORMAL));

    for (;;) {
        ResultadoEntrada resultado = ler_linha(rotulo, bruto, sizeof bruto);

        if (resultado == ENTRADA_FIM) {
            return -1;
        }
        if (resultado == ENTRADA_VAZIA) {
            return 0;
        }
        if (resultado == ENTRADA_OK && normalizar_cpf(bruto, cpf, tamanho)) {
            return 1;
        }
        ui_erro(DICA_CPF);
    }
}

/**
 * Pede o CPF, procura o cliente e, se encontrar, exibe seus dados.
 * Usada pela alteração e pela remoção.
 *
 * @return o índice do cliente na lista, ou -1 se cancelado/não encontrado.
 */
static int localizar_cliente(const ListaClientes *lista)
{
    char cpf[TAM_CPF];
    char cpf_formatado[TAM_CPF_FORMATADO];
    int indice;
    int leitura = ler_cpf(cpf, sizeof cpf);

    if (leitura == 0) {
        printf("\n");
        ui_aviso("Operação cancelada.");
    }
    if (leitura <= 0) {
        return -1;
    }

    indice = lista_buscar_cpf(lista, cpf);
    if (indice < 0) {
        formatar_cpf(cpf, cpf_formatado, sizeof cpf_formatado);
        printf("\n");
        ui_erro("Nenhum cliente encontrado com o CPF %s.", cpf_formatado);
        return -1;
    }

    printf("\n");
    ui_info("Cliente encontrado:");
    printf("\n");
    cliente_exibir(&lista->itens[indice]);
    return indice;
}

/* ------------------------------------------------------------------ */
/* Persistência com mensagens para o usuário                           */
/* ------------------------------------------------------------------ */

static int carregar_clientes(const char *caminho, ListaClientes *lista)
{
    size_t ignoradas = 0;
    ResultadoArquivo resultado = arquivo_carregar(caminho, lista, &ignoradas);

    if (resultado != ARQUIVO_OK) {
        printf("\n");
        ui_erro("Erro em '%s': %s.", caminho, arquivo_descrever_erro(resultado));
        return 0;
    }
    if (ignoradas > 0) {
        printf("\n");
        ui_aviso("%u linha(s) inválida(s) de '%s' foram ignoradas e serão "
                 "descartadas na próxima gravação.", (unsigned)ignoradas, caminho);
    }
    return 1;
}

static int salvar_clientes(const char *caminho, const ListaClientes *lista)
{
    ResultadoArquivo resultado = arquivo_salvar(caminho, lista);

    if (resultado != ARQUIVO_OK) {
        printf("\n");
        ui_erro("Erro em '%s': %s.", caminho, arquivo_descrever_erro(resultado));
        ui_erro("Nenhuma alteração foi gravada.");
        return 0;
    }
    return 1;
}

/**
 * Desenha o cabeçalho, carrega a lista, executa a operação e libera a memória.
 * Centralizar isso garante que nenhuma tela esqueça o lista_liberar().
 *
 * @return 1 se a operação foi executada, 0 se o arquivo não pôde ser lido.
 */
static int executar_operacao(const char *titulo, Operacao operacao, const char *caminho)
{
    ListaClientes lista;
    int carregou;

    ui_cabecalho(titulo);
    lista_iniciar(&lista);
    carregou = carregar_clientes(caminho, &lista);
    if (carregou) {
        operacao(&lista, caminho);
    }
    lista_liberar(&lista);
    return carregou;
}

/* ------------------------------------------------------------------ */
/* Operações do menu                                                   */
/* ------------------------------------------------------------------ */

/**
 * Cadastro: lê o CPF primeiro (para recusar duplicados logo no início),
 * depois os demais campos, mostra um resumo e pede confirmação.
 */
static void cadastrar_cliente(ListaClientes *lista, const char *caminho)
{
    Cliente novo;
    int leitura;

    memset(&novo, 0, sizeof novo);
    printf("\n");
    ui_info("Informe os dados do novo cliente.");
    printf("\n");

    for (;;) {
        leitura = ler_cpf(novo.cpf, sizeof novo.cpf);
        if (leitura == 0) {
            printf("\n");
            ui_aviso("Cadastro cancelado.");
        }
        if (leitura <= 0) {
            return;
        }
        if (lista_buscar_cpf(lista, novo.cpf) < 0) {
            break;
        }
        ui_erro("Já existe um cliente cadastrado com este CPF.");
    }

    if (!preencher_dados_cliente(&novo, 0)) {
        return;
    }

    printf("\n");
    ui_info("Confira os dados informados:");
    printf("\n");
    cliente_exibir(&novo);

    if (ui_confirmar("\n  Confirmar cadastro?") != 1) {
        printf("\n");
        ui_aviso("Cadastro cancelado. Nenhum dado foi gravado.");
        return;
    }
    if (!lista_adicionar(lista, &novo)) {
        printf("\n");
        ui_erro("Memória insuficiente. Cadastro não realizado.");
        return;
    }
    if (salvar_clientes(caminho, lista)) {
        printf("\n");
        ui_sucesso("Cliente cadastrado com sucesso!");
    }
}

/**
 * Desenha a tabela com os clientes que correspondem ao filtro.
 * A coluna "#" mostra a posição do cliente na lista completa.
 *
 * @return quantos clientes foram exibidos.
 */
static size_t exibir_tabela_clientes(const ListaClientes *lista, const char *filtro)
{
    static const char *const TITULOS[COLUNAS_TABELA] = {"#", "Nome", "CPF", "Cidade/UF"};
    size_t exibidos = 0;
    size_t i;

    ui_tabela_topo(LARGURAS_TABELA, COLUNAS_TABELA);
    ui_tabela_linha(TITULOS, LARGURAS_TABELA, COLUNAS_TABELA, UI_COR_TITULO);
    ui_tabela_divisoria(LARGURAS_TABELA, COLUNAS_TABELA);

    for (i = 0; i < lista->quantidade; i++) {
        const Cliente *c = &lista->itens[i];
        char posicao[12];
        char cpf[TAM_CPF_FORMATADO];
        char cidade_uf[TAM_CIDADE + TAM_ESTADO + 1];
        const char *celulas[COLUNAS_TABELA];

        if (!cliente_corresponde(c, filtro)) {
            continue;
        }
        snprintf(posicao, sizeof posicao, "%u", (unsigned)(i + 1));
        formatar_cpf(c->cpf, cpf, sizeof cpf);
        snprintf(cidade_uf, sizeof cidade_uf, "%s/%s", c->cidade, c->estado);

        celulas[0] = posicao;
        celulas[1] = c->nome;
        celulas[2] = cpf;
        celulas[3] = cidade_uf;
        ui_tabela_linha(celulas, LARGURAS_TABELA, COLUNAS_TABELA, UI_COR_NORMAL);
        exibidos++;
    }

    ui_tabela_base(LARGURAS_TABELA, COLUNAS_TABELA);
    return exibidos;
}

/**
 * Listagem em tabela com filtro. A própria pergunta do filtro faz o papel
 * da pausa: ENTER vazio volta ao menu; um termo refaz a tabela filtrada.
 */
static void listar_clientes(ListaClientes *lista, const char *caminho)
{
    char filtro[TAM_ENTRADA] = "";
    char entrada[TAM_ENTRADA];
    char rotulo[TAM_ENTRADA];
    int primeira_vez = 1;

    (void)caminho; /* a listagem não grava nada */

    if (lista->quantidade == 0) {
        printf("\n");
        ui_info("Nenhum cliente cadastrado até o momento.");
        ui_pausar();
        return;
    }

    snprintf(rotulo, sizeof rotulo,
             "\n  %sFiltrar%s por nome, cidade, UF, CPF ou e-mail %s(ENTER para voltar)%s: ",
             ui_codigo(UI_COR_ROTULO), ui_codigo(UI_COR_NORMAL), ui_codigo(UI_COR_SUAVE),
             ui_codigo(UI_COR_NORMAL));

    for (;;) {
        size_t exibidos;
        ResultadoEntrada leitura;

        if (!primeira_vez) {
            ui_cabecalho("LISTA DE CLIENTES"); /* redesenha a tela já filtrada */
        }
        primeira_vez = 0;

        printf("\n");
        if (filtro[0] != '\0') {
            ui_info("Filtro: \"%s\"", filtro);
        }
        exibidos = exibir_tabela_clientes(lista, filtro);

        if (exibidos == 0) {
            ui_aviso("Nenhum cliente corresponde ao filtro.");
        }
        printf("  %sExibindo %u de %u.%s Total: %u cliente(s) cadastrado(s).\n",
               ui_codigo(UI_COR_SUAVE), (unsigned)exibidos, (unsigned)lista->quantidade,
               ui_codigo(UI_COR_NORMAL), (unsigned)lista->quantidade);

        leitura = ler_linha(rotulo, entrada, sizeof entrada);
        if (leitura == ENTRADA_FIM || leitura == ENTRADA_VAZIA) {
            return;
        }
        if (leitura == ENTRADA_OK) {
            snprintf(filtro, sizeof filtro, "%s", entrada);
        }
    }
}

/**
 * Alteração: localiza pelo CPF, edita uma CÓPIA do cliente e só substitui
 * o original na lista depois da confirmação. Os demais clientes não são
 * tocados, e o arquivo é regravado por completo.
 *
 * O CPF não é editável porque é a chave que identifica o cliente.
 */
static void alterar_cliente(ListaClientes *lista, const char *caminho)
{
    Cliente editado;
    int indice;

    printf("\n");
    if (lista->quantidade == 0) {
        ui_info("Nenhum cliente cadastrado para alterar.");
        return;
    }

    ui_info("Informe o CPF do cliente que deseja alterar.");
    printf("\n");
    indice = localizar_cliente(lista);
    if (indice < 0) {
        return;
    }

    editado = lista->itens[indice];
    printf("\n");
    ui_info("Digite os novos dados. Pressione ENTER para manter o valor atual.");
    printf("  %s(O CPF identifica o cliente e não pode ser alterado.)%s\n\n",
           ui_codigo(UI_COR_SUAVE), ui_codigo(UI_COR_NORMAL));
    if (!preencher_dados_cliente(&editado, 1)) {
        return;
    }

    if (clientes_iguais(&editado, &lista->itens[indice])) {
        printf("\n");
        ui_info("Nenhum dado foi modificado.");
        return;
    }

    printf("\n");
    ui_info("Novos dados:");
    printf("\n");
    cliente_exibir(&editado);
    if (ui_confirmar("\n  Salvar as alterações?") != 1) {
        printf("\n");
        ui_aviso("Alteração cancelada. Nenhum dado foi gravado.");
        return;
    }

    lista->itens[indice] = editado;
    if (salvar_clientes(caminho, lista)) {
        printf("\n");
        ui_sucesso("Dados alterados com sucesso!");
    }
}

/**
 * Remoção: localiza pelo CPF, mostra os dados, pede confirmação e remove
 * apenas aquele cliente. Os demais são preservados na mesma ordem.
 */
static void remover_cliente(ListaClientes *lista, const char *caminho)
{
    int indice;

    printf("\n");
    if (lista->quantidade == 0) {
        ui_info("Nenhum cliente cadastrado para remover.");
        return;
    }

    ui_info("Informe o CPF do cliente que deseja remover.");
    printf("\n");
    indice = localizar_cliente(lista);
    if (indice < 0) {
        return;
    }

    if (ui_confirmar("\n  Tem certeza de que deseja remover este cliente?") != 1) {
        printf("\n");
        ui_aviso("Remoção cancelada. Nenhum dado foi alterado.");
        return;
    }

    if (!lista_remover(lista, (size_t)indice)) {
        printf("\n");
        ui_erro("Erro interno ao remover o cliente.");
        return;
    }
    if (salvar_clientes(caminho, lista)) {
        printf("\n");
        ui_sucesso("Cliente removido com sucesso!");
    }
}

/* ------------------------------------------------------------------ */
/* Menu principal                                                      */
/* ------------------------------------------------------------------ */

/** @return só o nome do arquivo ("clientes.csv"), sem as pastas. */
static const char *nome_do_arquivo(const char *caminho)
{
    const char *nome = caminho;
    const char *p;

    for (p = caminho; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            nome = p + 1;
        }
    }
    return nome;
}

/**
 * Monta a barra de status: usuário logado, quantidade de clientes e
 * arquivo de dados. A contagem relê o arquivo, então está sempre atual.
 * Mostra só o nome do arquivo, para a barra caber em uma linha.
 */
static void montar_status(char *status, size_t tamanho, const char *usuario,
                          const char *caminho)
{
    const char *separador = ui_modo_rico() ? "   •   " : "  |  ";
    ListaClientes lista;
    char quantidade[24] = "?";

    lista_iniciar(&lista);
    if (arquivo_carregar(caminho, &lista, NULL) == ARQUIVO_OK) {
        snprintf(quantidade, sizeof quantidade, "%u", (unsigned)lista.quantidade);
    }
    lista_liberar(&lista);

    snprintf(status, tamanho, "Usuário: %s%sClientes: %s%sDados: %s",
             usuario, separador, quantidade, separador, nome_do_arquivo(caminho));
}

void menu_executar(const char *caminho_arquivo, const char *usuario)
{
    int selecionado = 0;

    /* Laço em vez de recursão: o original chamava mainMenu() de dentro de
     * cada tela, empilhando chamadas sem nunca retornar (risco de estouro
     * de pilha em uso prolongado). */
    for (;;) {
        char status[TAM_ENTRADA];
        int escolha;
        int executou = 0;

        montar_status(status, sizeof status, usuario, caminho_arquivo);
        escolha = ui_menu("MENU PRINCIPAL", status, ITENS_MENU, TOTAL_OPCOES, &selecionado);

        if (escolha == UI_MENU_FIM) {
            printf("\n\n  Entrada encerrada. Até logo!\n");
            return;
        }
        if (escolha == UI_MENU_SAIR || escolha == OPCAO_ENCERRAR) {
            printf("\n  Fim da execução. Até logo!\n");
            return;
        }

        switch (escolha) {
        case OPCAO_CADASTRAR:
            executou = executar_operacao("CADASTRO DE CLIENTE", cadastrar_cliente, caminho_arquivo);
            break;
        case OPCAO_ALTERAR:
            executou = executar_operacao("ALTERAÇÃO DE CLIENTE", alterar_cliente, caminho_arquivo);
            break;
        case OPCAO_LISTAR:
            executou = executar_operacao("LISTA DE CLIENTES", listar_clientes, caminho_arquivo);
            break;
        case OPCAO_REMOVER:
            executou = executar_operacao("REMOÇÃO DE CLIENTE", remover_cliente, caminho_arquivo);
            break;
        default:
            break;
        }

        if (entrada_encerrada()) {
            printf("\n\n  Entrada encerrada. Até logo!\n");
            return;
        }
        /* A listagem tem a própria pausa (a pergunta do filtro); mas, se o
         * arquivo nem pôde ser lido, pausamos para o erro não sumir da tela. */
        if (escolha != OPCAO_LISTAR || !executou) {
            ui_pausar();
        }
    }
}
