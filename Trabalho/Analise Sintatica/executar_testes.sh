#!/usr/bin/env bash
# Compila o projeto e executa todas as entradas de testes/entradas.
# As saidas de cada teste ficam em testes/saidas/<nome do teste>/.
set -u
cd "$(dirname "$0")"

make -s || exit 1

for entrada in testes/entradas/*.p; do
    nome=$(basename "$entrada" .p)
    destino="testes/saidas/$nome"
    rm -rf "$destino"
    mkdir -p "$destino"
    ./compilador_p "$entrada" "$destino" > "$destino/$nome.resumo.txt"
    codigo=$?
    case $codigo in
        0) situacao="sem erros" ;;
        1) situacao="com erros (esperado nos testes de erro)" ;;
        *) situacao="FALHA DE EXECUCAO (codigo $codigo)" ;;
    esac
    printf '%-28s %s\n' "$nome" "$situacao"
done
