# Prática 2: geração de código de três endereços

O parser da Prática 1 foi adaptado para emitir código de três endereços (TAC) enquanto analisa o programa-fonte, em uma única passagem e sem construir árvore sintática.

## Arquivos

- `csmall_tac.c`: lexer, parser, análise semântica e geração de TAC.
- `programa_tac.csmall`: programa fornecido no enunciado, sem alterações.
- `programa_tac_corrigido.csmall`: o mesmo programa com `print(soma)` trocado por `print(s)` (ver abaixo).
- `programa_tac.tac`: TAC gerado a partir de `programa_tac_corrigido.csmall`.
- `exemplo_while.csmall` e `exemplo_while.tac`: o laço descrito no texto do enunciado, que imprime 27.

## Como compilar e executar

```bash
gcc -std=c11 -Wall -Wextra -o csmall_tac csmall_tac.c
./csmall_tac programa_tac_corrigido.csmall programa_tac.tac   # grava no arquivo
./csmall_tac programa_tac_corrigido.csmall                    # mostra na tela
```

No Windows, use `csmall_tac.exe` no lugar de `./csmall_tac`.

## Observações sobre o programa fornecido

**`print(soma)`.** A variável `soma` não é declarada no programa: as variáveis são `a`, `b` e `s`. Como a análise semântica da prática anterior foi mantida, o programa original é rejeitado:

```text
Erro semantico na linha 12: variavel nao declarada: 'soma'
```

Considerei que a intenção era imprimir `s` e gerei o `programa_tac.tac` a partir da versão corrigida. Desligar a verificação para aceitar `soma` produziria um `print` de uma variável que nunca recebeu valor.

**Texto e código diferentes.** A seção 2 descreve um laço com `i` de 1 a 5 que imprime 27, mas o código fornecido é um `if/else` sem laço. Por isso incluí também `exemplo_while.csmall`, que implementa o laço descrito e exercita a tradução do `while`.

**Declaração com inicialização.** A gramática da Prática 1 aceitava apenas `int a;`. Como o programa usa `int a = 5;`, a declaração passou a aceitar `= expressao` opcional. A inicialização é verificada quanto ao tipo e emite a cópia `a = 5`.

## Como a geração foi feita

- `novoTemporario()` e `novoRotulo()` criam `t1, t2, ...` e `L1, L2, ...`. Os contadores são zerados no início de `analisarPrograma()`.
- `emitir()` acrescenta uma instrução por linha ao código gerado.
- `fator()`, `termo()` e `expressao()` devolvem o tipo (para a análise semântica) e o nome do operando com o resultado. Identificadores e constantes são usados diretamente; cada operação cria um temporário. A precedência vem da própria gramática: `*` e `/` são emitidos antes de `+` e `-`.
- Atribuições emitem `x = operando`, e `print` emite `print operando`.
- `condicao(verdadeiro, falso)` recebe os rótulos do comando que a chamou e emite `if x op y goto Lv` seguido de `goto Lf`.
- `if/else` e `while` seguem exatamente os passos 5 e 6 do enunciado. Em um `if` sem `else`, o rótulo falso e o de saída ficam seguidos, o que é redundante mas correto.

O código gerado fica em memória e só é gravado quando a análise termina sem erros. Assim, um programa inválido não deixa um `.tac` incompleto nem sobrescreve um arquivo já existente.

## Limitação conhecida

O TAC não tem escopos. Se um bloco interno declarar uma variável com o mesmo nome de uma externa, as duas usarão o mesmo nome no código gerado. Os programas desta prática não fazem isso.
