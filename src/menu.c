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
 */
#include "menu.h"
#include "arquivo.h"
#include "cliente.h"
#include "terminal.h"
#include "validacao.h"

#include <stdio.h>
#include <string.h>

/* Os números das opções são os mesmos do trabalho original. */
typedef enum {
    OPCAO_CADASTRAR = 1,
    OPCAO_ALTERAR   = 2,
    OPCAO_LISTAR    = 3,
    OPCAO_REMOVER   = 4,
    OPCAO_ENCERRAR  = 5
} OpcaoMenu;

/** Assinatura comum das operações executadas sobre a lista de clientes. */
typedef void (*Operacao)(ListaClientes *lista, const char *caminho);

/* Mensagens de ajuda exibidas quando um campo é digitado incorretamente. */
#define DICA_CPF   "    ! CPF inválido. Digite 11 dígitos, com ou sem pontuação (ex.: 529.982.247-25).\n"
#define DICA_EMAIL "    ! E-mail inválido. Exemplo de formato aceito: nome@dominio.com\n"
#define DICA_UF    "    ! Estado inválido. Use a sigla da UF com 2 letras (ex.: SP, RJ, MG).\n"
#define DICA_CEP   "    ! CEP inválido. Digite 8 dígitos, com ou sem hífen (ex.: 01001-000).\n"

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
        snprintf(rotulo, sizeof rotulo, "  %s [%s]: ", nome_campo, destino);
    } else {
        snprintf(rotulo, sizeof rotulo, "  %s: ", nome_campo);
    }

    for (;;) {
        switch (ler_linha(rotulo, bruto, sizeof bruto)) {
        case ENTRADA_FIM:
            return 0;
        case ENTRADA_VAZIA:
            if (permitir_manter) {
                return 1; /* mantém o valor atual */
            }
            printf("    ! Campo obrigatório.\n");
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
            printf("%s", dica);
        } else {
            printf("    ! Valor inválido. Use até %u caracteres e não utilize ';'.\n",
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
    char bruto[TAM_ENTRADA];

    for (;;) {
        ResultadoEntrada resultado =
            ler_linha("  CPF (ENTER vazio para cancelar): ", bruto, sizeof bruto);

        if (resultado == ENTRADA_FIM) {
            return -1;
        }
        if (resultado == ENTRADA_VAZIA) {
            return 0;
        }
        if (resultado == ENTRADA_OK && normalizar_cpf(bruto, cpf, tamanho)) {
            return 1;
        }
        printf(DICA_CPF);
    }
}

/**
 * Pede o CPF, procura o cliente e, se encontrar, exibe seus dados.
 * Usada pela alteração e pela remoção (antes, essa lógica não existia).
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
        printf("\n  Operação cancelada.\n");
    }
    if (leitura <= 0) {
        return -1;
    }

    indice = lista_buscar_cpf(lista, cpf);
    if (indice < 0) {
        formatar_cpf(cpf, cpf_formatado, sizeof cpf_formatado);
        printf("\n  ! Nenhum cliente encontrado com o CPF %s.\n", cpf_formatado);
        return -1;
    }

    printf("\n  Cliente encontrado:\n\n");
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
        printf("\n  ! Erro em '%s': %s.\n", caminho, arquivo_descrever_erro(resultado));
        return 0;
    }
    if (ignoradas > 0) {
        printf("\n  ! Atenção: %u linha(s) inválida(s) de '%s' foram ignoradas.\n"
               "    Elas serão descartadas na próxima gravação.\n",
               (unsigned)ignoradas, caminho);
    }
    return 1;
}

static int salvar_clientes(const char *caminho, const ListaClientes *lista)
{
    ResultadoArquivo resultado = arquivo_salvar(caminho, lista);

    if (resultado != ARQUIVO_OK) {
        printf("\n  ! Erro em '%s': %s.\n"
               "    Nenhuma alteração foi gravada.\n",
               caminho, arquivo_descrever_erro(resultado));
        return 0;
    }
    return 1;
}

/**
 * Carrega a lista, executa a operação e libera a memória.
 * Centralizar isso garante que nenhuma tela esqueça o lista_liberar().
 */
static void executar_operacao(const char *titulo, Operacao operacao, const char *caminho)
{
    ListaClientes lista;

    imprimir_titulo(titulo);
    lista_iniciar(&lista);
    if (carregar_clientes(caminho, &lista)) {
        operacao(&lista, caminho);
    }
    lista_liberar(&lista);
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
    printf("\n  Informe os dados do novo cliente.\n\n");

    for (;;) {
        leitura = ler_cpf(novo.cpf, sizeof novo.cpf);
        if (leitura == 0) {
            printf("\n  Cadastro cancelado.\n");
        }
        if (leitura <= 0) {
            return;
        }
        if (lista_buscar_cpf(lista, novo.cpf) < 0) {
            break;
        }
        printf("    ! Já existe um cliente cadastrado com este CPF.\n");
    }

    if (!preencher_dados_cliente(&novo, 0)) {
        return;
    }

    printf("\n  Confira os dados informados:\n\n");
    cliente_exibir(&novo);

    if (confirmar("\n  Confirmar cadastro?") != 1) {
        printf("\n  Cadastro cancelado. Nenhum dado foi gravado.\n");
        return;
    }
    if (!lista_adicionar(lista, &novo)) {
        printf("\n  ! Memória insuficiente. Cadastro não realizado.\n");
        return;
    }
    if (salvar_clientes(caminho, lista)) {
        printf("\n  Cliente cadastrado com sucesso!\n");
    }
}

static void listar_clientes(ListaClientes *lista, const char *caminho)
{
    size_t i;

    (void)caminho; /* a listagem não grava nada */

    if (lista->quantidade == 0) {
        printf("\n  Nenhum cliente cadastrado até o momento.\n");
        return;
    }

    for (i = 0; i < lista->quantidade; i++) {
        printf("\n  --- Cliente %u de %u ---\n", (unsigned)(i + 1),
               (unsigned)lista->quantidade);
        cliente_exibir(&lista->itens[i]);
    }
    printf("\n  Total: %u cliente(s) cadastrado(s).\n", (unsigned)lista->quantidade);
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

    if (lista->quantidade == 0) {
        printf("\n  Nenhum cliente cadastrado para alterar.\n");
        return;
    }

    printf("\n  Informe o CPF do cliente que deseja alterar.\n\n");
    indice = localizar_cliente(lista);
    if (indice < 0) {
        return;
    }

    editado = lista->itens[indice];
    printf("\n  Digite os novos dados. Pressione ENTER para manter o valor atual.\n"
           "  (O CPF identifica o cliente e não pode ser alterado.)\n\n");
    if (!preencher_dados_cliente(&editado, 1)) {
        return;
    }

    if (clientes_iguais(&editado, &lista->itens[indice])) {
        printf("\n  Nenhum dado foi modificado.\n");
        return;
    }

    printf("\n  Novos dados:\n\n");
    cliente_exibir(&editado);
    if (confirmar("\n  Salvar as alterações?") != 1) {
        printf("\n  Alteração cancelada. Nenhum dado foi gravado.\n");
        return;
    }

    lista->itens[indice] = editado;
    if (salvar_clientes(caminho, lista)) {
        printf("\n  Dados alterados com sucesso!\n");
    }
}

/**
 * Remoção: localiza pelo CPF, mostra os dados, pede confirmação e remove
 * apenas aquele cliente. Os demais são preservados na mesma ordem.
 */
static void remover_cliente(ListaClientes *lista, const char *caminho)
{
    int indice;

    if (lista->quantidade == 0) {
        printf("\n  Nenhum cliente cadastrado para remover.\n");
        return;
    }

    printf("\n  Informe o CPF do cliente que deseja remover.\n\n");
    indice = localizar_cliente(lista);
    if (indice < 0) {
        return;
    }

    if (confirmar("\n  Tem certeza de que deseja remover este cliente?") != 1) {
        printf("\n  Remoção cancelada. Nenhum dado foi alterado.\n");
        return;
    }

    if (!lista_remover(lista, (size_t)indice)) {
        printf("\n  ! Erro interno ao remover o cliente.\n");
        return;
    }
    if (salvar_clientes(caminho, lista)) {
        printf("\n  Cliente removido com sucesso!\n");
    }
}

/* ------------------------------------------------------------------ */
/* Menu principal                                                      */
/* ------------------------------------------------------------------ */

static void exibir_menu(void)
{
    imprimir_titulo("GERENCIADOR DE CLIENTES DOGWALKER");
    printf("  [1] Cadastrar cliente\n");
    printf("  [2] Alterar cliente\n");
    printf("  [3] Listar clientes\n");
    printf("  [4] Remover cliente\n");
    printf("  [5] Encerrar\n");
}

void menu_executar(const char *caminho_arquivo)
{
    char entrada[TAM_ENTRADA];
    int opcao;

    /* Laço em vez de recursão: o original chamava mainMenu() de dentro de
     * cada tela, empilhando chamadas sem nunca retornar (risco de estouro
     * de pilha em uso prolongado). */
    for (;;) {
        ResultadoEntrada leitura;

        exibir_menu();
        leitura = ler_linha("\n  Escolha a opção desejada: ", entrada, sizeof entrada);
        if (leitura == ENTRADA_FIM) {
            printf("\n\n  Entrada encerrada. Até logo!\n");
            return;
        }
        if (leitura != ENTRADA_OK || !converter_inteiro(entrada, &opcao)) {
            opcao = 0; /* cai no default do switch */
        }

        switch (opcao) {
        case OPCAO_CADASTRAR:
            executar_operacao("CADASTRO DE CLIENTE", cadastrar_cliente, caminho_arquivo);
            break;
        case OPCAO_ALTERAR:
            executar_operacao("ALTERAÇÃO DE CLIENTE", alterar_cliente, caminho_arquivo);
            break;
        case OPCAO_LISTAR:
            executar_operacao("LISTA DE CLIENTES", listar_clientes, caminho_arquivo);
            break;
        case OPCAO_REMOVER:
            executar_operacao("REMOÇÃO DE CLIENTE", remover_cliente, caminho_arquivo);
            break;
        case OPCAO_ENCERRAR:
            printf("\n  Fim da execução. Até logo!\n");
            return;
        default:
            printf("\n  ! Opção inválida. Digite um número de 1 a 5.\n");
            continue; /* volta direto ao menu, sem pausar */
        }

        if (entrada_encerrada()) {
            printf("\n\n  Entrada encerrada. Até logo!\n");
            return;
        }
        pausar();
    }
}
