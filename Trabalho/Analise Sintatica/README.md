# Front-end do compilador da linguagem P: análise léxica, análise sintática e tabela de símbolos

Trabalho prático de Compiladores (UFSJ), feito individualmente e implementado em C (C11).

Esta entrega inclui:
- **análise léxica** guiada por um AFD, que gera o vetor de tokens e registra os erros léxicos;
- **análise sintática** por descida recursiva, que segue a gramática da linguagem P, consome o vetor de tokens e registra todos os erros sintáticos;
- **tabelas de símbolos**, uma por função, com `FunctionRegister` para cada chamada de função.

A construção da ASA e a análise semântica ficam para a entrega final.

## Estrutura

```
src/
  lexico.c, lexico.h        AFD (tabela de transicoes) e vetor de tokens
  sintatico.c, sintatico.h  parser: uma funcao por nao-terminal da gramatica
  tabela.c, tabela.h        tabelas de simbolos e FunctionRegister
  util.c, util.h            alocacao, textos e lista de erros
  main.c                    leitura do arquivo, integracao das fases e saidas
docs/afd.md                 diagrama e tabela do AFD
testes/entradas/            programas de teste em P
testes/saidas/              saidas geradas para cada teste
Makefile
executar_testes.sh
```

## Instalação no EndeavourOS

Só são necessários o GCC e o `make`, do grupo `base-devel`. Não há bibliotecas externas: o código usa apenas a biblioteca padrão de C.

```bash
sudo pacman -S --needed base-devel
```

## Compilação

Dentro desta pasta:

```bash
make
```

O comando gera o executável `compilador_p`. Se preferir compilar sem o `make`:

```bash
gcc -std=c11 -Wall -Wextra -pedantic -O2 -o compilador_p src/*.c
```

## Execução

```bash
./compilador_p arquivo.p [diretorio_de_saida]
```

Se o diretório de saída for omitido, as saídas vão para `./saida/`. Cada saída fica em um arquivo próprio, onde `<base>` é o nome do arquivo de entrada sem a extensão:

| Arquivo | Conteúdo |
|---|---|
| `<base>.tokens.txt` | vetor de tokens, na ordem de reconhecimento (índice, linha, token, lexema) |
| `<base>.erros_lexicos.txt` | erros léxicos, com a linha |
| `<base>.erros_sintaticos.txt` | erros sintáticos, com a linha |
| `<base>.tabelas.txt` | tabela de símbolos de cada função, com `ret_type` e os `FunctionRegister` |

O terminal mostra um resumo com a quantidade de tokens, os erros encontrados e os caminhos dos arquivos gerados. O código de saída é 0 sem erros, 1 se houver erros léxicos ou sintáticos e 2 se houver falha ao ler ou gravar arquivos.

Para ver o AFD usado pelo analisador léxico:

```bash
./compilador_p --afd
```

### Exemplo

```bash
./compilador_p testes/entradas/exemplo4_maior.p
cat saida/exemplo4_maior.tabelas.txt
```

## Testes

```bash
make testes          # ou: bash executar_testes.sh
```

O script compila o projeto, executa cada arquivo de `testes/entradas/` e grava as saídas em `testes/saidas/<nome do teste>/`, junto com o resumo do terminal (`<nome>.resumo.txt`). As saídas incluídas na entrega foram geradas com esse script.

| Teste | O que verifica | Resultado esperado |
|---|---|---|
| `exemplo1_soma.p` | Exemplo 1 da especificação: parâmetros, retorno, chamada, `println` | sem erros |
| `exemplo2_calculadora.p` | Exemplo 2: `char`, `float`, `else if` encadeado, chamada com literal char | sem erros |
| `exemplo3_while.p` | Exemplo 3: `while` | sem erros |
| `exemplo4_maior.p` | Exemplo da seção 7.6: as tabelas geradas devem ser iguais às do enunciado | sem erros |
| `lexico_todos_tokens.p` | Todas as palavras reservadas, operadores, pontuação, constantes, literais char e string | sem erros; os 35 tipos de token aparecem no vetor |
| `lexico_erros.p` | `_invalido`, `12.`, `@`, `#`, `!` isolado, `''`, `'ab'`, `""`, string não terminada | 9 erros léxicos |
| `sintatico_erros.p` | `;` e `)` faltando, expressão incompleta, `while` sem `{`, expressão como argumento, `else` solto, `1.5e3`, `let` sem `:`, `println` sem argumentos, tokens fora de função, função sem nome | 12 erros sintáticos, cada um relatado uma vez |
| `tabela_chamadas.p` | Chamada antes da definição, chamada aninhada, chamada como comando, função `void`, função não definida, posições de parâmetros | sem erros sintáticos; `call_refs` e `FunctionRegister` preenchidos |
| `vazio.p` | Arquivo vazio | 1 erro sintático (o programa precisa ter ao menos uma função) |

## Análise léxica

O AFD está documentado em [docs/afd.md](docs/afd.md), com diagrama, tabela de estados e mensagens de erro. No código, ele é uma tabela de transições (`estado × classe de caractere`) consultada a cada caractere. As palavras reservadas são reconhecidas pelo estado de identificador e depois comparadas com a lista de palavras reservadas.

Cada `Token` guarda `lexema`, `tipo` e `linha`. O analisador lê o arquivo inteiro e devolve um vetor com todos os tokens, terminado por um token `EOF` que marca o fim para o parser. Ao encontrar um erro, ele registra a mensagem com a linha, descarta o lexema inválido e continua. Assim, todos os erros léxicos aparecem em uma única execução.

## Análise sintática

O parser é descendente recursivo e consome o vetor de tokens. Cada não-terminal da gramática da seção 5 tem uma função com o mesmo nome (`programa`, `funcao`, `funcaoSeq`, `sequencia`, `comandoSe`, `exprOpc`, `listaArgs2`, ...), com a produção escrita em comentário acima dela. As produções com recursão à direita foram mantidas recursivas, como na gramática. A precedência dos operadores vem da própria gramática (`Expr` → `Rel` → `Adicao` → `Termo` → `Fator`).

**Recuperação de erros (modo pânico):** ao encontrar um erro, o parser o registra e passa a ignorar novos erros até se recuperar. Ele se recupera no início da próxima declaração ou comando, descartando tokens até um ponto seguro: `;`, `}`, início de comando ou `fn`. Com isso, o arquivo é analisado até o fim e cada problema gera, em geral, uma única mensagem. Quando o erro é a falta de um token no fim da linha, como um `;` esquecido, ele é relatado na linha em que o token deveria estar.

## Tabelas de símbolos

Cada função definida no código gera uma tabela, guardada em um vetor de tabelas. A tabela é criada assim que o parser reconhece o `fn` e o nome da função. Cada entrada tem:
- `name` e `datatype`;
- `is_param` e `pos_param`: 0, 1, 2... para parâmetros e -1 para variáveis locais;
- `call_refs`.

O `ret_type` da função fica em um atributo separado e vale `void` quando não há `->`.

Cada chamada de função gera um `FunctionRegister` com `name`, `num_args` e `args` (o lexema de cada argumento). O registro é anexado ao `call_refs` da entrada com o nome da função chamada, dentro da tabela da função onde a chamada aparece. Na impressão, `call_refs` mostra as referências (`#1`, `#2`, ...) para os registros listados logo abaixo da tabela.

A tipagem segue a seção 7.4: `DATA_ERROR`, `DATA_INT`, `DATA_FLOAT`, `DATA_CHAR` e `DATA_VOID`.

## Decisões de interpretação da especificação

- **Comentários:** a linguagem P não define comentários, então `//` é lido como dois operadores de divisão.
- **Identificadores** seguem `[a-zA-Z][a-zA-Z0-9_]*`. Um identificador que começa com `_` gera erro léxico no `_`.
- **Literal char:** exatamente um caractere entre aspas simples, sem quebra de linha. `''` e `'ab'` são erros léxicos.
- **String de formatação:** pelo menos um caractere entre aspas duplas (`"Σ+"`), sem quebra de linha.
- **Notação científica:** não é aceita. `1.5e3` vira `FLOAT_CONST 1.5` seguido de `ID e3`, e o parser rejeita.
- **Vetor de tokens:** um lexema com erro léxico não entra no vetor. Por isso, um erro léxico pode provocar também um erro sintático na mesma linha (por exemplo, `x = 12.;` vira `x = ;`).
- **Gramática aplicada à risca:**
  - `if` e `while` não usam parênteses;
  - `println` exige a vírgula e ao menos um argumento depois da string;
  - um argumento de chamada (`Arg`) só pode ser identificador, chamada ou constante, então `soma(x + 1, y)` é erro sintático.
- **Escopo:** blocos internos não criam escopo novo. Há uma tabela por função.
- **Chaves das tabelas:** a especificação sugere um HashMap indexado pelo lexema. Aqui cada tabela é um vetor com nomes únicos, o que preserva as mesmas informações e mantém a ordem de inserção na impressão. Se um nome for declarado duas vezes, a primeira declaração é mantida, e a redeclaração será relatada pela análise semântica.
- **Tipo de uma função chamada:** na tabela de quem chama, ele é o `ret_type` da função chamada. Esse valor é preenchido depois que todas as funções foram lidas, porque uma função pode ser chamada antes de ser definida. Uma função chamada que não existe fica com `DataTypes::error`.
- **`println`:** entra na tabela como `void`, sem `FunctionRegister`, como no exemplo da seção 7.6.
- **Argumento que é uma chamada aninhada:** em `dobro(dobro(a))`, o argumento é guardado com o texto da chamada, `dobro(a)`, e a chamada interna também gera o seu próprio registro.
