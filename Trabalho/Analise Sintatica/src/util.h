/*
 * Utilitarios comuns: alocacao com verificacao, copia de textos e
 * lista de erros (usada pelas fases lexica e sintatica).
 */
#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __GNUC__
#define FORMATO_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define FORMATO_PRINTF(fmt, args)
#endif

void *alocar(size_t bytes);
void *realocar(void *ponteiro, size_t bytes);
char *copiarTexto(const char *texto);
char *copiarTrecho(const char *inicio, size_t tamanho);
char *formatarTexto(const char *formato, ...) FORMATO_PRINTF(1, 2);
char *formatarTextoV(const char *formato, va_list argumentos);

/* Garante espaco para mais um elemento em um vetor dinamico. */
#define GARANTIR_CAPACIDADE(vetor, quantidade, capacidade)                      \
    do {                                                                        \
        if ((quantidade) == (capacidade)) {                                     \
            (capacidade) = (capacidade) ? (capacidade) * 2 : 8;                 \
            (vetor) = realocar((vetor), (capacidade) * sizeof(*(vetor)));       \
        }                                                                       \
    } while (0)

typedef struct {
    int linha;
    char *mensagem;
} Erro;

typedef struct {
    Erro *itens;
    size_t quantidade;
    size_t capacidade;
} ListaErros;

void registrarErro(ListaErros *lista, int linha, const char *formato, ...) FORMATO_PRINTF(3, 4);
void liberarErros(ListaErros *lista);

#endif
