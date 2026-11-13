#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAM_LEXEMA 128

typedef enum {
    TOK_INT, TOK_FLOAT, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_PRINT,
    TOK_IDENTIFIER, TOK_INT_CONST, TOK_FLOAT_CONST,
    TOK_PLUS, TOK_MINUS, TOK_MULT, TOK_DIV,
    TOK_ASSIGN, TOK_LT, TOK_LE, TOK_GT, TOK_GE, TOK_EQ, TOK_NE,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_SEMICOLON,
    TOK_EOF, TOK_INVALID
} TokenType;

typedef struct {
    TokenType tipo;
    char lexema[TAM_LEXEMA];
    int linha;
} Token;

const char *nomeToken(TokenType tipo)
{
    switch (tipo) {
        case TOK_INT: 
            return "INT";
        case TOK_FLOAT: 
            return "FLOAT";
        case TOK_IF: 
            return "IF";
        case TOK_ELSE: 
            return "ELSE";
        case TOK_WHILE: 
            return "WHILE";
        case TOK_PRINT: 
            return "PRINT";
        case TOK_IDENTIFIER: 
            return "IDENTIFIER";
        case TOK_INT_CONST: 
            return "INT_CONST";
        case TOK_FLOAT_CONST: 
            return "FLOAT_CONST";
        case TOK_PLUS: 
            return "PLUS";
        case TOK_MINUS: 
            return "MINUS";
        case TOK_MULT: 
            return "MULT";
        case TOK_DIV: 
            return "DIV";
        case TOK_ASSIGN: 
            return "ASSIGN";
        case TOK_LT: 
            return "LT";
        case TOK_LE: 
            return "LE";
        case TOK_GT: 
            return "GT";
        case TOK_GE: 
            return "GE";
        case TOK_EQ: 
            return "EQ";
        case TOK_NE: 
            return "NE";
        case TOK_LPAREN: 
            return "LPAREN";
        case TOK_RPAREN: 
            return "RPAREN";
        case TOK_LBRACE: 
            return "LBRACE";
        case TOK_RBRACE: 
            return "RBRACE";
        case TOK_SEMICOLON: 
            return "SEMICOLON";
        case TOK_EOF: 
            return "EOF";
        case TOK_INVALID: 
            return "INVALID";
        default: 
            return "UNKNOWN";
    }
}

void imprimirToken(Token token)
{
    printf("%-14s %-20s linha %d\n",
           nomeToken(token.tipo), token.lexema, token.linha);
}

typedef struct {
    FILE *arquivo;
    int linha;
} CodigoFonte;

int abrirCodigoFonte(CodigoFonte *codigo, const char *nomeArquivo)
{
    codigo->arquivo = fopen(nomeArquivo, "r");
    codigo->linha = 1;

    if (codigo->arquivo == NULL) {
        fprintf(stderr, "Nao foi possivel abrir o arquivo: %s\n", nomeArquivo);
        return 0;
    }

    return 1;
}

void fecharCodigoFonte(CodigoFonte *codigo)
{
    if (codigo->arquivo != NULL) {
        fclose(codigo->arquivo);
        codigo->arquivo = NULL;
    }
}

/* Retornamos int porque fgetc() precisa poder retornar EOF. */
int lerCaractere(CodigoFonte *codigo)
{
    int caractere = fgetc(codigo->arquivo);

    if (caractere == '\n') {
        codigo->linha++;
    }

    return caractere;
}

void devolverCaractere(CodigoFonte *codigo, int caractere)
{
    if (caractere == EOF) {
        return;
    }

    ungetc(caractere, codigo->arquivo);

    if (caractere == '\n') {
        codigo->linha--;
    }
}

/* Lê um caractere para consulta e depois o devolve ao arquivo (lookahead). */
int olharCaractere(CodigoFonte *codigo)
{
    int caractere = lerCaractere(codigo);
    devolverCaractere(codigo, caractere);
    return caractere;
}


void erroLexico(int linha)
{
    fprintf(stderr, "Erro lexico na linha %d.\n", linha);
}

void erroSintatico(int linha, TokenType esperado, TokenType encontrado)
{
    fprintf(stderr, "Erro sintatico na linha %d: esperado %s, encontrado %s.\n",
            linha, nomeToken(esperado), nomeToken(encontrado));
}

void erroSemantico(int linha, const char *mensagem)
{
    fprintf(stderr, "Erro semantico na linha %d: %s\n", linha, mensagem);
}


int adicionarCaractere(Token *token, size_t *tamanho, int caractere)
{
    if (*tamanho >= TAM_LEXEMA - 1) {
        fprintf(stderr, "Erro lexico na linha %d: lexema muito longo.\n", token->linha);
        token->tipo = TOK_INVALID;
        return 0;
    }

    token->lexema[(*tamanho)++] = (char)caractere;
    token->lexema[*tamanho] = '\0';
    return 1;
}

Token proximoToken(CodigoFonte *codigo)
{
    Token token = { TOK_EOF, "", codigo->linha };
    int caractere;
    size_t tamanho = 0;

    while (1) {
        caractere = lerCaractere(codigo);

        if (caractere == EOF) {
            token.linha = codigo->linha;
            return token;
        }
        if (isspace((unsigned char)caractere)) {
            continue;
        }
        if (caractere == '/' && olharCaractere(codigo) == '/') {
            lerCaractere(codigo); /* consome a segunda barra */
            while ((caractere = lerCaractere(codigo)) != EOF && caractere != '\n') {
                /* ignora o conteudo do comentario */
            }
            continue;
        }
        break;
    }

    token.linha = codigo->linha;

    /* Identificadores e palavras reservadas. */
    if (isalpha((unsigned char)caractere) || caractere == '_') {
        do {
            if (!adicionarCaractere(&token, &tamanho, caractere)) return token;
            caractere = lerCaractere(codigo);
        } while (caractere != EOF &&
                 (isalnum((unsigned char)caractere) || caractere == '_'));

        devolverCaractere(codigo, caractere);
        token.tipo = TOK_IDENTIFIER;

        if      (strcmp(token.lexema, "int") == 0)   token.tipo = TOK_INT;
        else if (strcmp(token.lexema, "float") == 0) token.tipo = TOK_FLOAT;
        else if (strcmp(token.lexema, "if") == 0)    token.tipo = TOK_IF;
        else if (strcmp(token.lexema, "else") == 0)  token.tipo = TOK_ELSE;
        else if (strcmp(token.lexema, "while") == 0) token.tipo = TOK_WHILE;
        else if (strcmp(token.lexema, "print") == 0) token.tipo = TOK_PRINT;
        return token;
    }

    /* Numeros inteiros e decimais no formato [0-9]+(\.[0-9]+)?. */
    if (isdigit((unsigned char)caractere)) {
        do {
            if (!adicionarCaractere(&token, &tamanho, caractere)) return token;
            caractere = lerCaractere(codigo);
        } while (caractere != EOF && isdigit((unsigned char)caractere));

        token.tipo = TOK_INT_CONST;
        if (caractere == '.') {
            if (!adicionarCaractere(&token, &tamanho, caractere)) return token;
            if (!isdigit((unsigned char)olharCaractere(codigo))) {
                fprintf(stderr, "Erro lexico na linha %d: numero decimal incompleto '%s'.\n",
                        token.linha, token.lexema);
                token.tipo = TOK_INVALID;
                return token;
            }

            caractere = lerCaractere(codigo);
            do {
                if (!adicionarCaractere(&token, &tamanho, caractere)) return token;
                caractere = lerCaractere(codigo);
            } while (caractere != EOF && isdigit((unsigned char)caractere));
            token.tipo = TOK_FLOAT_CONST;
        }
        devolverCaractere(codigo, caractere);
        return token;
    }

    adicionarCaractere(&token, &tamanho, caractere);
    switch (caractere) {
        case '+': token.tipo = TOK_PLUS; break;
        case '-': token.tipo = TOK_MINUS; break;
        case '*': token.tipo = TOK_MULT; break;
        case '/': token.tipo = TOK_DIV; break;
        case '(': token.tipo = TOK_LPAREN; break;
        case ')': token.tipo = TOK_RPAREN; break;
        case '{': token.tipo = TOK_LBRACE; break;
        case '}': token.tipo = TOK_RBRACE; break;
        case ';': token.tipo = TOK_SEMICOLON; break;
        case '=': token.tipo = TOK_ASSIGN; break;
        case '<': token.tipo = TOK_LT; break;
        case '>': token.tipo = TOK_GT; break;
        case '!': token.tipo = TOK_INVALID; break;
        default:  token.tipo = TOK_INVALID; break;
    }

    if ((caractere == '=' || caractere == '<' ||
         caractere == '>' || caractere == '!') && olharCaractere(codigo) == '=') {
        lerCaractere(codigo);
        adicionarCaractere(&token, &tamanho, '=');
        if      (caractere == '=') token.tipo = TOK_EQ;
        else if (caractere == '<') token.tipo = TOK_LE;
        else if (caractere == '>') token.tipo = TOK_GE;
        else                       token.tipo = TOK_NE;
    }

    if (token.tipo == TOK_INVALID) {
        fprintf(stderr, "Erro lexico na linha %d: caractere invalido '%s'.\n",
                token.linha, token.lexema);
    }
    return token;
}


typedef struct {
    CodigoFonte *codigo;
    Token tokenAtual;
} Parser;

void avancarEntrada(Parser *parser)
{
    parser->tokenAtual = proximoToken(parser->codigo);
}

void casar(Parser *parser, TokenType esperado)
{
    if (parser->tokenAtual.tipo == esperado) {
        avancarEntrada(parser);
    } else {
        erroSintatico(parser->tokenAtual.linha, esperado,
                      parser->tokenAtual.tipo);
    }
}

int analisarPrograma(Parser *parser)
{
    /* TODO: implementar a gramatica da Csmall por descida recursiva. */
    (void)parser;
    return 0;
}

typedef enum {
    TIPO_INT,
    TIPO_FLOAT,
    TIPO_ERRO
} TipoDado;

typedef struct EntradaSimbolo {
    char lexema[TAM_LEXEMA];
    TipoDado tipo;
    int linha;
    struct EntradaSimbolo *prox;
} EntradaSimbolo;

typedef struct TabelaSimbolos {
    EntradaSimbolo *entradas;
    struct TabelaSimbolos *anterior;
} TabelaSimbolos;

int inserirSimbolo(TabelaSimbolos *tabela, const char *lexema,
                   TipoDado tipo, int linha);
EntradaSimbolo *buscarSimbolo(TabelaSimbolos *tabela, const char *lexema);


int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Uso: %s arquivo.csmall\n", argv[0]);
        return EXIT_FAILURE;
    }

    CodigoFonte codigo;
    if (!abrirCodigoFonte(&codigo, argv[1])) {
        return EXIT_FAILURE;
    }

    printf("%-14s %-20s %s\n", "TOKEN", "LEXEMA", "LINHA");
    printf("-----------------------------------------------\n");

    Token token;
    do {
        token = proximoToken(&codigo);
        imprimirToken(token);
    } while (token.tipo != TOK_EOF && token.tipo != TOK_INVALID);

    int valido = (token.tipo != TOK_INVALID);
    fecharCodigoFonte(&codigo);
    return valido ? EXIT_SUCCESS : EXIT_FAILURE;
}