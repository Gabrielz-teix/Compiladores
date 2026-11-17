#include "tabela.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

const char *nomeDataType(DataTypes tipo)
{
    switch (tipo) {
        case DATA_INT: return "DataTypes::int";
        case DATA_FLOAT: return "DataTypes::float";
        case DATA_CHAR: return "DataTypes::char";
        case DATA_VOID: return "DataTypes::void";
        default: return "DataTypes::error";
    }
}

TabelaSimbolos *criarTabela(ConjuntoTabelas *tabelas, const char *funcao, int linha)
{
    TabelaSimbolos *tabela = alocar(sizeof(*tabela));
    tabela->funcao = copiarTexto(funcao);
    tabela->ret_type = DATA_VOID;
    tabela->linha = linha;
    GARANTIR_CAPACIDADE(tabelas->itens, tabelas->quantidade, tabelas->capacidade);
    tabelas->itens[tabelas->quantidade++] = tabela;
    return tabela;
}

Simbolo *buscarSimbolo(TabelaSimbolos *tabela, const char *nome)
{
    for (size_t i = 0; i < tabela->quantidade; i++)
        if (strcmp(tabela->simbolos[i].name, nome) == 0) return &tabela->simbolos[i];
    return NULL;
}

Simbolo *inserirSimbolo(TabelaSimbolos *tabela, const char *nome, DataTypes tipo,
                        int is_param, int pos_param, int linha)
{
    if (buscarSimbolo(tabela, nome)) return NULL;
    GARANTIR_CAPACIDADE(tabela->simbolos, tabela->quantidade, tabela->capacidade);
    Simbolo *s = &tabela->simbolos[tabela->quantidade++];
    memset(s, 0, sizeof(*s));
    s->name = copiarTexto(nome);
    s->datatype = tipo;
    s->is_param = is_param;
    s->pos_param = pos_param;
    s->linha = linha;
    return s;
}

FunctionRegister *registrarChamada(TabelaSimbolos *tabela, const char *nome, int linha)
{
    Simbolo *s = buscarSimbolo(tabela, nome);
    if (!s) {
        s = inserirSimbolo(tabela, nome, DATA_ERROR, 0, -1, linha);
        s->eh_chamada = 1;
    }
    FunctionRegister *registro = alocar(sizeof(*registro));
    registro->id = ++tabela->totalRegistros;
    registro->name = copiarTexto(nome);
    registro->linha = linha;
    GARANTIR_CAPACIDADE(s->call_refs, s->num_call_refs, s->capacidade_call_refs);
    s->call_refs[s->num_call_refs++] = registro;
    return registro;
}

void adicionarArgumento(FunctionRegister *registro, char *argumento)
{
    /* args cresce de um em um: chamadas costumam ter poucos argumentos. */
    registro->args = realocar(registro->args, (size_t)(registro->num_args + 1) * sizeof(char *));
    registro->args[registro->num_args++] = argumento;
}

void resolverTiposDasChamadas(ConjuntoTabelas *tabelas)
{
    for (size_t t = 0; t < tabelas->quantidade; t++) {
        TabelaSimbolos *tabela = tabelas->itens[t];
        for (size_t i = 0; i < tabela->quantidade; i++) {
            Simbolo *s = &tabela->simbolos[i];
            if (!s->eh_chamada) continue;
            s->datatype = DATA_ERROR;
            for (size_t f = 0; f < tabelas->quantidade; f++) {
                if (strcmp(tabelas->itens[f]->funcao, s->name) == 0) {
                    s->datatype = tabelas->itens[f]->ret_type;
                    break;
                }
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Impressao                                                           */
/* ------------------------------------------------------------------ */
static void imprimirCallRefs(FILE *saida, const Simbolo *s)
{
    if (s->num_call_refs == 0) {
        fprintf(saida, "NULL");
        return;
    }
    fprintf(saida, "[");
    for (int i = 0; i < s->num_call_refs; i++)
        fprintf(saida, "%s#%d", i ? ", " : "", s->call_refs[i]->id);
    fprintf(saida, "]");
}

static void imprimirTabela(FILE *saida, const TabelaSimbolos *tabela)
{
    fprintf(saida, "==================================================================\n");
    fprintf(saida, "Tabela de simbolos da funcao '%s' (definida na linha %d)\n", tabela->funcao, tabela->linha);
    fprintf(saida, "ret_type(%s) = %s\n", tabela->funcao, nomeDataType(tabela->ret_type));
    fprintf(saida, "==================================================================\n");

    if (tabela->quantidade == 0) {
        fprintf(saida, "(tabela vazia)\n\n");
        return;
    }

    int largura = 5;
    for (size_t i = 0; i < tabela->quantidade; i++) {
        int n = (int)strlen(tabela->simbolos[i].name);
        if (n > largura) largura = n;
    }

    fprintf(saida, "%-*s  %-*s  %-16s  %-8s  %-9s  %s\n", largura, "chave", largura, "name",
            "datatype", "is_param", "pos_param", "call_refs");
    for (size_t i = 0; i < tabela->quantidade; i++) {
        const Simbolo *s = &tabela->simbolos[i];
        fprintf(saida, "%-*s  %-*s  %-16s  %-8s  %-9d  ", largura, s->name, largura, s->name,
                nomeDataType(s->datatype), s->is_param ? "true" : "false", s->pos_param);
        imprimirCallRefs(saida, s);
        fprintf(saida, "\n");
    }

    if (tabela->totalRegistros > 0) {
        fprintf(saida, "\nFunctionRegister (chamadas feitas dentro de '%s'):\n", tabela->funcao);
        fprintf(saida, "  %-4s %-*s  %-8s  %-5s  %s\n", "ref", largura, "name", "num_args", "linha", "args");
        /* Imprime os registros na ordem em que as chamadas aparecem. */
        for (int id = 1; id <= tabela->totalRegistros; id++) {
            for (size_t i = 0; i < tabela->quantidade; i++) {
                const Simbolo *s = &tabela->simbolos[i];
                for (int r = 0; r < s->num_call_refs; r++) {
                    const FunctionRegister *reg = s->call_refs[r];
                    if (reg->id != id) continue;
                    char ref[16];
                    snprintf(ref, sizeof(ref), "#%d", reg->id);
                    fprintf(saida, "  %-4s %-*s  %-8d  %-5d  [", ref, largura, reg->name,
                            reg->num_args, reg->linha);
                    for (int a = 0; a < reg->num_args; a++)
                        fprintf(saida, "%s%s", a ? ", " : "", reg->args[a]);
                    fprintf(saida, "]\n");
                }
            }
        }
    }
    fprintf(saida, "\n");
}

void imprimirTabelas(FILE *saida, const ConjuntoTabelas *tabelas)
{
    fprintf(saida, "TABELAS DE SIMBOLOS (%zu funcoes)\n\n", tabelas->quantidade);
    for (size_t t = 0; t < tabelas->quantidade; t++) imprimirTabela(saida, tabelas->itens[t]);
}

void liberarTabelas(ConjuntoTabelas *tabelas)
{
    for (size_t t = 0; t < tabelas->quantidade; t++) {
        TabelaSimbolos *tabela = tabelas->itens[t];
        for (size_t i = 0; i < tabela->quantidade; i++) {
            Simbolo *s = &tabela->simbolos[i];
            for (int r = 0; r < s->num_call_refs; r++) {
                FunctionRegister *reg = s->call_refs[r];
                for (int a = 0; a < reg->num_args; a++) free(reg->args[a]);
                free(reg->args);
                free(reg->name);
                free(reg);
            }
            free(s->call_refs);
            free(s->name);
        }
        free(tabela->simbolos);
        free(tabela->funcao);
        free(tabela);
    }
    free(tabelas->itens);
    tabelas->itens = NULL;
    tabelas->quantidade = tabelas->capacidade = 0;
}
