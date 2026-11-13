/*
 * Compilar: gcc -std=c11 -Wall -Wextra -o csmall csmall.c
 * Executar: ./csmall arquivo.csmall
 * Tokens:   ./csmall --tokens arquivo.csmall
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define TAM_LEXEMA 128

typedef enum {
    TOK_INT, TOK_FLOAT, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_PRINT,
    TOK_IDENTIFIER, TOK_INT_CONST, TOK_FLOAT_CONST,
    TOK_PLUS, TOK_MINUS, TOK_MULT, TOK_DIV,
    TOK_ASSIGN, TOK_LT, TOK_LE, TOK_GT, TOK_GE, TOK_EQ, TOK_NE,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE, TOK_SEMICOLON,
    TOK_EOF, TOK_INVALID
} TokenType;

typedef struct { TokenType tipo; char lexema[TAM_LEXEMA]; int linha; } Token;

const char *nomeToken(TokenType tipo) {
    static const char *nomes[] = {
        "INT", "FLOAT", "IF", "ELSE", "WHILE", "PRINT", "IDENTIFIER",
        "INT_CONST", "FLOAT_CONST", "PLUS", "MINUS", "MULT", "DIV",
        "ASSIGN", "LT", "LE", "GT", "GE", "EQ", "NE", "LPAREN",
        "RPAREN", "LBRACE", "RBRACE", "SEMICOLON", "EOF", "INVALID"
    };
    if (tipo < TOK_INT || tipo > TOK_INVALID) return "UNKNOWN";
    return nomes[tipo];
}

void imprimirToken(Token t) {
    printf("%-14s %-20s linha %d\n", nomeToken(t.tipo), t.lexema, t.linha);
}

typedef struct { FILE *arquivo; int linha; } CodigoFonte;
int abrirCodigoFonte(CodigoFonte *c, const char *nome) {
    c->arquivo = fopen(nome, "r"); c->linha = 1;
    if (!c->arquivo) { fprintf(stderr, "Nao foi possivel abrir: %s\n", nome); return 0; }
    return 1;
}
void fecharCodigoFonte(CodigoFonte *c) { if (c->arquivo) { fclose(c->arquivo); c->arquivo = NULL; } }
int lerCaractere(CodigoFonte *c) { int ch = fgetc(c->arquivo); if (ch == '\n') c->linha++; return ch; }
void devolverCaractere(CodigoFonte *c, int ch) { if (ch != EOF) { ungetc(ch, c->arquivo); if (ch == '\n') c->linha--; } }
int olharCaractere(CodigoFonte *c) { int ch = lerCaractere(c); devolverCaractere(c, ch); return ch; }

void erroLexico(int linha) { fprintf(stderr, "Erro lexico na linha %d.\n", linha); }
void erroSintatico(int linha, TokenType esp, TokenType ach) {
    fprintf(stderr, "Erro sintatico na linha %d: esperado %s, encontrado %s.\n", linha, nomeToken(esp), nomeToken(ach));
}
void erroSemantico(int linha, const char *msg) { fprintf(stderr, "Erro semantico na linha %d: %s\n", linha, msg); }

/* O lexer percorre o arquivo e entrega um token por vez. */
Token proximoToken(CodigoFonte *c) {
    Token t = {TOK_EOF, "", c->linha};
    int ch;
    /* Ignora espaços e comentarios de linha. */
    for (;;) {
        ch = lerCaractere(c);
        if (ch == EOF) { t.linha = c->linha; return t; }
        if (isspace((unsigned char)ch)) continue;
        if (ch == '/' && olharCaractere(c) == '/') {
            lerCaractere(c);
            while ((ch = lerCaractere(c)) != EOF && ch != '\n') { }
            continue;
        }
        break;
    }
    t.linha = c->linha;
    size_t n = 0;
    if (isalpha((unsigned char)ch) || ch == '_') {
        do {
            if (n >= TAM_LEXEMA - 1) { erroLexico(t.linha); t.tipo = TOK_INVALID; return t; }
            t.lexema[n++] = (char)ch;
            ch = lerCaractere(c);
        } while (ch != EOF && (isalnum((unsigned char)ch) || ch == '_'));
        devolverCaractere(c, ch);
        t.lexema[n] = '\0';
        struct { const char *palavra; TokenType tipo; } palavras[] = {
            {"int", TOK_INT}, {"float", TOK_FLOAT}, {"if", TOK_IF},
            {"else", TOK_ELSE}, {"while", TOK_WHILE}, {"print", TOK_PRINT}
        };
        t.tipo = TOK_IDENTIFIER;
        for (size_t i = 0; i < sizeof(palavras) / sizeof(palavras[0]); i++)
            if (!strcmp(t.lexema, palavras[i].palavra)) t.tipo = palavras[i].tipo;
        return t;
    }
    if (isdigit((unsigned char)ch)) {
        do {
            if (n >= TAM_LEXEMA - 1) { erroLexico(t.linha); t.tipo = TOK_INVALID; return t; }
            t.lexema[n++] = (char)ch;
            ch = lerCaractere(c);
        } while (ch != EOF && isdigit((unsigned char)ch));
        t.tipo = TOK_INT_CONST;
        if (ch == '.') {
            if (!isdigit((unsigned char)olharCaractere(c))) {
                erroLexico(t.linha); t.tipo = TOK_INVALID; return t;
            }
            if (n >= TAM_LEXEMA - 1) { erroLexico(t.linha); t.tipo = TOK_INVALID; return t; }
            t.lexema[n++] = '.';
            ch = lerCaractere(c);
            do {
                if (n >= TAM_LEXEMA - 1) { erroLexico(t.linha); t.tipo = TOK_INVALID; return t; }
                t.lexema[n++] = (char)ch;
                ch = lerCaractere(c);
            } while (ch != EOF && isdigit((unsigned char)ch));
            t.tipo = TOK_FLOAT_CONST;
        }
        devolverCaractere(c, ch);
        t.lexema[n] = '\0';
        return t;
    }
    t.lexema[0] = (char)ch;
    t.lexema[1] = '\0';
    switch (ch) {
        case '+': t.tipo = TOK_PLUS; break;
        case '-': t.tipo = TOK_MINUS; break;
        case '*': t.tipo = TOK_MULT; break;
        case '/': t.tipo = TOK_DIV; break;
        case '(': t.tipo = TOK_LPAREN; break;
        case ')': t.tipo = TOK_RPAREN; break;
        case '{': t.tipo = TOK_LBRACE; break;
        case '}': t.tipo = TOK_RBRACE; break;
        case ';': t.tipo = TOK_SEMICOLON; break;
        case '=': t.tipo = TOK_ASSIGN; break;
        case '<': t.tipo = TOK_LT; break;
        case '>': t.tipo = TOK_GT; break;
        case '!': t.tipo = TOK_INVALID; break;
        default: t.tipo = TOK_INVALID; break;
    }
    if ((ch == '=' || ch == '<' || ch == '>' || ch == '!') && olharCaractere(c) == '=') {
        lerCaractere(c);
        t.lexema[1] = '='; t.lexema[2] = '\0';
        t.tipo = ch == '=' ? TOK_EQ : ch == '<' ? TOK_LE : ch == '>' ? TOK_GE : TOK_NE;
    }
    if (t.tipo == TOK_INVALID) erroLexico(t.linha);
    return t;
}

/* Tabela de simbolos e controle de escopos. */
typedef enum { TIPO_INT, TIPO_FLOAT, TIPO_ERRO } TipoDado;
typedef struct EntradaSimbolo {
    char lexema[TAM_LEXEMA]; TipoDado tipo; int linha;
    struct EntradaSimbolo *prox;
} EntradaSimbolo;
typedef struct TabelaSimbolos {
    EntradaSimbolo *entradas;
    struct TabelaSimbolos *anterior;
} TabelaSimbolos;

EntradaSimbolo *buscarNoEscopo(TabelaSimbolos *t, const char *nome) {
    for (EntradaSimbolo *e = t->entradas; e; e = e->prox)
        if (strcmp(e->lexema, nome) == 0) return e;
    return NULL;
}
int inserirSimbolo(TabelaSimbolos *t, const char *nome, TipoDado tipo, int linha) {
    if (buscarNoEscopo(t, nome)) return 0;
    EntradaSimbolo *e = malloc(sizeof(*e));
    if (!e) { fprintf(stderr, "Memoria insuficiente\n"); exit(EXIT_FAILURE); }
    strcpy(e->lexema, nome); e->tipo = tipo; e->linha = linha;
    e->prox = t->entradas; t->entradas = e;
    return 1;
}
EntradaSimbolo *buscarSimbolo(TabelaSimbolos *t, const char *nome) {
    for (; t; t = t->anterior) {
        EntradaSimbolo *e = buscarNoEscopo(t, nome);
        if (e) return e;
    }
    return NULL;
}
TabelaSimbolos *abrirEscopo(TabelaSimbolos *anterior) {
    TabelaSimbolos *t = malloc(sizeof(*t));
    if (!t) { fprintf(stderr, "Memoria insuficiente\n"); exit(EXIT_FAILURE); }
    t->entradas = NULL; t->anterior = anterior; return t;
}
TabelaSimbolos *fecharEscopo(TabelaSimbolos *t) {
    TabelaSimbolos *pai = t->anterior;
    EntradaSimbolo *e = t->entradas;
    while (e) { EntradaSimbolo *prox = e->prox; free(e); e = prox; }
    free(t); return pai;
}

/* Parser por descida recursiva e geracao TAC em uma passagem. */
typedef struct {
    TipoDado tipo;
    char lugar[TAM_LEXEMA];
} Resultado;

typedef struct {
    CodigoFonte *codigo;
    Token tokenAtual;
    TabelaSimbolos *tabela;
    FILE *saida;
    int proximoTemporario;
    int proximoRotulo;
} Parser;

void emitir(Parser *p, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(p->saida, fmt, args);
    va_end(args);
    fputc('\n', p->saida);
}

void novoTemporario(Parser *p, char *destino, size_t capacidade) {
    snprintf(destino, capacidade, "t%d", ++p->proximoTemporario);
}
void novoRotulo(Parser *p, char *destino, size_t capacidade) {
    snprintf(destino, capacidade, "L%d", ++p->proximoRotulo);
}

void avancarEntrada(Parser *p) {
    p->tokenAtual = proximoToken(p->codigo);
    if (p->tokenAtual.tipo == TOK_INVALID) exit(EXIT_FAILURE);
}
void casar(Parser *p, TokenType esperado) {
    if (p->tokenAtual.tipo != esperado) {
        erroSintatico(p->tokenAtual.linha, esperado, p->tokenAtual.tipo);
        exit(EXIT_FAILURE);
    }
    avancarEntrada(p);
}
void sintaxeInesperada(Parser *p, const char *esperado) {
    fprintf(stderr, "Erro sintatico na linha %d: esperado %s, encontrado %s ('%s').\n",
            p->tokenAtual.linha, esperado, nomeToken(p->tokenAtual.tipo), p->tokenAtual.lexema);
    exit(EXIT_FAILURE);
}
void semanticaInvalida(int linha, const char *motivo, const char *nome) {
    char mensagem[256];
    snprintf(mensagem, sizeof(mensagem), "%s: '%s'", motivo, nome);
    erroSemantico(linha, mensagem);
    exit(EXIT_FAILURE);
}
int ehTipo(TokenType t) { return t == TOK_INT || t == TOK_FLOAT; }
int ehComando(TokenType t) {
    return t == TOK_IDENTIFIER || t == TOK_PRINT || t == TOK_IF ||
           t == TOK_WHILE || t == TOK_LBRACE;
}

Resultado expressao(Parser *p);

void declaracao(Parser *p) {
    TipoDado tipo = p->tokenAtual.tipo == TOK_INT ? TIPO_INT : TIPO_FLOAT;
    avancarEntrada(p);
    if (p->tokenAtual.tipo != TOK_IDENTIFIER) sintaxeInesperada(p, "identificador");
    Token id = p->tokenAtual;
    casar(p, TOK_IDENTIFIER);
    if (!inserirSimbolo(p->tabela, id.lexema, tipo, id.linha))
        semanticaInvalida(id.linha, "variavel redeclarada no mesmo escopo", id.lexema);
    /* A Pratica 1 aceitava apenas 'int a;'. A entrada desta pratica usa 'int a = 5;'. */
    if (p->tokenAtual.tipo == TOK_ASSIGN) {
        casar(p, TOK_ASSIGN);
        Resultado r = expressao(p);
        if (tipo != r.tipo)
            semanticaInvalida(id.linha, "inicializacao com tipos incompativeis", id.lexema);
        emitir(p, "%s = %s", id.lexema, r.lugar);
    }
    casar(p, TOK_SEMICOLON);
}
void listaDecl(Parser *p) { while (ehTipo(p->tokenAtual.tipo)) declaracao(p); }

Resultado fator(Parser *p) {
    Token t = p->tokenAtual;
    Resultado r;
    if (t.tipo == TOK_INT_CONST || t.tipo == TOK_FLOAT_CONST) {
        r.tipo = t.tipo == TOK_INT_CONST ? TIPO_INT : TIPO_FLOAT;
        strcpy(r.lugar, t.lexema);
        avancarEntrada(p);
        return r;
    }
    if (t.tipo == TOK_IDENTIFIER) {
        EntradaSimbolo *e = buscarSimbolo(p->tabela, t.lexema);
        if (!e) semanticaInvalida(t.linha, "variavel nao declarada", t.lexema);
        r.tipo = e->tipo;
        strcpy(r.lugar, t.lexema);
        avancarEntrada(p);
        return r;
    }
    if (t.tipo == TOK_LPAREN) {
        casar(p, TOK_LPAREN);
        r = expressao(p);
        casar(p, TOK_RPAREN);
        return r;
    }
    sintaxeInesperada(p, "identificador, numero ou expressao entre parenteses");
    r.tipo = TIPO_ERRO;
    r.lugar[0] = '\0';
    return r;
}
TipoDado combinarTipos(TipoDado a, TipoDado b) {
    return a == TIPO_FLOAT || b == TIPO_FLOAT ? TIPO_FLOAT : TIPO_INT;
}
Resultado termo(Parser *p) {
    Resultado esquerda = fator(p);
    while (p->tokenAtual.tipo == TOK_MULT || p->tokenAtual.tipo == TOK_DIV) {
        char op = p->tokenAtual.tipo == TOK_MULT ? '*' : '/';
        avancarEntrada(p);
        Resultado direita = fator(p);
        Resultado r;
        r.tipo = combinarTipos(esquerda.tipo, direita.tipo);
        novoTemporario(p, r.lugar, sizeof(r.lugar));
        emitir(p, "%s = %s %c %s", r.lugar, esquerda.lugar, op, direita.lugar);
        esquerda = r;
    }
    return esquerda;
}
Resultado expressao(Parser *p) {
    Resultado esquerda = termo(p);
    while (p->tokenAtual.tipo == TOK_PLUS || p->tokenAtual.tipo == TOK_MINUS) {
        char op = p->tokenAtual.tipo == TOK_PLUS ? '+' : '-';
        avancarEntrada(p);
        Resultado direita = termo(p);
        Resultado r;
        r.tipo = combinarTipos(esquerda.tipo, direita.tipo);
        novoTemporario(p, r.lugar, sizeof(r.lugar));
        emitir(p, "%s = %s %c %s", r.lugar, esquerda.lugar, op, direita.lugar);
        esquerda = r;
    }
    return esquerda;
}
void condicao(Parser *p, const char *verdadeiro, const char *falso) {
    Resultado esquerda = expressao(p);
    Token rel = p->tokenAtual;
    switch (rel.tipo) {
        case TOK_LT: case TOK_LE: case TOK_GT:
        case TOK_GE: case TOK_EQ: case TOK_NE: avancarEntrada(p); break;
        default: sintaxeInesperada(p, "operador relacional");
    }
    Resultado direita = expressao(p);
    emitir(p, "if %s %s %s goto %s", esquerda.lugar, rel.lexema, direita.lugar, verdadeiro);
    emitir(p, "goto %s", falso);
}
void bloco(Parser *p);
void comando(Parser *p) {
    if (p->tokenAtual.tipo == TOK_IDENTIFIER) {
        Token id = p->tokenAtual;
        EntradaSimbolo *e = buscarSimbolo(p->tabela, id.lexema);
        if (!e) semanticaInvalida(id.linha, "variavel nao declarada", id.lexema);
        casar(p, TOK_IDENTIFIER);
        casar(p, TOK_ASSIGN);
        Resultado rhs = expressao(p);
        if (e->tipo != rhs.tipo) semanticaInvalida(id.linha, "atribuicao com tipos incompativeis", id.lexema);
        casar(p, TOK_SEMICOLON);
        emitir(p, "%s = %s", id.lexema, rhs.lugar);
    } else if (p->tokenAtual.tipo == TOK_PRINT) {
        casar(p, TOK_PRINT); casar(p, TOK_LPAREN);
        Resultado r = expressao(p);
        casar(p, TOK_RPAREN); casar(p, TOK_SEMICOLON);
        emitir(p, "print %s", r.lugar);
    } else if (p->tokenAtual.tipo == TOK_IF) {
        char verdadeiro[32], falso[32], fim[32];
        novoRotulo(p, verdadeiro, sizeof(verdadeiro));
        novoRotulo(p, falso, sizeof(falso));
        novoRotulo(p, fim, sizeof(fim));
        casar(p, TOK_IF); casar(p, TOK_LPAREN);
        condicao(p, verdadeiro, falso); casar(p, TOK_RPAREN);
        emitir(p, "%s:", verdadeiro);
        bloco(p);
        emitir(p, "goto %s", fim);
        emitir(p, "%s:", falso);
        if (p->tokenAtual.tipo == TOK_ELSE) { casar(p, TOK_ELSE); bloco(p); }
        emitir(p, "%s:", fim);
    } else if (p->tokenAtual.tipo == TOK_WHILE) {
        char teste[32], corpo[32], saida[32];
        novoRotulo(p, teste, sizeof(teste));
        novoRotulo(p, corpo, sizeof(corpo));
        novoRotulo(p, saida, sizeof(saida));
        casar(p, TOK_WHILE); casar(p, TOK_LPAREN);
        emitir(p, "%s:", teste);
        condicao(p, corpo, saida); casar(p, TOK_RPAREN);
        emitir(p, "%s:", corpo);
        bloco(p);
        emitir(p, "goto %s", teste);
        emitir(p, "%s:", saida);
    } else if (p->tokenAtual.tipo == TOK_LBRACE) {
        bloco(p);
    } else {
        sintaxeInesperada(p, "comando");
    }
}
void listaCmd(Parser *p) { while (ehComando(p->tokenAtual.tipo)) comando(p); }
void bloco(Parser *p) {
    casar(p, TOK_LBRACE);
    p->tabela = abrirEscopo(p->tabela);
    listaDecl(p);
    listaCmd(p);
    casar(p, TOK_RBRACE);
    p->tabela = fecharEscopo(p->tabela);
}
int analisarPrograma(Parser *p) {
    p->proximoTemporario = 0;
    p->proximoRotulo = 0;
    p->tabela = abrirEscopo(NULL);
    avancarEntrada(p);
    listaDecl(p);
    listaCmd(p);
    casar(p, TOK_EOF);
    p->tabela = fecharEscopo(p->tabela);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Uso: %s [--tokens] arquivo.csmall [saida.tac]\n", argv[0]);
        return EXIT_FAILURE;
    }
    int apenasTokens = argc == 3 && strcmp(argv[1], "--tokens") == 0;
    const char *nome = apenasTokens ? argv[2] : argv[1];
    CodigoFonte codigo;
    if (!abrirCodigoFonte(&codigo, nome)) return EXIT_FAILURE;
    if (apenasTokens) {
        printf("%-14s %-20s %s\n", "TOKEN", "LEXEMA", "LINHA");
        Token t;
        do { t = proximoToken(&codigo); imprimirToken(t); }
        while (t.tipo != TOK_EOF && t.tipo != TOK_INVALID);
        fecharCodigoFonte(&codigo);
        return t.tipo == TOK_INVALID ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    FILE *saida = stdout;
    if (argc == 3) {
        saida = fopen(argv[2], "w");
        if (!saida) { perror("Erro abrindo arquivo TAC"); fecharCodigoFonte(&codigo); return EXIT_FAILURE; }
    }
    Parser parser = { .codigo = &codigo, .saida = saida };
    int sucesso = analisarPrograma(&parser);
    fecharCodigoFonte(&codigo);
    if (saida != stdout) fclose(saida);
    return sucesso ? EXIT_SUCCESS : EXIT_FAILURE;
}
