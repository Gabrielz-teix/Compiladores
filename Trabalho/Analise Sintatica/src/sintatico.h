/*
 * Analisador sintatico da linguagem P (descida recursiva).
 *
 * Consome o vetor de tokens produzido pelo analisador lexico, registra
 * todos os erros sintaticos (com recuperacao em modo panico) e preenche as
 * tabelas de simbolos de cada funcao.
 */
#ifndef SINTATICO_H
#define SINTATICO_H

#include "lexico.h"
#include "tabela.h"
#include "util.h"

void analisarSintatico(const VetorTokens *tokens, ListaErros *erros, ConjuntoTabelas *tabelas);

#endif
