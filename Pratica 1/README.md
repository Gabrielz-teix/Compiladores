# Compilador Csmall

Trabalho da disciplina de Compiladores, desenvolvido em C a partir do arquivo-base fornecido.

## O que foi implementado

O programa lê um arquivo escrito em Csmall e verifica se ele segue a especificação:

- **Análise léxica:** reconhece identificadores, palavras reservadas, constantes, operadores, delimitadores e comentários `//`. Cada token guarda o lexema e a linha em que apareceu.
- **Análise sintática:** usa descida recursiva para analisar declarações, atribuições, expressões, `print`, `if`, `else`, `while` e blocos.
- **Tabela de símbolos:** registra nome, tipo e linha da declaração e mantém uma tabela para cada escopo. Um bloco pode ocultar uma variável de um escopo externo.
- **Análise semântica:** verifica declaração antes do uso, redeclaração no mesmo escopo e compatibilidade de tipos das atribuições.

Quando encontra um problema, o programa mostra o tipo de erro e a linha e encerra a análise. Ele **não executa** o código Csmall: apenas verifica sua validade.

## Arquivos

- `csmall.c`: implementação completa do compilador.
- `exemplo_valido.csmall`: exemplo com declarações, expressões, `if`, `while` e escopos.
- `testes/`: entradas válidas e inválidas para conferir o comportamento.
- `testar.py`: roda os testes e confere se cada arquivo foi aceito ou rejeitado como esperado.

## Como compilar

É necessário ter o GCC instalado. No terminal, dentro desta pasta:

**Windows (PowerShell):**

```powershell
gcc -std=c11 -Wall -Wextra -o csmall.exe csmall.c
.\csmall.exe exemplo_valido.csmall
```

**Linux ou macOS:**

```bash
gcc -std=c11 -Wall -Wextra -o csmall csmall.c
./csmall exemplo_valido.csmall
```

Um arquivo correto mostra:

```text
Programa Csmall valido: analise concluida sem erros.
```

Para listar os tokens em vez de analisar o programa inteiro:

```powershell
.\csmall.exe --tokens exemplo_valido.csmall
```

No Linux ou macOS, use `./csmall` no lugar de `.\csmall.exe`.

## Como rodar os testes

Após compilar, execute:

```powershell
python testar.py
```

O script utiliza somente a biblioteca padrão do Python. Ele encontra `csmall.exe` ou `csmall` na pasta atual. Os testes cobrem os exemplos do enunciado e casos adicionais, incluindo escopos e erros de sintaxe.

## Algumas decisões da implementação

As declarações ficam no começo do programa ou de cada bloco, como exige a gramática. Os corpos de `if`, `else` e `while` exigem chaves. Uma expressão com `int` e `float` tem resultado `float`, mas uma atribuição exige tipos exatamente iguais. Não há menos unário nem conversão implícita de tipos.

O analisador encerra na primeira falha. A tabela usa listas encadeadas, e cada bloco cria um escopo novo, que é descartado na saída.
