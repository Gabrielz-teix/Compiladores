#include "lexico.h"

#include <stdlib.h>
#include <string.h>

#define FIM_ENTRADA (-1)

/* ------------------------------------------------------------------ */
/* Alfabeto do AFD: cada caractere da entrada pertence a uma classe.   */
/* ------------------------------------------------------------------ */
typedef enum {
    C_LETRA, C_DIGITO, C_UNDERLINE, C_PONTO, C_ASPAS_SIMPLES, C_ASPAS_DUPLAS,
    C_IGUAL, C_EXCLAMACAO, C_MENOR, C_MAIOR, C_MENOS, C_MAIS, C_ASTERISCO,
    C_BARRA, C_ABRE_PAR, C_FECHA_PAR, C_ABRE_CHAVE, C_FECHA_CHAVE,
    C_DOIS_PONTOS, C_PONTO_VIRGULA, C_VIRGULA, C_ESPACO, C_NOVA_LINHA,
    C_OUTRO, C_FIM,
    NUM_CLASSES
} ClasseCaractere;

static const char *nomesClasses[NUM_CLASSES] = {
    "letra", "digito", "_", ".", "'", "\"", "=", "!", "<", ">", "-", "+", "*",
    "/", "(", ")", "{", "}", ":", ";", ",", "espaco", "\\n", "outro", "EOF"
};

/* ------------------------------------------------------------------ */
/* Estados do AFD (q0 = INICIAL).                                      */
/* ------------------------------------------------------------------ */
typedef enum {
    E_INICIAL, E_ID, E_INT, E_FLOAT_PONTO, E_FLOAT,
    E_CHAR_ABRE, E_CHAR_CONTEUDO, E_CHAR,
    E_STR_ABRE, E_STR_CONTEUDO, E_STR,
    E_ASSIGN, E_EQ, E_EXCLAMACAO, E_NE, E_LT, E_LE, E_GT, E_GE,
    E_MINUS, E_ARROW, E_PLUS, E_MULT, E_DIV,
    E_LBRACKET, E_RBRACKET, E_LBRACE, E_RBRACE, E_COLON, E_SEMICOLON, E_COMMA,
    NUM_ESTADOS
} Estado;

#define E_ERRO (-1) /* ausencia de transicao */

static const char *nomesEstados[NUM_ESTADOS] = {
    "INICIAL", "ID", "INT", "FLOAT_PONTO", "FLOAT",
    "CHAR_ABRE", "CHAR_CONTEUDO", "CHAR",
    "STR_ABRE", "STR_CONTEUDO", "STR",
    "ASSIGN", "EQ", "EXCLAMACAO", "NE", "LT", "LE", "GT", "GE",
    "MINUS", "ARROW", "PLUS", "MULT", "DIV",
    "LBRACKET", "RBRACKET", "LBRACE", "RBRACE", "COLON", "SEMICOLON", "COMMA"
};

static int transicao[NUM_ESTADOS][NUM_CLASSES]; /* proximo estado ou E_ERRO */
static int aceitacao[NUM_ESTADOS];              /* token reconhecido ou -1  */
static int afdConstruido = 0;

static void ligar(Estado de, ClasseCaractere classe, Estado para)
{
    transicao[de][classe] = (int)para;
}

static void aceitar(Estado estado, TipoToken token)
{
    aceitacao[estado] = (int)token;
}

static void construirAFD(void)
{
    if (afdConstruido) return;
    for (int e = 0; e < NUM_ESTADOS; e++) {
        aceitacao[e] = -1;
        for (int c = 0; c < NUM_CLASSES; c++) transicao[e][c] = E_ERRO;
    }

    /* Transicoes a partir do estado inicial. */
    ligar(E_INICIAL, C_LETRA, E_ID);
    ligar(E_INICIAL, C_DIGITO, E_INT);
    ligar(E_INICIAL, C_ASPAS_SIMPLES, E_CHAR_ABRE);
    ligar(E_INICIAL, C_ASPAS_DUPLAS, E_STR_ABRE);
    ligar(E_INICIAL, C_IGUAL, E_ASSIGN);
    ligar(E_INICIAL, C_EXCLAMACAO, E_EXCLAMACAO);
    ligar(E_INICIAL, C_MENOR, E_LT);
    ligar(E_INICIAL, C_MAIOR, E_GT);
    ligar(E_INICIAL, C_MENOS, E_MINUS);
    ligar(E_INICIAL, C_MAIS, E_PLUS);
    ligar(E_INICIAL, C_ASTERISCO, E_MULT);
    ligar(E_INICIAL, C_BARRA, E_DIV);
    ligar(E_INICIAL, C_ABRE_PAR, E_LBRACKET);
    ligar(E_INICIAL, C_FECHA_PAR, E_RBRACKET);
    ligar(E_INICIAL, C_ABRE_CHAVE, E_LBRACE);
    ligar(E_INICIAL, C_FECHA_CHAVE, E_RBRACE);
    ligar(E_INICIAL, C_DOIS_PONTOS, E_COLON);
    ligar(E_INICIAL, C_PONTO_VIRGULA, E_SEMICOLON);
    ligar(E_INICIAL, C_VIRGULA, E_COMMA);

    /* Identificador: [a-zA-Z]([a-zA-Z0-9_])* */
    ligar(E_ID, C_LETRA, E_ID);
    ligar(E_ID, C_DIGITO, E_ID);
    ligar(E_ID, C_UNDERLINE, E_ID);

    /* Inteiro [0-9]+ e float [0-9]+.[0-9]+ */
    ligar(E_INT, C_DIGITO, E_INT);
    ligar(E_INT, C_PONTO, E_FLOAT_PONTO);
    ligar(E_FLOAT_PONTO, C_DIGITO, E_FLOAT);
    ligar(E_FLOAT, C_DIGITO, E_FLOAT);

    /* Literal char: ' qualquer ' e string de formatacao: " qualquer+ ".
       O conteudo nao pode conter quebra de linha nem o fim da entrada. */
    for (int c = 0; c < NUM_CLASSES; c++) {
        if (c == C_NOVA_LINHA || c == C_FIM) continue;
        if (c != C_ASPAS_SIMPLES) ligar(E_CHAR_ABRE, (ClasseCaractere)c, E_CHAR_CONTEUDO);
        if (c != C_ASPAS_DUPLAS) {
            ligar(E_STR_ABRE, (ClasseCaractere)c, E_STR_CONTEUDO);
            ligar(E_STR_CONTEUDO, (ClasseCaractere)c, E_STR_CONTEUDO);
        }
    }
    ligar(E_CHAR_CONTEUDO, C_ASPAS_SIMPLES, E_CHAR);
    ligar(E_STR_CONTEUDO, C_ASPAS_DUPLAS, E_STR);

    /* Operadores de dois caracteres. */
    ligar(E_ASSIGN, C_IGUAL, E_EQ);
    ligar(E_EXCLAMACAO, C_IGUAL, E_NE);
    ligar(E_LT, C_IGUAL, E_LE);
    ligar(E_GT, C_IGUAL, E_GE);
    ligar(E_MINUS, C_MAIOR, E_ARROW);

    /* Estados de aceitacao. */
    aceitar(E_ID, TK_ID);
    aceitar(E_INT, TK_INT_CONST);
    aceitar(E_FLOAT, TK_FLOAT_CONST);
    aceitar(E_CHAR, TK_CHAR_LITERAL);
    aceitar(E_STR, TK_FMT_STRING);
    aceitar(E_ASSIGN, TK_ASSIGN);
    aceitar(E_EQ, TK_EQ);
    aceitar(E_NE, TK_NE);
    aceitar(E_LT, TK_LT);
    aceitar(E_LE, TK_LE);
    aceitar(E_GT, TK_GT);
    aceitar(E_GE, TK_GE);
    aceitar(E_MINUS, TK_MINUS);
    aceitar(E_ARROW, TK_ARROW);
    aceitar(E_PLUS, TK_PLUS);
    aceitar(E_MULT, TK_MULT);
    aceitar(E_DIV, TK_DIV);
    aceitar(E_LBRACKET, TK_LBRACKET);
    aceitar(E_RBRACKET, TK_RBRACKET);
    aceitar(E_LBRACE, TK_LBRACE);
    aceitar(E_RBRACE, TK_RBRACE);
    aceitar(E_COLON, TK_COLON);
    aceitar(E_SEMICOLON, TK_SEMICOLON);
    aceitar(E_COMMA, TK_COMMA);

    afdConstruido = 1;
}

static ClasseCaractere classificar(int c)
{
    if (c == FIM_ENTRADA) return C_FIM;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return C_LETRA;
    if (c >= '0' && c <= '9') return C_DIGITO;
    switch (c) {
        case '_': return C_UNDERLINE;
        case '.': return C_PONTO;
        case '\'': return C_ASPAS_SIMPLES;
        case '"': return C_ASPAS_DUPLAS;
        case '=': return C_IGUAL;
        case '!': return C_EXCLAMACAO;
        case '<': return C_MENOR;
        case '>': return C_MAIOR;
        case '-': return C_MENOS;
        case '+': return C_MAIS;
        case '*': return C_ASTERISCO;
        case '/': return C_BARRA;
        case '(': return C_ABRE_PAR;
        case ')': return C_FECHA_PAR;
        case '{': return C_ABRE_CHAVE;
        case '}': return C_FECHA_CHAVE;
        case ':': return C_DOIS_PONTOS;
        case ';': return C_PONTO_VIRGULA;
        case ',': return C_VIRGULA;
        case ' ': case '\t': case '\r': case '\v': case '\f': return C_ESPACO;
        case '\n': return C_NOVA_LINHA;
        default: return C_OUTRO;
    }
}

/* ------------------------------------------------------------------ */
/* Tokens                                                              */
/* ------------------------------------------------------------------ */
static const char *nomesTokens[] = {
    "FUNCTION", "MAIN", "LET", "INT", "FLOAT", "CHAR", "IF", "ELSE",
    "WHILE", "PRINTLN", "RETURN",
    "LBRACKET", "RBRACKET", "LBRACE", "RBRACE", "ARROW", "COLON",
    "SEMICOLON", "COMMA",
    "ASSIGN", "EQ", "NE", "GT", "GE", "LT", "LE",
    "PLUS", "MINUS", "MULT", "DIV",
    "ID", "INT_CONST", "FLOAT_CONST", "CHAR_LITERAL", "FMT_STRING",
    "EOF"
};

const char *nomeToken(TipoToken tipo)
{
    if ((int)tipo < 0 || tipo > TK_EOF) return "DESCONHECIDO";
    return nomesTokens[tipo];
}

static const struct {
    const char *palavra;
    TipoToken token;
} reservadas[] = {
    {"fn", TK_FUNCTION}, {"main", TK_MAIN}, {"let", TK_LET}, {"int", TK_INT},
    {"float", TK_FLOAT}, {"char", TK_CHAR}, {"if", TK_IF}, {"else", TK_ELSE},
    {"while", TK_WHILE}, {"println", TK_PRINTLN}, {"return", TK_RETURN}
};

/* Um lexema aceito no estado ID pode ser uma palavra reservada. */
static TipoToken classificarIdentificador(const char *inicio, size_t tamanho)
{
    for (size_t i = 0; i < sizeof(reservadas) / sizeof(reservadas[0]); i++) {
        if (strlen(reservadas[i].palavra) == tamanho &&
            strncmp(reservadas[i].palavra, inicio, tamanho) == 0)
            return reservadas[i].token;
    }
    return TK_ID;
}

static void adicionarToken(VetorTokens *tokens, const char *inicio, size_t tamanho,
                           TipoToken tipo, int linha)
{
    GARANTIR_CAPACIDADE(tokens->itens, tokens->quantidade, tokens->capacidade);
    Token *t = &tokens->itens[tokens->quantidade++];
    t->lexema = copiarTrecho(inicio, tamanho);
    t->tipo = tipo;
    t->linha = linha;
}

/* ------------------------------------------------------------------ */
/* Tratamento de erros lexicos                                         */
/* ------------------------------------------------------------------ */

/* Chamado quando o AFD para em um estado que nao e de aceitacao.
   `inicio` e a posicao do primeiro caractere do lexema e `parada` a
   posicao do caractere sem transicao. Registra o erro e devolve a
   posicao em que a analise deve continuar (sempre maior que `inicio`). */
static size_t tratarErroLexico(const char *fonte, size_t tamanho, size_t inicio,
                               size_t parada, int estado, int linha, ListaErros *erros)
{
    char seguinte = parada < tamanho ? fonte[parada] : '\0';
    int lido = (int)(parada - inicio);

    switch (estado) {
        case E_INICIAL: {
            /* Caractere fora do alfabeto. Um caractere UTF-8 de varios bytes
               e tratado como um unico erro. */
            size_t n = 1;
            while (inicio + n < tamanho && ((unsigned char)fonte[inicio + n] & 0xC0) == 0x80) n++;
            registrarErro(erros, linha, "caractere invalido '%.*s'", (int)n, fonte + inicio);
            return inicio + n;
        }
        case E_FLOAT_PONTO:
            registrarErro(erros, linha, "constante float mal formada '%.*s': falta digito apos o ponto",
                          lido, fonte + inicio);
            return parada;
        case E_EXCLAMACAO:
            registrarErro(erros, linha, "caractere '!' isolado (o unico operador com '!' e '!=')");
            return parada;
        case E_CHAR_ABRE:
            if (seguinte == '\'') {
                registrarErro(erros, linha, "literal de caractere vazio ''");
                return parada + 1;
            }
            registrarErro(erros, linha, "literal de caractere nao terminado");
            return parada;
        case E_CHAR_CONTEUDO: {
            /* Se houver aspa de fechamento na mesma linha, descarta o
               literal inteiro para nao gerar erros em cascata. */
            size_t j = parada;
            while (j < tamanho && fonte[j] != '\n' && fonte[j] != '\'') j++;
            if (j < tamanho && fonte[j] == '\'') {
                registrarErro(erros, linha, "literal de caractere com mais de um caractere: %.*s",
                              (int)(j + 1 - inicio), fonte + inicio);
                return j + 1;
            }
            registrarErro(erros, linha, "literal de caractere nao terminado: %.*s", lido, fonte + inicio);
            return parada;
        }
        case E_STR_ABRE:
            if (seguinte == '"') {
                registrarErro(erros, linha, "string de formatacao vazia \"\"");
                return parada + 1;
            }
            registrarErro(erros, linha, "string de formatacao nao terminada");
            return parada;
        case E_STR_CONTEUDO: {
            int n = lido;
            while (n > 0 && fonte[inicio + (size_t)n - 1] == '\r') n--;
            registrarErro(erros, linha, "string de formatacao nao terminada: %.*s", n, fonte + inicio);
            return parada;
        }
        default:
            registrarErro(erros, linha, "lexema invalido '%.*s'", lido, fonte + inicio);
            return parada > inicio ? parada : inicio + 1;
    }
}

/* ------------------------------------------------------------------ */
/* Analise lexica guiada pelo AFD                                      */
/* ------------------------------------------------------------------ */
void analisarLexico(const char *fonte, size_t tamanho, VetorTokens *tokens, ListaErros *erros)
{
    construirAFD();
    size_t pos = 0;
    int linha = 1;

    for (;;) {
        /* Espacos e quebras de linha separam lexemas e sao descartados
           antes de iniciar o AFD em q0. */
        while (pos < tamanho) {
            ClasseCaractere c = classificar((unsigned char)fonte[pos]);
            if (c == C_NOVA_LINHA) linha++;
            else if (c != C_ESPACO) break;
            pos++;
        }
        if (pos >= tamanho) break;

        /* Executa o AFD ate nao haver transicao (lexema mais longo). */
        int estado = E_INICIAL;
        size_t i = pos;
        for (;;) {
            int c = i < tamanho ? (unsigned char)fonte[i] : FIM_ENTRADA;
            int proximo = transicao[estado][classificar(c)];
            if (proximo == E_ERRO) break;
            estado = proximo;
            i++;
        }

        if (aceitacao[estado] >= 0) {
            TipoToken tipo = (TipoToken)aceitacao[estado];
            if (tipo == TK_ID) tipo = classificarIdentificador(fonte + pos, i - pos);
            adicionarToken(tokens, fonte + pos, i - pos, tipo, linha);
            pos = i;
        } else {
            pos = tratarErroLexico(fonte, tamanho, pos, i, estado, linha, erros);
        }
    }
    adicionarToken(tokens, "", 0, TK_EOF, linha);
}

/* ------------------------------------------------------------------ */
/* Saidas                                                              */
/* ------------------------------------------------------------------ */
void imprimirTokens(FILE *saida, const VetorTokens *tokens)
{
    fprintf(saida, "VETOR DE TOKENS (%zu tokens, na ordem de reconhecimento)\n\n", tokens->quantidade);
    fprintf(saida, "%-7s %-6s %-14s %s\n", "INDICE", "LINHA", "TOKEN", "LEXEMA");
    fprintf(saida, "%-7s %-6s %-14s %s\n", "------", "-----", "-----", "------");
    for (size_t i = 0; i < tokens->quantidade; i++) {
        const Token *t = &tokens->itens[i];
        fprintf(saida, "%-7zu %-6d %-14s %s\n", i, t->linha, nomeToken(t->tipo), t->lexema);
    }
}

void imprimirAFD(FILE *saida)
{
    construirAFD();
    fprintf(saida, "AFD DO ANALISADOR LEXICO (%d estados, %d classes de caracteres)\n\n",
            NUM_ESTADOS, NUM_CLASSES);
    for (int e = 0; e < NUM_ESTADOS; e++) {
        char rotulo[64];
        snprintf(rotulo, sizeof(rotulo), "q%d %s", e, nomesEstados[e]);
        if (aceitacao[e] >= 0)
            fprintf(saida, "%-20s aceita %s\n", rotulo, nomeToken((TipoToken)aceitacao[e]));
        else
            fprintf(saida, "%-20s %s\n", rotulo, e == E_INICIAL ? "estado inicial" : "nao aceita");

        /* Agrupa as classes que levam ao mesmo destino. */
        for (int destino = 0; destino < NUM_ESTADOS; destino++) {
            int quantidade = 0;
            for (int c = 0; c < NUM_CLASSES; c++)
                if (transicao[e][c] == destino) quantidade++;
            if (quantidade == 0) continue;

            fprintf(saida, "    --[");
            if (quantidade > NUM_CLASSES / 2) {
                fprintf(saida, "qualquer exceto");
                for (int c = 0; c < NUM_CLASSES; c++)
                    if (transicao[e][c] != destino) fprintf(saida, " %s", nomesClasses[c]);
            } else {
                int primeiro = 1;
                for (int c = 0; c < NUM_CLASSES; c++) {
                    if (transicao[e][c] != destino) continue;
                    fprintf(saida, "%s%s", primeiro ? "" : " ", nomesClasses[c]);
                    primeiro = 0;
                }
            }
            fprintf(saida, "]--> q%d %s\n", destino, nomesEstados[destino]);
        }
    }
}

void liberarTokens(VetorTokens *tokens)
{
    for (size_t i = 0; i < tokens->quantidade; i++) free(tokens->itens[i].lexema);
    free(tokens->itens);
    tokens->itens = NULL;
    tokens->quantidade = tokens->capacidade = 0;
}
