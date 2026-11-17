CC      = gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -pedantic -O2
FONTES  := src/main.c src/lexico.c src/sintatico.c src/tabela.c src/util.c
HEADERS := src/lexico.h src/sintatico.h src/tabela.h src/util.h
ALVO    := compilador_p

.PHONY: all testes limpar

all: $(ALVO)

$(ALVO): $(FONTES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(FONTES)

testes: $(ALVO)
	bash executar_testes.sh

limpar:
	rm -f $(ALVO)
	rm -rf saida
