/*
 * Cada nao-terminal da gramatica da linguagem P tem uma funcao com o mesmo
 * nome. As producoes com recursao a direita (FuncaoSeq, Sequencia, ...Opc,
 * ...2) foram mantidas como funcoes recursivas, como na gramatica.
 *
 * Recuperacao de erros (modo panico): ao encontrar um erro, o parser o
 * registra e entra em panico; enquanto estiver em panico, novos erros nao
 * sao registrados (evita cascata). No inicio de cada declaracao/comando o
 * parser descarta tokens ate um ponto de sincronizacao (';', '}', inicio de
 * comando ou de funcao) e volta ao estado normal.
 */
#include "sintatico.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    const VetorTokens *tokens;
    size_t pos;
    ListaErros *erros;
    ConjuntoTabelas *tabelas;
    TabelaSimbolos *tabelaAtual;
    int panico;
    int linhaErro;
} Parser;

/* ------------------------------------------------------------------ */
/* Acesso ao vetor de tokens                                           */
/* ------------------------------------------------------------------ */
static const Token *atual(const Parser *p) { return &p->tokens->itens[p->pos]; }
static TipoToken tipoAtual(const Parser *p) { return atual(p)->tipo; }

static void avancar(Parser *p)
{
    if (tipoAtual(p) != TK_EOF) p->pos++;
}

/* Descricao usada nas mensagens: "';'", "identificador", ... */
static const char *descricao(TipoToken tipo)
{
    switch (tipo) {
        case TK_FUNCTION: return "'fn'";
        case TK_MAIN: return "'main'";
        case TK_LET: return "'let'";
        case TK_INT: return "'int'";
        case TK_FLOAT: return "'float'";
        case TK_CHAR: return "'char'";
        case TK_IF: return "'if'";
        case TK_ELSE: return "'else'";
        case TK_WHILE: return "'while'";
        case TK_PRINTLN: return "'println'";
        case TK_RETURN: return "'return'";
        case TK_LBRACKET: return "'('";
        case TK_RBRACKET: return "')'";
        case TK_LBRACE: return "'{'";
        case TK_RBRACE: return "'}'";
        case TK_ARROW: return "'->'";
        case TK_COLON: return "':'";
        case TK_SEMICOLON: return "';'";
        case TK_COMMA: return "','";
        case TK_ASSIGN: return "'='";
        case TK_EQ: return "'=='";
        case TK_NE: return "'!='";
        case TK_GT: return "'>'";
        case TK_GE: return "'>='";
        case TK_LT: return "'<'";
        case TK_LE: return "'<='";
        case TK_PLUS: return "'+'";
        case TK_MINUS: return "'-'";
        case TK_MULT: return "'*'";
        case TK_DIV: return "'/'";
        case TK_ID: return "identificador";
        case TK_INT_CONST: return "constante inteira";
        case TK_FLOAT_CONST: return "constante float";
        case TK_CHAR_LITERAL: return "literal char";
        case TK_FMT_STRING: return "string de formatacao";
        case TK_EOF: return "fim do arquivo";
    }
    return "token desconhecido";
}

static char *descreverEncontrado(const Token *t)
{
    switch (t->tipo) {
        case TK_ID: case TK_INT_CONST: case TK_FLOAT_CONST:
        case TK_CHAR_LITERAL: case TK_FMT_STRING:
            return formatarTexto("%s %s", descricao(t->tipo), t->lexema);
        default:
            return copiarTexto(descricao(t->tipo));
    }
}

/* ------------------------------------------------------------------ */
/* Erros e recuperacao                                                 */
/* ------------------------------------------------------------------ */
/* `faltaToken` indica que o erro e a ausencia de um token (';', ')', ...).
   Nesse caso, se o token encontrado ja esta em outra linha, o erro e
   relatado no fim da linha anterior, onde o token deveria estar. */
static void registrarErroSintatico(Parser *p, const char *esperado, int faltaToken)
{
    if (p->panico) return;
    p->panico = 1;

    const Token *t = atual(p);
    const Token *anterior = p->pos > 0 ? &p->tokens->itens[p->pos - 1] : NULL;
    char *encontrado = descreverEncontrado(t);

    if (faltaToken && anterior && anterior->linha < t->linha) {
        /* O token que faltou deveria estar no fim da linha anterior
           (ex.: ';' esquecido): o erro e relatado nessa linha. */
        p->linhaErro = anterior->linha;
        registrarErro(p->erros, anterior->linha, "esperado %s apos '%s', encontrado %s na linha %d",
                      esperado, anterior->lexema, encontrado, t->linha);
    } else {
        p->linhaErro = t->linha;
        registrarErro(p->erros, t->linha, "esperado %s, encontrado %s", esperado, encontrado);
    }
    free(encontrado);
}

/* Token inesperado: o erro e relatado na linha do token encontrado. */
static void erroSintatico(Parser *p, const char *esperado)
{
    registrarErroSintatico(p, esperado, 0);
}

static int esperar(Parser *p, TipoToken tipo)
{
    if (tipoAtual(p) == tipo) {
        avancar(p);
        return 1;
    }
    registrarErroSintatico(p, descricao(tipo), 1);
    return 0;
}

static int inicioDeComando(TipoToken t)
{
    return t == TK_ID || t == TK_IF || t == TK_LBRACE || t == TK_WHILE ||
           t == TK_PRINTLN || t == TK_RETURN;
}

static int inicioDeArg(TipoToken t)
{
    return t == TK_ID || t == TK_INT_CONST || t == TK_FLOAT_CONST || t == TK_CHAR_LITERAL;
}

/* Descarta tokens ate um ponto seguro para recomecar a analise. Um
   identificador so e ponto de sincronizacao se estiver em uma linha
   posterior ao erro (provavel inicio de uma nova atribuicao). */
static void sincronizar(Parser *p)
{
    for (;;) {
        const Token *t = atual(p);
        if (t->tipo == TK_EOF || t->tipo == TK_RBRACE || t->tipo == TK_FUNCTION || t->tipo == TK_LET)
            break;
        if (inicioDeComando(t->tipo) && (t->tipo != TK_ID || t->linha > p->linhaErro))
            break;
        avancar(p);
        if (t->tipo == TK_SEMICOLON) break;
    }
    p->panico = 0;
}

/* Texto dos tokens [inicio, fim), usado para guardar os argumentos. */
static char *textoDosTokens(const Parser *p, size_t inicio, size_t fim)
{
    size_t tamanho = 1;
    for (size_t i = inicio; i < fim; i++) tamanho += strlen(p->tokens->itens[i].lexema) + 1;
    char *texto = alocar(tamanho);
    for (size_t i = inicio; i < fim; i++) {
        const Token *t = &p->tokens->itens[i];
        strcat(texto, t->lexema);
        if (t->tipo == TK_COMMA) strcat(texto, " ");
    }
    return texto;
}

/* ------------------------------------------------------------------ */
/* Gramatica                                                           */
/* ------------------------------------------------------------------ */
static void funcao(Parser *p);
static void funcaoSeq(Parser *p);
static const char *nomeFuncao(Parser *p);
static void listaParams(Parser *p);
static void listaParams2(Parser *p, int posicao);
static DataTypes tipoRetornoFuncao(Parser *p);
static void bloco(Parser *p);
static void sequencia(Parser *p);
static void declaracao(Parser *p);
static DataTypes type(Parser *p);
static void comando(Parser *p);
static void atribuicaoOuChamada(Parser *p, const Token *id);
static void comandoSe(Parser *p);
static void comandoSenao(Parser *p);
static void expr(Parser *p);
static void exprOpc(Parser *p);
static void rel(Parser *p);
static void relOpc(Parser *p);
static void adicao(Parser *p);
static void adicaoOpc(Parser *p);
static void termo(Parser *p);
static void termoOpc(Parser *p);
static void fator(Parser *p);
static void chamadaFuncao(Parser *p, const Token *id);
static void listaArgs(Parser *p, FunctionRegister *registro);
static void listaArgs2(Parser *p, FunctionRegister *registro);
static void arg(Parser *p, FunctionRegister *registro);

/* Programa -> Funcao FuncaoSeq */
static void programa(Parser *p)
{
    if (tipoAtual(p) == TK_FUNCTION)
        funcao(p);
    else if (tipoAtual(p) == TK_EOF)
        erroSintatico(p, "'fn' (o programa deve ter ao menos uma funcao)");
    funcaoSeq(p);

    /* Tokens fora de qualquer funcao (inclusive antes da primeira):
       relata e procura a proxima funcao. */
    while (tipoAtual(p) != TK_EOF) {
        p->panico = 0;
        erroSintatico(p, "'fn' ou fim do arquivo");
        while (tipoAtual(p) != TK_FUNCTION && tipoAtual(p) != TK_EOF) avancar(p);
        funcaoSeq(p);
    }
}

/* FuncaoSeq -> Funcao FuncaoSeq | epsilon */
static void funcaoSeq(Parser *p)
{
    if (tipoAtual(p) == TK_FUNCTION) {
        funcao(p);
        funcaoSeq(p);
    }
}

/* Funcao -> fn NomeFuncao ( ListaParams ) TipoRetornoFuncao Bloco */
static void funcao(Parser *p)
{
    p->panico = 0;
    int linha = atual(p)->linha;
    esperar(p, TK_FUNCTION);
    const char *nome = nomeFuncao(p);
    if (nome) {
        p->tabelaAtual = criarTabela(p->tabelas, nome, linha);
    } else {
        char *anonimo = formatarTexto("<sem nome, linha %d>", linha);
        p->tabelaAtual = criarTabela(p->tabelas, anonimo, linha);
        free(anonimo);
    }
    esperar(p, TK_LBRACKET);
    listaParams(p);
    esperar(p, TK_RBRACKET);
    p->tabelaAtual->ret_type = tipoRetornoFuncao(p);

    if (tipoAtual(p) != TK_LBRACE) {
        erroSintatico(p, "'{' iniciando o corpo da funcao");
        while (tipoAtual(p) != TK_LBRACE && tipoAtual(p) != TK_FUNCTION && tipoAtual(p) != TK_EOF)
            avancar(p);
        if (tipoAtual(p) != TK_LBRACE) return;
    }
    p->panico = 0;
    bloco(p);
}

/* NomeFuncao -> ID | MAIN */
static const char *nomeFuncao(Parser *p)
{
    if (tipoAtual(p) == TK_ID || tipoAtual(p) == TK_MAIN) {
        const char *nome = atual(p)->lexema;
        avancar(p);
        return nome;
    }
    erroSintatico(p, "nome da funcao (identificador ou 'main')");
    return NULL;
}

/* ListaParams -> ID : Type ListaParams2 | epsilon */
static void listaParams(Parser *p)
{
    if (tipoAtual(p) != TK_ID) return;
    const Token *id = atual(p);
    avancar(p);
    esperar(p, TK_COLON);
    DataTypes tipo = type(p);
    inserirSimbolo(p->tabelaAtual, id->lexema, tipo, 1, 0, id->linha);
    listaParams2(p, 1);
}

/* ListaParams2 -> , ID : Type ListaParams2 | epsilon */
static void listaParams2(Parser *p, int posicao)
{
    if (tipoAtual(p) != TK_COMMA) return;
    avancar(p);
    const Token *id = atual(p);
    int temId = esperar(p, TK_ID);
    esperar(p, TK_COLON);
    DataTypes tipo = type(p);
    if (temId) inserirSimbolo(p->tabelaAtual, id->lexema, tipo, 1, posicao, id->linha);
    listaParams2(p, posicao + 1);
}

/* TipoRetornoFuncao -> -> Type | epsilon */
static DataTypes tipoRetornoFuncao(Parser *p)
{
    if (tipoAtual(p) != TK_ARROW) return DATA_VOID;
    avancar(p);
    return type(p);
}

/* Bloco -> { Sequencia } */
static void bloco(Parser *p)
{
    if (!esperar(p, TK_LBRACE)) {
        /* Se houver um '{' logo adiante (antes de ';' ou '}'), os tokens
           extras sao descartados ate ele, ex.: "if x 3 { ... }". Senao, o
           bloco continua como se o '{' tivesse sido escrito. */
        size_t i = p->pos;
        const Token *itens = p->tokens->itens;
        while (itens[i].tipo != TK_EOF && itens[i].tipo != TK_SEMICOLON &&
               itens[i].tipo != TK_RBRACE && itens[i].tipo != TK_LBRACE)
            i++;
        if (itens[i].tipo == TK_LBRACE) {
            p->pos = i + 1;
            p->panico = 0;
        }
    }
    sequencia(p);
    esperar(p, TK_RBRACE);
}

/* Sequencia -> Declaracao Sequencia | Comando Sequencia | epsilon */
static void sequencia(Parser *p)
{
    if (p->panico) sincronizar(p);

    TipoToken t = tipoAtual(p);
    if (t == TK_LET) {
        declaracao(p);
        sequencia(p);
    } else if (inicioDeComando(t)) {
        comando(p);
        sequencia(p);
    } else if (t == TK_RBRACE || t == TK_EOF || t == TK_FUNCTION) {
        /* epsilon */
    } else {
        erroSintatico(p, "declaracao, comando ou '}'");
        sincronizar(p);
        sequencia(p);
    }
}

/* Declaracao -> let VarList : Type ;
   VarList    -> ID VarList2
   VarList2   -> , ID VarList2 | epsilon
   Os nomes sao guardados ate o tipo ser conhecido. */
typedef struct {
    const Token **itens;
    size_t quantidade;
    size_t capacidade;
} ListaIds;

static void varList2(Parser *p, ListaIds *ids);

static void varList(Parser *p, ListaIds *ids)
{
    const Token *id = atual(p);
    if (esperar(p, TK_ID)) {
        GARANTIR_CAPACIDADE(ids->itens, ids->quantidade, ids->capacidade);
        ids->itens[ids->quantidade++] = id;
    }
    varList2(p, ids);
}

static void varList2(Parser *p, ListaIds *ids)
{
    if (tipoAtual(p) != TK_COMMA) return;
    avancar(p);
    const Token *id = atual(p);
    if (esperar(p, TK_ID)) {
        GARANTIR_CAPACIDADE(ids->itens, ids->quantidade, ids->capacidade);
        ids->itens[ids->quantidade++] = id;
    }
    varList2(p, ids);
}

static void declaracao(Parser *p)
{
    ListaIds ids = {0};
    esperar(p, TK_LET);
    varList(p, &ids);
    esperar(p, TK_COLON);
    DataTypes tipo = type(p);
    esperar(p, TK_SEMICOLON);
    for (size_t i = 0; i < ids.quantidade; i++)
        inserirSimbolo(p->tabelaAtual, ids.itens[i]->lexema, tipo, 0, -1, ids.itens[i]->linha);
    free(ids.itens);
}

/* Type -> int | float | char */
static DataTypes type(Parser *p)
{
    switch (tipoAtual(p)) {
        case TK_INT: avancar(p); return DATA_INT;
        case TK_FLOAT: avancar(p); return DATA_FLOAT;
        case TK_CHAR: avancar(p); return DATA_CHAR;
        default:
            erroSintatico(p, "tipo ('int', 'float' ou 'char')");
            return DATA_ERROR;
    }
}

/* Comando -> ID AtribuicaoOuChamada
            | ComandoSe
            | while Expr Bloco
            | println ( FMT_STRING , ListaArgs ) ;
            | return Expr ; */
static void comando(Parser *p)
{
    const Token *t = atual(p);
    switch (t->tipo) {
        case TK_ID:
            avancar(p);
            atribuicaoOuChamada(p, t);
            break;
        case TK_IF:
        case TK_LBRACE:
            comandoSe(p);
            break;
        case TK_WHILE:
            avancar(p);
            expr(p);
            bloco(p);
            break;
        case TK_PRINTLN:
            avancar(p);
            /* println aparece na tabela da funcao como void, sem
               FunctionRegister (conforme o exemplo da especificacao). */
            inserirSimbolo(p->tabelaAtual, "println", DATA_VOID, 0, -1, t->linha);
            esperar(p, TK_LBRACKET);
            esperar(p, TK_FMT_STRING);
            esperar(p, TK_COMMA);
            listaArgs(p, NULL);
            esperar(p, TK_RBRACKET);
            esperar(p, TK_SEMICOLON);
            break;
        case TK_RETURN:
            avancar(p);
            expr(p);
            esperar(p, TK_SEMICOLON);
            break;
        default:
            erroSintatico(p, "comando");
            break;
    }
}

/* AtribuicaoOuChamada -> = Expr ; | ( ListaArgs ) ; */
static void atribuicaoOuChamada(Parser *p, const Token *id)
{
    if (tipoAtual(p) == TK_ASSIGN) {
        avancar(p);
        expr(p);
        esperar(p, TK_SEMICOLON);
    } else if (tipoAtual(p) == TK_LBRACKET) {
        FunctionRegister *registro = registrarChamada(p->tabelaAtual, id->lexema, id->linha);
        avancar(p);
        listaArgs(p, registro);
        esperar(p, TK_RBRACKET);
        esperar(p, TK_SEMICOLON);
    } else {
        erroSintatico(p, "'=' ou '(' apos o identificador");
    }
}

/* ComandoSe -> if Expr Bloco ComandoSenao | Bloco */
static void comandoSe(Parser *p)
{
    if (tipoAtual(p) == TK_IF) {
        avancar(p);
        expr(p);
        bloco(p);
        comandoSenao(p);
    } else {
        bloco(p);
    }
}

/* ComandoSenao -> else ComandoSe | epsilon */
static void comandoSenao(Parser *p)
{
    if (tipoAtual(p) != TK_ELSE) return;
    avancar(p);
    if (tipoAtual(p) == TK_IF || tipoAtual(p) == TK_LBRACE)
        comandoSe(p);
    else
        erroSintatico(p, "'if' ou '{' apos 'else'");
}

/* Expr -> Rel ExprOpc */
static void expr(Parser *p)
{
    rel(p);
    exprOpc(p);
}

/* ExprOpc -> OpIgual Rel ExprOpc | epsilon      OpIgual -> == | != */
static void exprOpc(Parser *p)
{
    if (tipoAtual(p) != TK_EQ && tipoAtual(p) != TK_NE) return;
    avancar(p);
    rel(p);
    exprOpc(p);
}

/* Rel -> Adicao RelOpc */
static void rel(Parser *p)
{
    adicao(p);
    relOpc(p);
}

/* RelOpc -> OpRel Adicao RelOpc | epsilon        OpRel -> < | <= | > | >= */
static void relOpc(Parser *p)
{
    TipoToken t = tipoAtual(p);
    if (t != TK_LT && t != TK_LE && t != TK_GT && t != TK_GE) return;
    avancar(p);
    adicao(p);
    relOpc(p);
}

/* Adicao -> Termo AdicaoOpc */
static void adicao(Parser *p)
{
    termo(p);
    adicaoOpc(p);
}

/* AdicaoOpc -> OpAdicao Termo AdicaoOpc | epsilon  OpAdicao -> + | - */
static void adicaoOpc(Parser *p)
{
    if (tipoAtual(p) != TK_PLUS && tipoAtual(p) != TK_MINUS) return;
    avancar(p);
    termo(p);
    adicaoOpc(p);
}

/* Termo -> Fator TermoOpc */
static void termo(Parser *p)
{
    fator(p);
    termoOpc(p);
}

/* TermoOpc -> OpMult Fator TermoOpc | epsilon    OpMult -> * | / */
static void termoOpc(Parser *p)
{
    if (tipoAtual(p) != TK_MULT && tipoAtual(p) != TK_DIV) return;
    avancar(p);
    fator(p);
    termoOpc(p);
}

/* Fator -> ID ChamadaFuncao | INT_CONST | FLOAT_CONST | CHAR_LITERAL | ( Expr ) */
static void fator(Parser *p)
{
    const Token *t = atual(p);
    switch (t->tipo) {
        case TK_ID:
            avancar(p);
            chamadaFuncao(p, t);
            break;
        case TK_INT_CONST:
        case TK_FLOAT_CONST:
        case TK_CHAR_LITERAL:
            avancar(p);
            break;
        case TK_LBRACKET:
            avancar(p);
            expr(p);
            esperar(p, TK_RBRACKET);
            break;
        default:
            erroSintatico(p, "expressao (identificador, constante ou '(')");
            break;
    }
}

/* ChamadaFuncao -> ( ListaArgs ) | epsilon */
static void chamadaFuncao(Parser *p, const Token *id)
{
    if (tipoAtual(p) != TK_LBRACKET) return;
    FunctionRegister *registro = registrarChamada(p->tabelaAtual, id->lexema, id->linha);
    avancar(p);
    listaArgs(p, registro);
    esperar(p, TK_RBRACKET);
}

/* ListaArgs -> Arg ListaArgs2 | epsilon */
static void listaArgs(Parser *p, FunctionRegister *registro)
{
    if (!inicioDeArg(tipoAtual(p))) return;
    arg(p, registro);
    listaArgs2(p, registro);
}

/* ListaArgs2 -> , Arg ListaArgs2 | epsilon */
static void listaArgs2(Parser *p, FunctionRegister *registro)
{
    if (tipoAtual(p) != TK_COMMA) return;
    avancar(p);
    arg(p, registro);
    listaArgs2(p, registro);
}

/* Arg -> ID ChamadaFuncao | INT_CONST | FLOAT_CONST | CHAR_LITERAL */
static void arg(Parser *p, FunctionRegister *registro)
{
    size_t inicio = p->pos;
    const Token *t = atual(p);
    switch (t->tipo) {
        case TK_ID:
            avancar(p);
            chamadaFuncao(p, t);
            break;
        case TK_INT_CONST:
        case TK_FLOAT_CONST:
        case TK_CHAR_LITERAL:
            avancar(p);
            break;
        default:
            erroSintatico(p, "argumento (identificador, chamada ou constante)");
            return;
    }
    if (registro) adicionarArgumento(registro, textoDosTokens(p, inicio, p->pos));
}

/* ------------------------------------------------------------------ */
void analisarSintatico(const VetorTokens *tokens, ListaErros *erros, ConjuntoTabelas *tabelas)
{
    Parser p = {0};
    p.tokens = tokens;
    p.erros = erros;
    p.tabelas = tabelas;
    programa(&p);
}
