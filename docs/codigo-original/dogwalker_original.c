/*
 * CÓDIGO ORIGINAL DO TRABALHO (preservado apenas para comparação histórica).
 *
 * Este arquivo NÃO faz parte da compilação do projeto. Ele está aqui para
 * mostrar a evolução do código. A análise completa dos problemas está em
 * docs/ANALISE_CODIGO_ORIGINAL.md.
 *
 * Observação: o código abaixo depende de <conio.h> e de system("cls"),
 * portanto só compila (com warnings e erros) em ambientes Windows antigos.
 */

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <conio.h>
#include <stdlib.h>
void Cadastro();
void Listar();
void Alterar();
void Remover();
// Chamada de funções externas
struct VarG {
 int Op;
 char Cd;
};
// Variáveis do tipo Cadastro.Pessoa
struct Cadastro {
 char Nome[200];
 char Email[100];
 char Cpf[20];
 char Rua[100];
 char Numero[20];
 char Bairro[100];
 char Cidade[100];
 char Estado[30];
 char Cep[20];
};
FILE* DB;
struct VarG Global;
struct Cadastro Pessoa;
void mainMenu() {
 setlocale(LC_ALL,"Portuguese");
 system("cls");
 printf("\n\n\t\t ****** \n");
 printf("\t\t * GERENCIADOR DE CLIENTES DOGWALKER * \n");
 printf("\t\t ****** \n\n");
 printf("\t\t ****** \n");
 printf("\t\t * [1] - Cadastrar * \n");
 printf("\t\t * [2] - Alterar * \n");
 printf("\t\t * [3] - Listar * \n");
 printf("\t\t * [4] - Remover * \n");
 printf("\t\t * [5] - Encerrar * \n");
 printf("\t\t ****** \n");
 printf("\n\t\t Escolha a opção desejada: ");
 scanf("%d", &Global.Op);
 getchar();
 switch (Global.Op) {
 case 1:
 system("cls");
 Cadastro();
 break;
 case 2:
 system("cls");
 Alterar();
 break;
 case 3:
 system("cls");
 Listar();
 break;
 case 4:
 system("cls");
 Remover();
 break;
 case 5:
 system("cls");
 printf("\n\t\t ***** \n");
 printf("\t\t * Fim da Execucao... *\n");
 printf("\t\t ***** \n\n");
 break;
 default:
 system("cls");
 printf("\t\t Valor inválido! Tente novamente...\n");
 sleep(1);
 mainMenu();
 break;
 }
}
void Cadastro() {
 Cadastro;
 Pessoa;
 system("cls");
 DB = fopen("Dados_Clientes.txt", "a");
 if(DB == NULL) {
 printf("\n\t\t Erro no Banco de Dados...\n");
 return 1;
 } else {
 system("cls");
 printf("\n\n\t\t ***** \n");
 printf("\t\t * TELA DE CADASTRO * \n");
 printf("\t\t ***** \n");
 printf("\n\t\t Nome: ");
 fgets(Pessoa.Nome, 200, stdin);
 printf("\t\t E-mail: ");
 fgets(Pessoa.Email, 100, stdin);
 printf("\t\t CPF: ");
 fgets(Pessoa.Cpf, 20, stdin);
 printf("\t\t Rua: ");
 fgets(Pessoa.Rua, 100, stdin);
 printf("\t\t Número: ");
 fgets(Pessoa.Numero, 20, stdin);
 printf("\t\t Bairro: ");
 fgets(Pessoa.Bairro, 100, stdin);
 printf("\t\t Cidade: ");
 fgets(Pessoa.Cidade, 100, stdin);
 printf("\t\t Estado: ");
 fgets(Pessoa.Estado, 30, stdin);
 printf("\t\t Cep: ");
 fgets(Pessoa.Cep, 20, stdin);
 printf("\n\n\t\t Finalizar cadastro? [s/n] : ");
 scanf("%c", &Global.Cd);
 if(Global.Cd == 's') {
 system("cls");
 fprintf(DB,
 "Nome: %s E-mail: %s CPF: %s Rua: %s Número: %s Bairro: %s Cidade: %s ESTADO: %s CEP: %s",
 Pessoa.Nome,
 Pessoa.Email,
 Pessoa.Cpf,
 Pessoa.Rua,
 Pessoa.Numero,
 Pessoa.Bairro,
 Pessoa.Cidade,
 Pessoa.Estado,
 Pessoa.Cep
 );
 fprintf(DB, "----------------------------------------------------------------------------------------\n");
 fclose(DB);
 printf("\t\t ***** \n");
 printf("\t\t * CADASTRO CONCLUÍDO COM SUCESSO * \n");
 printf("\t\t ***** \n");
 sleep(1);
 mainMenu();
 } else if(Global.Cd == 'n') {
 system("cls");
 printf("\t\t Cadastro Cancelado! Voltando ao Menu Principal...\n");
 fclose(DB);
 getchar();
 sleep(1);
 mainMenu();
 } else {
 system("cls");
 printf("\t\t Valor inválido! Tente novamente...\n");
 fclose(DB);
 sleep(1);
 Cadastro();
 }
 }
}
void Alterar() {
 system("cls");
 char cpf[12];
 Cadastro;
 Pessoa;
 DB=fopen("Dados_Clientes.txt","r");
 if (DB==NULL){
 system ("cls");
 printf("\n\t\t Erro no Banco de Dados...\n\n");
 return 1;
 }
 else{
 system("cls");
 printf("\n\n\t\t ***** \n");
 printf("\t\t * ALTERAÇÃO DE DADOS * \n");
 printf("\t\t ***** \n\n");
 }
 printf("\nInforme o CPF do cliente para alteração de dados:");
 scanf("%s",&cpf);
 printf("\nDeseja continuar com a atualização?\n");
 printf("(1) - SIM\n");
 printf("(2) - NAO\n\n");
 printf("OPÇÃO: ");
 int simounao;
 scanf("%d",&simounao);
 if(simounao == 2){
 system("cls");
 mainMenu();
 } else {
 FILE *DB;
 DB=fopen("Dados_Clientes.txt","r+");
 if(DB == NULL){
 printf("CLIENTE INEXISTENTE EM NOSSO BANCO DE DADOS!\n\n");
 Alterar();
 } else {
 system("cls");
 fflush(stdin);
 system("cls");
 printf("\n\n\t\t ***** \n");
 printf("\t\t * ALTERAÇÃO DE DADOS * \n");
 printf("\t\t ***** \n\n");
 printf("\nDigite os novos dados do Cliente:\n");
 printf("\t\n E-mail: ");
 fgets(Pessoa.Email, 100, stdin);
 printf("\t\n Rua: ");
 fgets(Pessoa.Rua, 100, stdin);
 printf("\t\n Número: ");
 fgets(Pessoa.Numero, 20, stdin);
 printf("\t\n Bairro: ");
 fgets(Pessoa.Bairro, 100, stdin);
 printf("\t\n Cidade: ");
 fgets(Pessoa.Cidade, 100, stdin);
 printf("\t\n Estado: ");
 fgets(Pessoa.Estado, 30, stdin);
 printf("\t\n Cep: ");
 fgets(Pessoa.Cep, 20, stdin);
 fclose(DB);
 DB=fopen("Dados_Clientes.txt","rb+");
 fprintf(DB,"===============================================================");
 fprintf(DB,"\n");
 fprintf(DB,"Email: ");
 fputs(Pessoa.Email,DB);
 fprintf(DB,"Rua: ");
 fputs(Pessoa.Rua,DB);
 fprintf(DB,"Número: ");
 fputs(Pessoa.Numero,DB);
 fprintf(DB,"Bairro: ");
 fputs(Pessoa.Bairro,DB);
 fprintf(DB,"Cidade: ");
 fputs(Pessoa.Cidade,DB);
 fprintf(DB,"Estado: ");
 fputs(Pessoa.Estado,DB);
 fprintf(DB,"Cep: ");
 fputs(Pessoa.Cep,DB);
 fprintf(DB,"===============================================================");
 fclose(DB);
 fflush(stdin);
 system("cls");
 printf("\t\t ***** \n");
 printf("\t\t * DADOS ALTERADOS COM SUCESSO * \n");
 printf("\t\t ***** \n");
 sleep(1);
 mainMenu();
 }
 }
}
void Listar() {
 system("cls");
 DB = fopen("Dados_Clientes.txt", "r");
 if(DB == NULL) {
 system("cls");
 printf("\n\t\t Erro no Banco de Dados...\n\n");
 return 1;
 } else {
 system("cls");
 printf("\n\n\t\t ***** \n");
 printf("\t\t * LISTA DE CLIENTES * \n");
 printf("\t\t ***** \n\n");
 char listagem[300];
 while(fgets(listagem, 300, DB) != NULL) {
 printf("%s", listagem);
 }
 fclose(DB);
 printf("\n\t\t Voltar ao Menu Principal? [s/n]: ");
 scanf("%s", &Global.Cd);
 if(Global.Cd == 's') {
 mainMenu();
 } else if(Global.Cd != 's' && Global.Cd != 'n') {
 printf("\t\t Valor inválido! Voltando ao Menu Principal...\n\n");
 sleep(1);
 mainMenu();
 }
 }
}
void Remover() {
}
int opcao = 0;
int main(int argc, char** argv) {
 setlocale(LC_ALL,"Portuguese");
 char login[15] = "admin";
 char login1[15];
 char senha[15] = "admin";
 char senha1[15];
 int login_efetuado = 0;
 while(!login_efetuado){
 system("cls");
 printf("\n\n\t\t ***** \n");
 printf("\t\t * TELA DE LOGIN * \n");
 printf("\t\t ***** \n\n");
 printf("\t\t Insira o Login: ");
 scanf("%s", login1);
 printf("\t\t Insira a Senha: ");
 scanf("%s", senha1);
 if (strcmp(login, login1) == 0 &&
 strcmp(senha, senha1) == 0) {
 system("cls");
 printf("\n\n\t\t LOGADO COM SUCESSO!");
 login_efetuado = 1;
 } else {
 printf("\n\n\t\t DADOS INVÁLIDOS! ");
 }
 }
 mainMenu();
 return 0;
}
