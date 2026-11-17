/*
 * Analisador lexico da linguagem P.
 *
 * O reconhecimento e feito por um AFD representado em uma tabela de
 * transicoes (estado x classe de caractere). O resultado e um vetor com
 * todos os tokens, na ordem em que foram reconhecidos, terminado por EOF.
 */
#ifndef LEXICO_H
#define LEXICO_H

#include <stddef.h>
#include <stdio.h>

#include "util.h"

typedef enum {
    /* palavras reservadas */
    TK_FUNCTION, TK_MAIN, TK_LET, TK_INT, TK_FLOAT, TK_CHAR, TK_IF, TK_ELSE,
    TK_WHILE, TK_PRINTLN, TK_RETURN,
    /* pontuacao e estruturacao */
    TK_LBRACKET, TK_RBRACKET, TK_LBRACE, TK_RBRACE, TK_ARROW, TK_COLON,
    TK_SEMICOLON, TK_COMMA,
    /* operadores */
    TK_ASSIGN, TK_EQ, TK_NE, TK_GT, TK_GE, TK_LT, TK_LE,
    TK_PLUS, TK_MINUS, TK_MULT, TK_DIV,
    /* identificadores e literais */
    TK_ID, TK_INT_CONST, TK_FLOAT_CONST, TK_CHAR_LITERAL, TK_FMT_STRING,
    /* marcador de fim do vetor (nao e um lexema da linguagem) */
    TK_EOF
} TipoToken;

typedef struct {
    char *lexema;
    TipoToken tipo;
    int linha;
} Token;

typedef struct {
    Token *itens;
    size_t quantidade;
    size_t capacidade;
} VetorTokens;

const char *nomeToken(TipoToken tipo);

/* Reconhece todo o codigo-fonte. Erros lexicos sao registrados em `erros`
   e a analise continua a partir do proximo caractere. */
void analisarLexico(const char *fonte, size_t tamanho, VetorTokens *tokens, ListaErros *erros);

void imprimirTokens(FILE *saida, const VetorTokens *tokens);
void imprimirAFD(FILE *saida);
void liberarTokens(VetorTokens *tokens);

#endif
