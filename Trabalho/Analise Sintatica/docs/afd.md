# AFD do analisador léxico

O analisador léxico é guiado por este autômato. Ele está implementado como uma tabela de transições `transicao[estado][classe]` em [src/lexico.c](../src/lexico.c), na função `construirAFD()`. O comando abaixo imprime a tabela que o programa realmente usa, então é possível conferir que o código e este diagrama são o mesmo autômato:

```bash
./compilador_p --afd
```

## Alfabeto (classes de caracteres)

Cada caractere da entrada é classificado antes de consultar a tabela:

| Classe | Caracteres |
|---|---|
| `letra` | `a`–`z`, `A`–`Z` |
| `digito` | `0`–`9` |
| `_ . ' " = ! < > - + * / ( ) { } : ; ,` | o próprio caractere |
| `espaco` | espaço, tabulação, `\r` |
| `\n` | quebra de linha |
| `outro` | qualquer outro caractere (por exemplo `@`, `#`, `%`, acentos) |
| `EOF` | fim da entrada |

## Diagrama

Estados com círculo duplo são de aceitação. O rótulo de cada estado de aceitação indica o token reconhecido.

```mermaid
flowchart LR
    q0((q0<br/>INICIAL))

    q0 -- letra --> q1(((q1<br/>ID)))
    q1 -- "letra, digito, _" --> q1

    q0 -- digito --> q2(((q2<br/>INT_CONST)))
    q2 -- digito --> q2
    q2 -- "." --> q3((q3))
    q3 -- digito --> q4(((q4<br/>FLOAT_CONST)))
    q4 -- digito --> q4

    q0 -- "'" --> q5((q5))
    q5 -- "qualquer exceto ' \n EOF" --> q6((q6))
    q6 -- "'" --> q7(((q7<br/>CHAR_LITERAL)))

    q0 -- "&quot;" --> q8((q8))
    q8 -- "qualquer exceto &quot; \n EOF" --> q9((q9))
    q9 -- "qualquer exceto &quot; \n EOF" --> q9
    q9 -- "&quot;" --> q10(((q10<br/>FMT_STRING)))

    q0 -- "=" --> q11(((q11<br/>ASSIGN)))
    q11 -- "=" --> q12(((q12<br/>EQ)))
    q0 -- "!" --> q13((q13))
    q13 -- "=" --> q14(((q14<br/>NE)))
    q0 -- "<" --> q15(((q15<br/>LT)))
    q15 -- "=" --> q16(((q16<br/>LE)))
    q0 -- ">" --> q17(((q17<br/>GT)))
    q17 -- "=" --> q18(((q18<br/>GE)))
    q0 -- "-" --> q19(((q19<br/>MINUS)))
    q19 -- ">" --> q20(((q20<br/>ARROW)))

    q0 -- "+" --> q21(((q21<br/>PLUS)))
    q0 -- "*" --> q22(((q22<br/>MULT)))
    q0 -- "/" --> q23(((q23<br/>DIV)))
    q0 -- "(" --> q24(((q24<br/>LBRACKET)))
    q0 -- ")" --> q25(((q25<br/>RBRACKET)))
    q0 -- "{" --> q26(((q26<br/>LBRACE)))
    q0 -- "}" --> q27(((q27<br/>RBRACE)))
    q0 -- ":" --> q28(((q28<br/>COLON)))
    q0 -- ";" --> q29(((q29<br/>SEMICOLON)))
    q0 -- "," --> q30(((q30<br/>COMMA)))
```

## Tabela de estados

| Estado | Aceita | Transições |
|---|---|---|
| q0 INICIAL | — | ver diagrama |
| q1 ID | `ID` ou palavra reservada | letra, dígito, `_` → q1 |
| q2 INT | `INT_CONST` | dígito → q2; `.` → q3 |
| q3 FLOAT_PONTO | não | dígito → q4 |
| q4 FLOAT | `FLOAT_CONST` | dígito → q4 |
| q5 CHAR_ABRE | não | qualquer exceto `'`, `\n`, EOF → q6 |
| q6 CHAR_CONTEUDO | não | `'` → q7 |
| q7 CHAR | `CHAR_LITERAL` | — |
| q8 STR_ABRE | não | qualquer exceto `"`, `\n`, EOF → q9 |
| q9 STR_CONTEUDO | não | qualquer exceto `"`, `\n`, EOF → q9; `"` → q10 |
| q10 STR | `FMT_STRING` | — |
| q11 ASSIGN | `ASSIGN` | `=` → q12 |
| q12 EQ | `EQ` | — |
| q13 EXCLAMACAO | não | `=` → q14 |
| q14 NE | `NE` | — |
| q15 LT | `LT` | `=` → q16 |
| q16 LE | `LE` | — |
| q17 GT | `GT` | `=` → q18 |
| q18 GE | `GE` | — |
| q19 MINUS | `MINUS` | `>` → q20 |
| q20 ARROW | `ARROW` | — |
| q21–q30 | `PLUS`, `MULT`, `DIV`, `LBRACKET`, `RBRACKET`, `LBRACE`, `RBRACE`, `COLON`, `SEMICOLON`, `COMMA` | — |

Qualquer par (estado, classe) que não aparece na tabela não tem transição.

## Como o AFD é usado

1. Espaços e quebras de linha entre lexemas são descartados, e o contador de linhas é atualizado.
2. O AFD parte de q0 e segue as transições enquanto existirem (lexema mais longo).
3. Se parar em um estado de aceitação, o lexema vira um token. No estado q1, o lexema é comparado com a lista de palavras reservadas (`fn`, `main`, `let`, `int`, `float`, `char`, `if`, `else`, `while`, `println`, `return`), e o token passa a ser o da palavra reservada.
4. Se parar em um estado que não é de aceitação, é um erro léxico. A mensagem depende do estado em que o AFD parou:

| Estado de parada | Erro relatado | Exemplo |
|---|---|---|
| q0 | caractere inválido | `@`, `#`, `_abc` |
| q3 | constante float mal formada | `12.` |
| q5 | literal de caractere vazio ou não terminado | `''` |
| q6 | literal com mais de um caractere ou não terminado | `'ab'` |
| q8 | string de formatação vazia ou não terminada | `""` |
| q9 | string de formatação não terminada | `"sem fim` |
| q13 | `!` isolado | `x ! 3` |

O lexema com erro não entra no vetor de tokens, e a análise continua logo depois dele. Assim, todos os erros léxicos do arquivo são encontrados em uma única execução.
