/*
 * Tabelas de simbolos: uma por funcao definida no codigo.
 *
 * Cada tabela guarda variaveis locais, parametros e as funcoes chamadas
 * dentro dela. A especificacao sugere um HashMap indexado pelo lexema; aqui
 * cada tabela e um vetor em que a chave (o nome) e unica, o que preserva as
 * mesmas informacoes e mantem a ordem de insercao na impressao.
 */
#ifndef TABELA_H
#define TABELA_H

#include <stddef.h>
#include <stdio.h>

/* Representacao interna dos tipos (nao confundir com os tokens). */
typedef enum {
    DATA_ERROR = -1,
    DATA_INT = 0,
    DATA_FLOAT = 1,
    DATA_CHAR = 2,
    DATA_VOID = 3
} DataTypes;

/* Registro de uma chamada de funcao. */
typedef struct {
    int id;          /* numero do registro dentro da tabela (para impressao) */
    char *name;      /* nome da funcao chamada */
    int num_args;    /* numero de argumentos da chamada */
    char **args;     /* lexema de cada argumento */
    int linha;
} FunctionRegister;

typedef struct {
    char *name;
    DataTypes datatype;
    int is_param;
    int pos_param;                  /* -1 para variaveis locais */
    FunctionRegister **call_refs;   /* chamadas registradas para este nome */
    int num_call_refs;
    int capacidade_call_refs;
    int eh_chamada;                 /* entrada criada por uma chamada de funcao */
    int linha;
} Simbolo;

typedef struct {
    char *funcao;
    DataTypes ret_type;
    int linha;
    Simbolo *simbolos;
    size_t quantidade;
    size_t capacidade;
    int totalRegistros;
} TabelaSimbolos;

typedef struct {
    TabelaSimbolos **itens;
    size_t quantidade;
    size_t capacidade;
} ConjuntoTabelas;

const char *nomeDataType(DataTypes tipo);

TabelaSimbolos *criarTabela(ConjuntoTabelas *tabelas, const char *funcao, int linha);
Simbolo *buscarSimbolo(TabelaSimbolos *tabela, const char *nome);

/* Insere um simbolo novo. Devolve NULL se o nome ja existe na tabela
   (a redeclaracao sera relatada pela analise semantica). */
Simbolo *inserirSimbolo(TabelaSimbolos *tabela, const char *nome, DataTypes tipo,
                        int is_param, int pos_param, int linha);

/* Cria um FunctionRegister para uma chamada e o anexa ao call_refs da
   entrada com o nome da funcao (criada na primeira chamada). */
FunctionRegister *registrarChamada(TabelaSimbolos *tabela, const char *nome, int linha);
void adicionarArgumento(FunctionRegister *registro, char *argumento);

/* Depois de todas as funcoes serem lidas, o datatype de cada funcao chamada
   passa a ser o ret_type da funcao correspondente (que pode ter sido
   definida depois da chamada). Funcoes inexistentes ficam com DATA_ERROR. */
void resolverTiposDasChamadas(ConjuntoTabelas *tabelas);

void imprimirTabelas(FILE *saida, const ConjuntoTabelas *tabelas);
void liberarTabelas(ConjuntoTabelas *tabelas);

#endif
