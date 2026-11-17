/*
 * Front-end do compilador da linguagem P: analise lexica, analise sintatica
 * e tabelas de simbolos.
 *
 * Uso:
 *   ./compilador_p arquivo.p [diretorio_de_saida]   (padrao: ./saida)
 *   ./compilador_p --afd                            (mostra o AFD do lexico)
 *
 * Saidas (um arquivo para cada uma), com <base> = nome do arquivo sem extensao:
 *   <base>.tokens.txt            vetor de tokens
 *   <base>.erros_lexicos.txt     erros lexicos
 *   <base>.erros_sintaticos.txt  erros sintaticos
 *   <base>.tabelas.txt           tabelas de simbolos de todas as funcoes
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define criarDiretorio(caminho) _mkdir(caminho)
#else
#define criarDiretorio(caminho) mkdir((caminho), 0755)
#endif

#include "lexico.h"
#include "sintatico.h"
#include "tabela.h"
#include "util.h"

static char *lerArquivo(const char *nome, size_t *tamanho)
{
    FILE *arquivo = fopen(nome, "rb");
    if (!arquivo) {
        fprintf(stderr, "Nao foi possivel abrir '%s': %s\n", nome, strerror(errno));
        return NULL;
    }
    size_t capacidade = 4096, usado = 0;
    char *conteudo = alocar(capacidade);
    size_t lidos;
    while ((lidos = fread(conteudo + usado, 1, capacidade - usado, arquivo)) > 0) {
        usado += lidos;
        if (usado == capacidade) {
            capacidade *= 2;
            conteudo = realocar(conteudo, capacidade);
        }
    }
    int falhou = ferror(arquivo);
    fclose(arquivo);
    if (falhou) {
        fprintf(stderr, "Erro ao ler '%s'.\n", nome);
        free(conteudo);
        return NULL;
    }
    *tamanho = usado;
    return conteudo;
}

/* "testes/entradas/exemplo.p" -> "exemplo" */
static char *nomeBase(const char *caminho)
{
    const char *inicio = caminho;
    for (const char *c = caminho; *c; c++)
        if (*c == '/' || *c == '\\') inicio = c + 1;
    const char *ponto = strrchr(inicio, '.');
    size_t tamanho = ponto && ponto != inicio ? (size_t)(ponto - inicio) : strlen(inicio);
    return copiarTrecho(inicio, tamanho);
}

static FILE *abrirSaida(const char *diretorio, const char *base, const char *sufixo, char **caminho)
{
    *caminho = formatarTexto("%s/%s.%s", diretorio, base, sufixo);
    FILE *arquivo = fopen(*caminho, "w");
    if (!arquivo) fprintf(stderr, "Nao foi possivel criar '%s': %s\n", *caminho, strerror(errno));
    return arquivo;
}

static void imprimirErros(FILE *saida, const char *titulo, const ListaErros *erros)
{
    fprintf(saida, "%s (%zu)\n\n", titulo, erros->quantidade);
    if (erros->quantidade == 0) fprintf(saida, "Nenhum erro encontrado.\n");
    for (size_t i = 0; i < erros->quantidade; i++)
        fprintf(saida, "Linha %d: %s\n", erros->itens[i].linha, erros->itens[i].mensagem);
}

typedef enum { SAIDA_TOKENS, SAIDA_LEXICOS, SAIDA_SINTATICOS, SAIDA_TABELAS, NUM_SAIDAS } TipoSaida;

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--afd") == 0) {
        imprimirAFD(stdout);
        return EXIT_SUCCESS;
    }
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Uso: %s arquivo.p [diretorio_de_saida]\n", argv[0]);
        fprintf(stderr, "     %s --afd\n", argv[0]);
        return 2;
    }
    const char *entrada = argv[1];
    const char *diretorio = argc == 3 ? argv[2] : "saida";

    size_t tamanho;
    char *fonte = lerArquivo(entrada, &tamanho);
    if (!fonte) return 2;

    /* Fase lexica -> vetor de tokens -> fase sintatica + tabelas. */
    VetorTokens tokens = {0};
    ListaErros errosLexicos = {0}, errosSintaticos = {0};
    ConjuntoTabelas tabelas = {0};

    analisarLexico(fonte, tamanho, &tokens, &errosLexicos);
    analisarSintatico(&tokens, &errosSintaticos, &tabelas);
    resolverTiposDasChamadas(&tabelas);

    if (criarDiretorio(diretorio) != 0 && errno != EEXIST) {
        fprintf(stderr, "Nao foi possivel criar o diretorio '%s': %s\n", diretorio, strerror(errno));
        return 2;
    }

    static const char *sufixos[NUM_SAIDAS] = {
        "tokens.txt", "erros_lexicos.txt", "erros_sintaticos.txt", "tabelas.txt"
    };
    char *base = nomeBase(entrada);
    char *caminhos[NUM_SAIDAS] = {0};
    int ok = 1;
    for (int s = 0; s < NUM_SAIDAS && ok; s++) {
        FILE *arquivo = abrirSaida(diretorio, base, sufixos[s], &caminhos[s]);
        if (!arquivo) { ok = 0; break; }
        switch ((TipoSaida)s) {
            case SAIDA_TOKENS: imprimirTokens(arquivo, &tokens); break;
            case SAIDA_LEXICOS: imprimirErros(arquivo, "ERROS LEXICOS", &errosLexicos); break;
            case SAIDA_SINTATICOS: imprimirErros(arquivo, "ERROS SINTATICOS", &errosSintaticos); break;
            case SAIDA_TABELAS: imprimirTabelas(arquivo, &tabelas); break;
            default: break;
        }
        if (fclose(arquivo) != 0) ok = 0;
    }

    /* Resumo no terminal. */
    printf("Arquivo analisado: %s\n", entrada);
    printf("  tokens reconhecidos: %zu (incluindo EOF)\n", tokens.quantidade);
    printf("  erros lexicos:       %zu\n", errosLexicos.quantidade);
    for (size_t i = 0; i < errosLexicos.quantidade; i++)
        printf("    linha %d: %s\n", errosLexicos.itens[i].linha, errosLexicos.itens[i].mensagem);
    printf("  erros sintaticos:    %zu\n", errosSintaticos.quantidade);
    for (size_t i = 0; i < errosSintaticos.quantidade; i++)
        printf("    linha %d: %s\n", errosSintaticos.itens[i].linha, errosSintaticos.itens[i].mensagem);
    printf("  tabelas de simbolos: %zu\n", tabelas.quantidade);
    if (ok) {
        printf("Saidas gravadas:\n");
        for (int s = 0; s < NUM_SAIDAS; s++) printf("  %s\n", caminhos[s]);
    }

    int temErros = errosLexicos.quantidade > 0 || errosSintaticos.quantidade > 0;
    for (int s = 0; s < NUM_SAIDAS; s++) free(caminhos[s]);
    free(base);
    free(fonte);
    liberarTokens(&tokens);
    liberarErros(&errosLexicos);
    liberarErros(&errosSintaticos);
    liberarTabelas(&tabelas);

    if (!ok) return 2;
    return temErros ? EXIT_FAILURE : EXIT_SUCCESS;
}
