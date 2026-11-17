#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void semMemoria(void)
{
    fprintf(stderr, "Erro: memoria insuficiente.\n");
    exit(EXIT_FAILURE);
}

void *alocar(size_t bytes)
{
    void *ponteiro = calloc(1, bytes ? bytes : 1);
    if (!ponteiro) semMemoria();
    return ponteiro;
}

void *realocar(void *ponteiro, size_t bytes)
{
    void *novo = realloc(ponteiro, bytes ? bytes : 1);
    if (!novo) semMemoria();
    return novo;
}

char *copiarTrecho(const char *inicio, size_t tamanho)
{
    char *copia = alocar(tamanho + 1);
    memcpy(copia, inicio, tamanho);
    copia[tamanho] = '\0';
    return copia;
}

char *copiarTexto(const char *texto)
{
    return copiarTrecho(texto, strlen(texto));
}

char *formatarTextoV(const char *formato, va_list argumentos)
{
    va_list copia;
    va_copy(copia, argumentos);
    int tamanho = vsnprintf(NULL, 0, formato, copia);
    va_end(copia);
    if (tamanho < 0) {
        fprintf(stderr, "Erro ao formatar mensagem.\n");
        exit(EXIT_FAILURE);
    }
    char *texto = alocar((size_t)tamanho + 1);
    vsnprintf(texto, (size_t)tamanho + 1, formato, argumentos);
    return texto;
}

char *formatarTexto(const char *formato, ...)
{
    va_list argumentos;
    va_start(argumentos, formato);
    char *texto = formatarTextoV(formato, argumentos);
    va_end(argumentos);
    return texto;
}

void registrarErro(ListaErros *lista, int linha, const char *formato, ...)
{
    GARANTIR_CAPACIDADE(lista->itens, lista->quantidade, lista->capacidade);
    va_list argumentos;
    va_start(argumentos, formato);
    lista->itens[lista->quantidade].linha = linha;
    lista->itens[lista->quantidade].mensagem = formatarTextoV(formato, argumentos);
    va_end(argumentos);
    lista->quantidade++;
}

void liberarErros(ListaErros *lista)
{
    for (size_t i = 0; i < lista->quantidade; i++) free(lista->itens[i].mensagem);
    free(lista->itens);
    lista->itens = NULL;
    lista->quantidade = lista->capacidade = 0;
}
