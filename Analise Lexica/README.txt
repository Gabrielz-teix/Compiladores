COMPILADORES - PRIMEIRO ENVIO: ANALISE LEXICA

Arquivo principal: csmall.c
O arquivo foi feito a partir do codigo-base fornecido pelo professor.
Nesta etapa, a funcao proximoToken() reconhece os tokens da Csmall.
A parte do parser e da tabela de simbolos permanece como estrutura inicial,
pois pertence a entregas posteriores.

Compilar com GCC:
  gcc -std=c11 -Wall -Wextra csmall.c -o csmall

Executar no Linux/macOS:
  ./csmall exemplo.csmall

Executar no Windows (PowerShell):
  gcc -std=c11 -Wall -Wextra csmall.c -o csmall.exe
  .\csmall.exe exemplo.csmall

A saida mostra TOKEN, LEXEMA e LINHA. Ao encontrar erro lexico,
o programa indica a linha e termina com codigo de erro.
