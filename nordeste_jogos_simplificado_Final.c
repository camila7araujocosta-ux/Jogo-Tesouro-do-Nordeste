git /*
 * Nordeste Jogos - Quiz do Nordeste + Tesouro do Nordeste
 * Compilar: gcc -Wall -Wextra -std=c11 -o nordeste_jogos nordeste_jogos.c
 * Rodar:    ./nordeste_jogos
 *
 * COMO ESTE ARQUIVO ESTA ORGANIZADO (de cima para baixo):
 *   1. Constantes, cores e funcoes utilitarias (leitura de teclado, minusculas)
 *   2. Scoreboard: placar salvo em arquivo binario, ordenado por pontos e tempo
 *   3. Jogo 1 - Quiz do Nordeste: 5 etapas x 3 perguntas, com cronometro
 *   4. Jogo 2 - Tesouro do Nordeste: aventura em texto com mapa, itens, agua e fases do dia
 *   5. main(): menu principal que chama os dois jogos
 *
 * Os comentarios que comecam com // foram adicionados para explicar o codigo;
 * eles nao alteram o comportamento do programa.
 */
// Bibliotecas usadas:
//   ctype.h  -> tolower (converter letras)    stdio.h  -> printf, fgets, FILE, fopen
//   stdlib.h -> exit, rand, srand             string.h -> strlen, strcmp, memset
//   time.h   -> time, difftime (cronometro e semente do aleatorio)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Constantes de configuracao: mudar aqui ajusta o jogo sem mexer na logica.
#define ARQUIVO_SCORE  "scoreboard_nordeste.dat"  // arquivo binario onde o placar e salvo
#define MAX_SCORES     20  // quantos registros o placar guarda (top 20)
#define MAX_NOME       24  // tamanho maximo do nome (inclui o '\0' final)
#define MAX_ITENS      6  // capacidade da mochila no Tesouro
#define AGUA_INICIAL   8  // agua inicial (e maxima, no comeco) do jogador
#define PASSOS_FASE    3  // passos necessarios para o dia mudar de fase

// Variavel global (prefixo g_): 1 = cores ligadas, 0 = desligadas.
// O comando 'cores' dentro do Tesouro alterna esse valor.
static int g_cores = 1;

// As cores usam codigos ANSI de terminal: ESC[<codigo>m muda a cor do texto
// e ESC[0m volta ao normal (ESC e o caractere de escape, escrito como \033).
// Exemplos de codigo: 93 = amarelo claro, 91 = vermelho claro, 1;92 = verde em negrito.
// cor() liga a cor e fim_cor() desliga; ambas respeitam g_cores.
static void cor(const char *codigo) {
    if (g_cores) printf("\033[%sm", codigo);
}

static void fim_cor(void) {
    if (g_cores) printf("\033[0m");
}

// Um registro do placar: quem jogou, qual jogo, pontos e tempo gasto.
// A struct e gravada inteira no arquivo binario (fwrite) e lida de volta (fread).
typedef struct {
    char nome[MAX_NOME];
    char jogo[24];
    int pontos;
    double segundos;
} RegistroScore;

// O placar: vetor de tamanho fixo + contador (qtd) de quantos registros estao em uso.
// Tamanho fixo evita malloc/free e simplifica o codigo.
typedef struct {
    RegistroScore itens[MAX_SCORES];
    int qtd;
} Scoreboard;

// fgets guarda o '\n' do ENTER no fim do texto lido. Aqui removemos '\n' e '\r'
// (o '\r' aparece em arquivos/terminais do Windows), de tras para frente.
static void limpar_newline(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

// Le uma linha do teclado com seguranca: fgets nunca escreve alem de n bytes (diferente
// de gets ou scanf('%s')). Se a entrada acabar (Ctrl+D / EOF), encerra o programa
// de forma limpa em vez de ficar num laco infinito.
static void ler_linha(char *buf, size_t n) {
    if (!fgets(buf, (int)n, stdin)) {
        printf("\nAte a proxima!\n");
        exit(0);
    }
    limpar_newline(buf);
}

// Pausa ate o jogador apertar ENTER (usado entre as etapas do quiz).
static void esperar_enter(const char *msg) {
    char buf[16];
    printf("\n%s", msg);
    ler_linha(buf, sizeof buf);
}

// Converte o texto para minusculas, assim 'IR', 'Ir' e 'ir' sao o mesmo comando.
// O cast para unsigned char evita comportamento indefinido do tolower com valores negativos.
static void minusculas(char *s) {
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

/* =========================================================================
 * SCOREBOARD POR VELOCIDADE
 * ========================================================================= */

// Le o arquivo binario de uma vez com fread. Se o arquivo nao existir (primeira vez
// que o jogo roda), fopen falha e o placar simplesmente comeca vazio.
static void carregar_scoreboard(Scoreboard *sb) {
    sb->qtd = 0;
    FILE *f = fopen(ARQUIVO_SCORE, "rb");
    if (!f) return;
    sb->qtd = (int)fread(sb->itens, sizeof(RegistroScore), MAX_SCORES, f);
    fclose(f);
}

// Grava os registros em uso no arquivo. O modo 'wb' sobrescreve o arquivo inteiro.
static void salvar_scoreboard(const Scoreboard *sb) {
    FILE *f = fopen(ARQUIVO_SCORE, "wb");
    if (!f) return;
    fwrite(sb->itens, sizeof(RegistroScore), (size_t)sb->qtd, f);
    fclose(f);
}

// Ordenacao simples por troca (O(n^2)): com no maximo 20 registros e mais que suficiente.
// Criterio: mais pontos primeiro; em caso de empate, vence quem fez em menos segundos.
static void ordenar_scoreboard(Scoreboard *sb) {
    for (int i = 0; i < sb->qtd; i++) {
        for (int j = i + 1; j < sb->qtd; j++) {
            int troca = 0;
            if (sb->itens[j].pontos > sb->itens[i].pontos) troca = 1;
            else if (sb->itens[j].pontos == sb->itens[i].pontos &&
                     sb->itens[j].segundos < sb->itens[i].segundos) troca = 1;
            if (troca) {
                RegistroScore tmp = sb->itens[i];
                sb->itens[i] = sb->itens[j];
                sb->itens[j] = tmp;
            }
        }
    }
}

// Calcula a pontuacao. A ideia: acertar/vencer da a base, e ser rapido da um bonus.
//   Quiz   : 200 por acerto + ate 800 * bonus, onde bonus = 90 / (segundos + 15), limitado a 2.
//   Tesouro: 1000 fixos + 1000 * bonus, onde bonus = 120 / (segundos + 20), limitado a 2.2.
// Quanto mais segundos, menor o bonus (a divisao diminui). O limite impede pontuacao
// absurda em tempos muito curtos, e 'seg < 1' evita divisao por valores minusculos.
static int calcular_pontos(int is_quiz, int acertos, int max_acertos, double seg) {
    if (seg < 1.0) seg = 1.0;
    if (is_quiz) {
        double ratio = (max_acertos > 0) ? ((double)acertos / max_acertos) : 0.0;
        double bonus = 90.0 / (seg + 15.0);
        if (bonus > 2.0) bonus = 2.0;
        return acertos * 200 + (int)(ratio * 800.0 * bonus);
    } else {
        double bonus = 120.0 / (seg + 20.0);
        if (bonus > 2.2) bonus = 2.2;
        return 1000 + (int)(1000.0 * bonus);
    }
}

// Pede o nome, monta o registro e insere no placar. Com o placar cheio (20), o novo
// resultado so entra se for melhor que o ultimo colocado (ele e substituido).
static void salvar_resultado(Scoreboard *sb, const char *jogo, int pontos, double segundos) {
    char nome[MAX_NOME];
    printf("\nTempo: %.1fs | Pontos: %d\n", segundos, pontos);
    printf("Seu nome (ou ENTER para 'Viajante'): ");
    ler_linha(nome, sizeof nome);

    RegistroScore r;
    // Zera a struct inteira para nao gravar 'lixo' de memoria no arquivo.
    memset(&r, 0, sizeof r);
    // snprintf copia sem estourar o tamanho do campo. Se o nome vier vazio (so ENTER),
    // usa 'Viajante' (operador ternario: condicao ? valor_se_verdadeiro : valor_se_falso).
    snprintf(r.nome, sizeof r.nome, "%s", nome[0] ? nome : "Viajante");
    snprintf(r.jogo, sizeof r.jogo, "%s", jogo);
    r.pontos = pontos;
    r.segundos = segundos;

    if (sb->qtd < MAX_SCORES) {
        sb->itens[sb->qtd++] = r;
    } else {
        ordenar_scoreboard(sb);
        if (r.pontos > sb->itens[sb->qtd - 1].pontos ||
            (r.pontos == sb->itens[sb->qtd - 1].pontos && r.segundos < sb->itens[sb->qtd - 1].segundos)) {
            sb->itens[sb->qtd - 1] = r;
        }
    }
    ordenar_scoreboard(sb);
    salvar_scoreboard(sb);
    printf("Pontuacao salva!\n");
}

// Imprime a tabela. No printf, '%-18.18s' alinha o texto a esquerda numa coluna de 18
// caracteres e corta o que passar disso, mantendo as colunas retas.
static void mostrar_scoreboard(const Scoreboard *sb, int limite_max) {
    printf("\n=== SCOREBOARD (RAPIDEZ NO NORDESTE) ===\n");
    if (sb->qtd == 0) {
        printf("Nenhuma partida registrada ainda.\n");
        return;
    }
    printf("%-4s %-18s %-20s %7s %7s\n", "POS", "JOGADOR", "JOGO", "TEMPO", "PONTOS");
    printf("----------------------------------------------------------------\n");
    int lim = sb->qtd < limite_max ? sb->qtd : limite_max;
    for (int i = 0; i < lim; i++) {
        printf("#%-3d %-18.18s %-20.20s %6.1fs %7d\n",
               i + 1, sb->itens[i].nome, sb->itens[i].jogo,
               sb->itens[i].segundos, sb->itens[i].pontos);
    }
}

/* =========================================================================
 * JOGO 1: QUIZ DO NORDESTE
 * ========================================================================= */

// Uma pergunta: enunciado, 4 alternativas, indice (0 a 3) da alternativa correta
// e uma curiosidade mostrada depois que o jogador responde.
typedef struct {
    const char *pergunta;
    const char *opcoes[4];
    int certa;
    const char *curiosidade;
} PerguntaQuiz;

// Uma etapa do quiz = um lugar do Nordeste (nome, descricao, cor e arte ASCII)
// com 3 perguntas sobre ele.
typedef struct {
    const char *local;
    const char *descricao;
    const char *cor_arte;
    const char *arte[5];
    PerguntaQuiz perguntas[3];
} EtapaQuiz;

// Os DADOS do quiz ficam separados da LOGICA: para criar perguntas novas basta editar
// esta tabela. O 'const' coloca os dados em area somente leitura (nao mudam durante o jogo).
// Atencao: 'certa' e o indice da resposta certa dentro de 'opcoes'. Na tela, as alternativas
// sao embaralhadas (veja embaralhar_ordem), entao a resposta certa muda de posicao.
static const EtapaQuiz ETAPAS[5] = {
    {
        "Feira de Caruaru",
        "Barracas coloridas, cordel pendurado e uma fonte no meio da praca.",
        "95",
        {
            "^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v",
            " /\\/\\/\\/\\/\\        '.'       /\\/\\/\\/\\/\\ ",
            " |~~~~~~~~~~|    .-~~~-.     |~~~~~~~~~~",
            " | [] []    |   (  ~~~  )    |  o  o    ",
            "========================================",
        },
        {
            { "Em qual estado fica a cidade de Caruaru?",
              { "Pernambuco", "Bahia", "Ceara", "Paraiba" }, 0,
              "Caruaru fica no Agreste de Pernambuco." },
            { "Qual literatura popular e vendida pendurada em barbantes?",
              { "Cordel", "Romance", "Cronica", "Haicai" }, 0,
              "O cordel ganha esse nome por ficar exposto em cordas." },
            { "Qual festa junina tem forro, fogueira e quadrilha?",
              { "Sao Joao", "Carnaval", "Natal", "Ano Novo" }, 0,
              "Sao Joao e um dos maiores destaques culturais do Nordeste." }
        }
    },
    {
        "Casa de Farinha",
        "Cheiro de mandioca no ar. Aqui nasce a farinha do Nordeste.",
        "33",
        {
            "           ) (                          ",
            "       ___(___)___                      ",
            "      /____________\\       \\|/   \\|/    ",
            "      | [] |##| [] |        |     |     ",
            "========================================",
        },
        {
            { "A farinha feita nas casas de farinha vem de qual raiz?",
              { "Mandioca", "Batata-doce", "Inhame", "Cenoura" }, 0,
              "Tambem chamada de macaxeira ou aipim conforme a regiao." },
            { "Qual prato nordestino e feito com a goma da mandioca?",
              { "Tapioca", "Pamonha", "Coxinha", "Brigadeiro" }, 0,
              "A goma vai a chapa quente e vira tapioca." },
            { "Quantos estados formam a regiao Nordeste?",
              { "7", "8", "9", "10" }, 2,
              "Sao 9 estados: MA, PI, CE, RN, PB, PE, AL, SE e BA." }
        }
    },
    {
        "Sertao",
        "Caatinga a perder de vista, sol forte e mandacaru.",
        "93",
        {
            "       _                ~v~             ",
            "   _  | |  _                            ",
            "  | |_| |_| |                           ",
            "  |_  | |  _|                  _        ",
            ",.-'`-.,_,.-'`-.,_,.-'`-.,_,.-'`-.,_,.-'",
        },
        {
            { "Qual bioma exclusivo do Brasil predomina no sertao?",
              { "Caatinga", "Pampa", "Pantanal", "Mata Atlantica" }, 0,
              "Caatinga significa 'mata branca' em tupi." },
            { "Qual cacto guarda agua e floresce a noite no sertao?",
              { "Mandacaru", "Vitoria-regia", "Araucaria", "Seringueira" }, 0,
              "As flores do mandacaru se abrem durante a noite." },
            { "Qual rio, chamado de Velho Chico, passa pelo sertao?",
              { "Sao Francisco", "Amazonas", "Parana", "Tocantins" }, 0,
              "O Sao Francisco corta varios estados ate o Atlantico." }
        }
    },
    {
        "Chapada Diamantina",
        "Paredoes de pedra, rios e cachoeiras na Bahia.",
        "31",
        {
            "          ____________________          ",
            " |        \\      |::::|      /        | ",
            " |         \\     |::::|     /         | ",
            " |          |    |::::|    |    .--.  | ",
            "===========~~~~~~~~~~~~~~~~~~===========",
        },
        {
            { "Em qual estado fica a Chapada Diamantina?",
              { "Bahia", "Maranhao", "Sergipe", "Piaui" }, 0,
              "Fica no centro da Bahia e e famosa pelo ecoturismo." },
            { "De onde vem o nome 'Diamantina'?",
              { "Dos diamantes garimpados", "Das praias brancas",
                "Do petroleo", "Do cacau" }, 0,
              "O garimpo de diamantes marcou a historia da regiao." },
            { "Qual paisagem e marcante na Chapada Diamantina?",
              { "Serras, cachoeiras e cavernas", "Geleiras e neve",
                "Dunas e mangues", "Campos e pampas" }, 0,
              "Abriga trilhas, grutas e cachoeiras altas." }
        }
    },
    {
        "Gruta do Tesouro",
        "Um bau cheio de ouro aguarda quem conhece a cultura nordestina.",
        "1;93",
        {
            "                                        ",
            "                 _______                ",
            "                /_______\\               ",
            "     ^         |  [$$$]  |       ^      ",
            "==========o O o 0 o O o 0 o O o=========",
        },
        {
            { "Luiz Gonzaga ficou conhecido como o Rei do...?",
              { "Baiao", "Samba", "Rock", "Funk" }, 0,
              "Luiz Gonzaga popularizou o baiao e o forro em todo o pais." },
            { "Qual danca pernambucana usa sombrinhas coloridas?",
              { "Frevo", "Tango", "Valsa", "Bolero" }, 0,
              "O frevo nasceu no Recife e e Patrimonio da Humanidade." },
            { "O acaraje e prato simbolo de qual estado?",
              { "Bahia", "Santa Catarina", "Parana", "Rio Grande do Sul" }, 0,
              "O oficio das baianas de acaraje e patrimonio cultural." }
        }
    }
};

// Desenha a moldura com 5 linhas de arte, aplicando a cor so ao desenho (nao a moldura).
// Na arte, cada barra invertida e escrita duas vezes porque, em strings de C,
// uma barra sozinha e caractere de escape.
static void desenhar_arte_5_linhas(const char *titulo, const char *cor_cenario, const char *const arte[5]) {
    printf("\n+--- %s ---+\n", titulo);
    for (int i = 0; i < 5; i++) {
        putchar('|');
        cor(cor_cenario);
        printf("%s", arte[i]);
        fim_cor();
        printf("|\n");
    }
    printf("+------------------------------------------+\n");
}

// Repete a pergunta ate o jogador digitar 1, 2, 3 ou 4 (validacao de entrada).
// 'buf[0] - '0'' transforma o caractere '3' no numero 3.
static int ler_opcao_1_a_4(void) {
    char buf[32];
    while (1) {
        printf("\nResposta (1-4): ");
        ler_linha(buf, sizeof buf);
        if (buf[0] >= '1' && buf[0] <= '4') return buf[0] - '0';
        printf("Digite um numero de 1 a 4.\n");
    }
}

// Embaralha a ordem em que as 4 alternativas aparecem (algoritmo de Fisher-Yates).
// 'ordem' guarda, para cada posicao da tela, o indice da alternativa original:
// ordem[0] == 2 significa que a 1a opcao mostrada e a alternativa de indice 2 em ETAPAS.
// Assim a resposta certa muda de lugar a cada pergunta, sem mexer nos dados do quiz.
static void embaralhar_ordem(int ordem[4]) {
    for (int i = 0; i < 4; i++) ordem[i] = i;
    for (int i = 3; i > 0; i--) {
        int k = rand() % (i + 1);   // sorteia uma posicao entre 0 e i
        int tmp = ordem[i];
        ordem[i] = ordem[k];
        ordem[k] = tmp;
    }
}

// Fluxo do quiz: inicia o cronometro -> 5 etapas x 3 perguntas -> calcula pontos -> salva.
// time(NULL) devolve o horario atual em segundos; a diferenca entre dois horarios
// (difftime) e o tempo de jogo.
static void rodar_quiz(Scoreboard *sb) {
    printf("\n=== QUIZ DO NORDESTE ===\n");
    printf("5 etapas, 3 perguntas por etapa. Responda rapido para ganhar mais pontos!\n");
    esperar_enter("Pressione ENTER para iniciar o cronometro...");

    time_t inicio = time(NULL);
    int total_acertos = 0;

    for (int e = 0; e < 5; e++) {
        const EtapaQuiz *et = &ETAPAS[e];
        desenhar_arte_5_linhas(et->local, et->cor_arte, et->arte);
        printf("Etapa %d/5 - %s\n%s\n", e + 1, et->local, et->descricao);

        int acertos_etapa = 0;
        for (int q = 0; q < 3; q++) {
            const PerguntaQuiz *pq = &et->perguntas[q];
            printf("\nPergunta %d: %s\n", q + 1, pq->pergunta);

            // Sorteia a ordem de exibicao e anota em que posicao da tela ficou a resposta certa.
            int ordem[4];
            embaralhar_ordem(ordem);
            int pos_certa = 0;
            for (int i = 0; i < 4; i++) {
                printf("  %d) %s\n", i + 1, pq->opcoes[ordem[i]]);
                if (ordem[i] == pq->certa) pos_certa = i;
            }

            // O '- 1' converte a resposta digitada (1 a 4) para indice do vetor (0 a 3).
            int resp = ler_opcao_1_a_4() - 1;
            if (resp == pos_certa) {
                cor("1;92"); printf("[ACERTOU!] "); fim_cor();
                acertos_etapa++;
            } else {
                cor("1;91"); printf("[ERROU] "); fim_cor();
                printf("Certa: %d) %s. ", pos_certa + 1, pq->opcoes[pq->certa]);
            }
            printf("Curiosidade: %s\n", pq->curiosidade);
        }

        total_acertos += acertos_etapa;
        printf("\nAcertos nesta etapa: %d/3 | Total: %d/15\n", acertos_etapa, total_acertos);
        if (e < 4) esperar_enter("Pressione ENTER para ir ao proximo lugar...");
    }

    // Fim do quiz: tempo total (fim - inicio) e pontuacao final.
    double seg = difftime(time(NULL), inicio);
    int pts = calcular_pontos(1, total_acertos, 15, seg);

    printf("\n*** FIM DO QUIZ! Voce acertou %d de 15 perguntas. ***\n", total_acertos);
    if (total_acertos >= 13)      printf("Titulo: MESTRE DO NORDESTE!\n");
    else if (total_acertos >= 9)  printf("Titulo: Grande conhecedor do Nordeste!\n");
    else                          printf("Titulo: Bom viajante!\n");

    salvar_resultado(sb, "Quiz do Nordeste", pts, seg);
}

/* =========================================================================
 * JOGO 2: TESOURO DO NORDESTE
 * ========================================================================= */

// enum da nomes legiveis a numeros: LOC_CASA = 0, LOC_SERTAO = 1, ... O ultimo (NUM_LOC)
// vira automaticamente a quantidade de locais, truque comum para dimensionar vetores.
// ITEM_NENHUM = -1 significa 'sem item' (o -1 nunca e um indice valido de vetor).
enum { LOC_CASA, LOC_SERTAO, LOC_FEIRA, LOC_CHAPADA, LOC_TESOURO, NUM_LOC };
enum { DIR_NORTE, DIR_SUL, DIR_LESTE, DIR_OESTE, NUM_DIR };
enum { ITEM_NENHUM = -1, ITEM_MAPA = 0, ITEM_CORDA, ITEM_CHAVE, ITEM_LAMPIAO, ITEM_CANTIL, NUM_ITENS };

// Vetores de nomes indexados pelos enums: NOMES_ITENS[ITEM_CORDA] e 'corda'.
// Servem para imprimir na tela e para comparar com o texto que o jogador digitou.
// CORES_FASES guarda a cor de cada fase do dia (manha, tarde, noite).
static const char *NOMES_DIR[NUM_DIR]     = { "norte", "sul", "leste", "oeste" };
static const char *NOMES_ITENS[NUM_ITENS] = { "mapa", "corda", "chave", "lampiao", "cantil" };
static const char *NOMES_FASES[3]       = { "Manha", "Tarde", "Noite" };
static const char *CORES_FASES[3]       = { "93", "91", "94" };

// Um local do mapa. Os locais formam um GRAFO: 'saidas' guarda, para cada direcao,
// o indice do local vizinho (-1 = sem saida).
//   item_chao   -> aparece no local e e pego com 'pegar'
//   item_oculto -> so aparece com 'explorar'
//   exige_item  -> item que precisa estar na mochila para ENTRAR no local
//   tem_fonte   -> 1 se o local reabastece a agua
//   explorado   -> 1 depois que o jogador ja explorou (o 'explorar' so vale uma vez)
typedef struct {
    const char *nome;
    const char *descricao;
    const char *dica_explorar;
    const char *cor_arte;
    const char *arte[5];
    int saidas[NUM_DIR];
    int item_chao;
    int item_oculto;
    int exige_item;
    int tem_fonte;
    int explorado;
} LocalJogo;

// Estado do jogador durante a partida (tudo o que muda enquanto se joga):
// onde esta, agua atual e maxima, passos dados (controlam a fase do dia), mochila,
// e duas flags: se o bau ja foi aberto e se o atalho da Chapada ja foi usado.
typedef struct {
    int local;
    int agua;
    int agua_max;
    int passos;
    int mochila[MAX_ITENS];
    int qtd_itens;
    int bau_aberto;
    int atalho_chapada;
} Jogador;

// Monta o mapa. Cada local e um 'compound literal' (LocalJogo){...}, com os valores
// na MESMA ORDEM dos campos da struct. E uma funcao (e nao uma tabela const) porque
// item_chao, item_oculto e explorado mudam durante o jogo; assim cada partida
// comeca com um mapa novo.
static void criar_locais(LocalJogo loc[NUM_LOC]) {
    loc[LOC_CASA] = (LocalJogo){
        "Casa de Farinha",
        "Casa antiga com cheiro de mandioca.",
        "Atras dos sacos de farinha voce acha um LAMPIAO!",
        "33",
        {
            "           ) (                          ",
            "       ___(___)___                      ",
            "      /____________\\       \\|/   \\|/    ",
            "      | [] |##| [] |        |     |     ",
            "========================================"
        },
        { LOC_SERTAO, -1, LOC_FEIRA, -1 }, ITEM_MAPA, ITEM_LAMPIAO, ITEM_NENHUM, 0, 0
    };
    loc[LOC_SERTAO] = (LocalJogo){
        "Sertao",
        "Caatinga, mandacaru e sol quente.",
        "Perto de um juazeiro voce encontra agua fresca (+2 agua)!",
        "93",
        {
            "       _                ~v~             ",
            "   _  | |  _                            ",
            "  | |_| |_| |                           ",
            "  |_  | |  _|                  _        ",
            ",.-'`-.,_,.-'`-.,_,.-'`-.,_,.-'`-.,_,.-'"
        },
        { -1, LOC_CASA, LOC_CHAPADA, -1 }, ITEM_NENHUM, ITEM_NENHUM, ITEM_NENHUM, 0, 0
    };
    loc[LOC_FEIRA] = (LocalJogo){
        "Feira de Caruaru",
        "Barracas coloridas, cordel e uma fonte de agua.",
        "Dica do cordelista: 'Use a corda na Chapada e a chave na Gruta.'",
        "95",
        {
            "^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v^v",
            " /\\/\\/\\/\\/\\        '.'       /\\/\\/\\/\\/\\ ",
            " |~~~~~~~~~~|    .-~~~-.     |~~~~~~~~~~",
            " | [] []    |   (  ~~~  )    |  o  o    ",
            "========================================"
        },
        { -1, -1, -1, LOC_CASA }, ITEM_CORDA, ITEM_NENHUM, ITEM_NENHUM, 1, 0
    };
    loc[LOC_CHAPADA] = (LocalJogo){
        "Chapada Diamantina",
        "Paredoes de pedra e uma fenda estreita.",
        "Ha marcas de escalada descendo por uma fenda na pedra.",
        "31",
        {
            "          ____________________          ",
            " |        \\      |::::|      /        | ",
            " |         \\     |::::|     /         | ",
            " |          |    |::::|    |    .--.  | ",
            "===========~~~~~~~~~~~~~~~~~~==========="
        },
        { LOC_TESOURO, -1, -1, LOC_SERTAO }, ITEM_CHAVE, ITEM_NENHUM, ITEM_CORDA, 0, 0
    };
    loc[LOC_TESOURO] = (LocalJogo){
        "Gruta do Tesouro",
        "Um bau enorme trancado com cadeado.",
        "O cadeado pede uma chave.",
        "1;93",
        {
            "                                        ",
            "                 _______                ",
            "                /_______\\               ",
            "     ^         |  [$$$]  |       ^      ",
            "==========o O o 0 o O o 0 o O o========="
        },
        { -1, LOC_CHAPADA, -1, -1 }, ITEM_NENHUM, ITEM_NENHUM, ITEM_CHAVE, 0, 0
    };
}

// A cada PASSOS_FASE passos a fase avanca. O '% 3' faz o dia recomecar:
// 0 = manha, 1 = tarde, 2 = noite, 0 = manha de novo, e assim por diante.
static int fase_atual(const Jogador *j) {
    return (j->passos / PASSOS_FASE) % 3;
}

// Busca linear na mochila: devolve 1 se o item esta la, 0 se nao.
static int tem_item(const Jogador *j, int item) {
    for (int i = 0; i < j->qtd_itens; i++) {
        if (j->mochila[i] == item) return 1;
    }
    return 0;
}

// Guarda o item na mochila, mas so se ele existe, ainda nao esta la e ha espaco.
static void dar_item(Jogador *j, int item) {
    if (item != ITEM_NENHUM && !tem_item(j, item) && j->qtd_itens < MAX_ITENS) {
        j->mochila[j->qtd_itens++] = item;
    }
}

// Soma (ou subtrai, se delta for negativo) agua sem passar do maximo.
static void ajustar_agua(Jogador *j, int delta) {
    j->agua += delta;
    if (j->agua > j->agua_max) j->agua = j->agua_max;
}

// Desenha a cena do local e o painel de status: agua, fase do dia, descricao,
// item no chao e saidas. A agua fica vermelha quando esta em 2 ou menos.
static void mostrar_local(const Jogador *j, const LocalJogo loc[NUM_LOC]) {
    int f = fase_atual(j);
    const LocalJogo *l = &loc[j->local];
    char titulo[64];
    snprintf(titulo, sizeof titulo, "%s | %s", l->nome, NOMES_FASES[f]);
    desenhar_arte_5_linhas(titulo, l->cor_arte, l->arte);

    printf("Agua: ");
    cor(j->agua <= 2 ? "1;91" : "96");
    printf("%d/%d", j->agua, j->agua_max);
    fim_cor();
    printf(" | Fase: ");
    cor(CORES_FASES[f]);
    printf("%s", NOMES_FASES[f]);
    fim_cor();
    printf(" | %s\n", l->descricao);

    if (l->item_chao != ITEM_NENHUM) {
        printf("Item no chao: ");
        cor("1;93");
        printf("%s\n", NOMES_ITENS[l->item_chao]);
        fim_cor();
    }
    printf("Saidas:");
    for (int d = 0; d < NUM_DIR; d++) {
        if (l->saidas[d] != -1) printf(" %s", NOMES_DIR[d]);
    }
    printf("\n");
}

// Mapa fixo em ASCII. Para cada local, o operador ternario escolhe '*' (voce esta aqui) ou ' '.
static void mostrar_mapa(const Jogador *j) {
    printf("\n--- MAPA (* = voce) ---\n");
    printf("               [Gruta %c]\n",     j->local == LOC_TESOURO ? '*' : ' ');
    printf("                   |\n");
    printf("  [Sertao %c] -- [Chapada %c]\n", j->local == LOC_SERTAO  ? '*' : ' ',
                                              j->local == LOC_CHAPADA ? '*' : ' ');
    printf("      |\n");
    printf(" [Casa %c] ---- [Feira %c]\n",   j->local == LOC_CASA    ? '*' : ' ',
                                              j->local == LOC_FEIRA   ? '*' : ' ');
}

// Dica contextual. O if/else if segue ordem de prioridade: primeiro o problema mais
// urgente (pouca agua), depois a proxima acao provavel, por fim uma dica generica.
static void mostrar_dica(const Jogador *j, const LocalJogo loc[NUM_LOC]) {
    int f = fase_atual(j);
    cor("96"); printf("[DICA] "); fim_cor();
    if (j->agua <= 2 && !loc[j->local].tem_fonte) {
        printf("Pouca agua (%d/%d)! Va a Feira (fonte) ou descanse.\n", j->agua, j->agua_max);
    } else if (j->local == LOC_CHAPADA && tem_item(j, ITEM_CORDA) && !j->atalho_chapada) {
        printf("Use 'usar corda' aqui na Chapada!\n");
    } else if (j->local == LOC_TESOURO && tem_item(j, ITEM_CHAVE) && !j->bau_aberto) {
        printf("Use 'usar chave' para abrir o bau!\n");
    } else if (f == 1) {
        printf("E Tarde: cada passo gasta 2 de agua (fora de fontes).\n");
    } else if (f == 2 && !tem_item(j, ITEM_LAMPIAO) && j->local != LOC_TESOURO) {
        printf("Esta a Noite: pegue/use o lampiao na Casa para explorar no escuro.\n");
    } else {
        printf("Use 'explorar', 'pegar <item>', 'usar <item>' ou 'ir <direcao>'.\n");
    }
}

// Passa o tempo. Compara a fase ANTES e DEPOIS de somar os passos e avisa o jogador
// quando o dia muda de fase.
static void avancar_tempo(Jogador *j, int passos) {
    int antes = fase_atual(j);
    j->passos += passos;
    int depois = fase_atual(j);
    if (antes != depois) {
        printf("\n");
        cor(CORES_FASES[depois]);
        printf("[TEMPO] Agora e %s!\n", NOMES_FASES[depois]);
        fim_cor();
    }
}

// Movimento. Etapas: 1) a direcao existe? 2) ha caminho? 3) falta o item exigido?
// 4) muda de local e passa o tempo; 5) cobra agua: na tarde gasta 2, nas outras fases 1.
// Em locais com fonte, o cantil e enchido em vez de gastar agua.
static void cmd_ir(Jogador *j, LocalJogo loc[NUM_LOC], const char *arg) {
    int dir = -1;
    for (int d = 0; d < NUM_DIR; d++) {
        if (strcmp(arg, NOMES_DIR[d]) == 0) dir = d;
    }
    if (dir == -1) { printf("Direcao invalida (norte, sul, leste, oeste).\n"); return; }

    int dest = loc[j->local].saidas[dir];
    if (dest == -1) { printf("Nao ha caminho para o %s.\n", arg); return; }

    int req = loc[dest].exige_item;
    if (req != ITEM_NENHUM && !tem_item(j, req)) {
        printf("Voce precisa de '%s' para seguir.\n", NOMES_ITENS[req]);
        return;
    }

    j->local = dest;
    avancar_tempo(j, 1);

    if (loc[dest].tem_fonte) {
        j->agua = j->agua_max;
        cor("1;92"); printf("Cantil cheio na fonte! (%d/%d)\n", j->agua, j->agua_max); fim_cor();
    } else {
        int gasto = (fase_atual(j) == 1) ? 2 : 1;
        ajustar_agua(j, -gasto);
        // Evento aleatorio: rand() % 5 vale 0 em 1 de cada 5 vezes (20%) -> acha um umbu (+2 agua).
        if (rand() % 5 == 0) {
            cor("1;92"); printf("[EVENTO] Achou umbu no caminho! (+2 agua)\n"); fim_cor();
            ajustar_agua(j, 2);
        }
    }
    mostrar_local(j, loc);
}

// Pega o item do chao do local atual, se o nome digitado for igual ao do item.
static void cmd_pegar(Jogador *j, LocalJogo loc[NUM_LOC], const char *arg) {
    LocalJogo *l = &loc[j->local];
    if (l->item_chao != ITEM_NENHUM && strcmp(arg, NOMES_ITENS[l->item_chao]) == 0) {
        dar_item(j, l->item_chao);
        printf("Voce pegou: %s\n", NOMES_ITENS[l->item_chao]);
        l->item_chao = ITEM_NENHUM;
    } else {
        printf("Nao ha '%s' aqui.\n", arg);
    }
}

// Explorar revela o item oculto do local (uma unica vez, controlado por 'explorado').
// A noite e preciso ter o lampiao, exceto na gruta. Na primeira exploracao do Sertao,
// que nao tem item oculto, o jogador ganha +2 de agua.
static void cmd_explorar(Jogador *j, LocalJogo loc[NUM_LOC]) {
    LocalJogo *l = &loc[j->local];
    if (fase_atual(j) == 2 && j->local != LOC_TESOURO && !tem_item(j, ITEM_LAMPIAO)) {
        printf("Escuro demais para explorar a noite sem lampiao!\n");
        return;
    }
    printf("%s\n", l->dica_explorar);
    if (!l->explorado) {
        l->explorado = 1;
        if (l->item_oculto != ITEM_NENHUM) {
            dar_item(j, l->item_oculto);
            printf("Voce encontrou: %s!\n", NOMES_ITENS[l->item_oculto]);
            l->item_oculto = ITEM_NENHUM;
        } else if (j->local == LOC_SERTAO) {
            ajustar_agua(j, 2);
            printf("Agua: %d/%d\n", j->agua, j->agua_max);
        }
    }
}

// Usar item: o efeito depende do item E do lugar onde o jogador esta.
//   mapa  -> mostra o mapa
//   corda -> na Chapada abre o atalho: acha um cantil e aumenta a agua maxima em 3
//   chave -> na Gruta abre o bau (condicao de vitoria)
static void cmd_usar(Jogador *j, const char *arg) {
    int item = -1;
    for (int i = 0; i < NUM_ITENS; i++) {
        if (strcmp(arg, NOMES_ITENS[i]) == 0) item = i;
    }
    if (item == -1 || !tem_item(j, item)) {
        printf("Voce nao tem '%s'.\n", arg);
        return;
    }

    if (item == ITEM_MAPA) {
        mostrar_mapa(j);
    } else if (item == ITEM_CORDA && j->local == LOC_CHAPADA && !j->atalho_chapada) {
        j->atalho_chapada = 1;
        j->agua_max += 3;
        ajustar_agua(j, 3);
        dar_item(j, ITEM_CANTIL);
        printf("Voce desceu pela fenda, achou um CANTIL e aumentou a agua max para %d!\n", j->agua_max);
    } else if (item == ITEM_CHAVE && j->local == LOC_TESOURO) {
        j->bau_aberto = 1;
        printf("A chave abriu o cadeado do bau!\n");
    } else {
        printf("Voce usou '%s'.\n", NOMES_ITENS[item]);
    }
}

// Descansar passa o tempo ate a PROXIMA fase do dia: 'falta' e quantos passos restam
// para completar a fase atual. So recupera 1 de agua na Feira (fonte) ou na Casa.
static void cmd_descansar(Jogador *j, const LocalJogo loc[NUM_LOC]) {
    int falta = PASSOS_FASE - (j->passos % PASSOS_FASE);
    printf("Descansando ate a proxima fase do dia...\n");
    avancar_tempo(j, falta);
    if (loc[j->local].tem_fonte || j->local == LOC_CASA) {
        ajustar_agua(j, 1);
        printf("Recuperou +1 de agua (%d/%d).\n", j->agua, j->agua_max);
    }
}

static void cmd_inventario(const Jogador *j) {
    printf("Agua: %d/%d | Fase: %s | Itens:", j->agua, j->agua_max, NOMES_FASES[fase_atual(j)]);
    if (j->qtd_itens == 0) printf(" nenhum");
    for (int i = 0; i < j->qtd_itens; i++) printf(" %s", NOMES_ITENS[j->mochila[i]]);
    printf("\n");
}

// Despachante de comandos: compara o texto digitado com cada comando conhecido e chama
// a funcao certa. Retorna 0 quando o jogador pede 'sair' (o laco principal entao
// encerra) e 1 em todos os outros casos.
static int executar_comando_tesouro(Jogador *j, LocalJogo loc[NUM_LOC], const char *cmd, const char *arg) {
    if      (strcmp(cmd, "sair") == 0)       return 0;
    else if (strcmp(cmd, "ir") == 0)         cmd_ir(j, loc, arg);
    else if (strcmp(cmd, "olhar") == 0)      mostrar_local(j, loc);
    else if (strcmp(cmd, "explorar") == 0)   cmd_explorar(j, loc);
    else if (strcmp(cmd, "pegar") == 0)      cmd_pegar(j, loc, arg);
    else if (strcmp(cmd, "usar") == 0)       cmd_usar(j, arg);
    else if (strcmp(cmd, "descansar") == 0)  cmd_descansar(j, loc);
    else if (strcmp(cmd, "dica") == 0)       mostrar_dica(j, loc);
    else if (strcmp(cmd, "inventario") == 0) cmd_inventario(j);
    else if (strcmp(cmd, "cores") == 0) {
        g_cores = !g_cores;
        printf("Cores %s.\n", g_cores ? "ativadas" : "desativadas");
    }
    else if (cmd[0] != '\0')                 printf("Comando desconhecido.\n");
    return 1;
}

// Uma etapa do tutorial: mostra a instrucao e so avanca quando o jogador digita o comando
// esperado. Retorna -1 ('sair'), 0 ('pular') ou 1 (passo concluido).
static int etapa_tutorial(Jogador *j, LocalJogo loc[NUM_LOC],
                          const char *instrucao, const char *cmd_esp, const char *arg_esp) {
    char linha[64], cmd[20], arg[20];
    cor("1;93"); printf("\n[TUTORIAL] "); fim_cor();
    printf("%s\n", instrucao);

    while (1) {
        printf("\n> ");
        ler_linha(linha, sizeof linha);
        cmd[0] = arg[0] = '\0';
        sscanf(linha, "%19s %19s", cmd, arg);
        minusculas(cmd);
        minusculas(arg);

        if (strcmp(cmd, "sair") == 0) return -1;
        if (strcmp(cmd, "pular") == 0) {
            printf("[TUTORIAL] Tutorial pulado. Boa sorte!\n");
            return 0;
        }
        if (strcmp(cmd, cmd_esp) == 0 && strcmp(arg, arg_esp) == 0) {
            executar_comando_tesouro(j, loc, cmd, arg);
            return 1;
        }
        printf("Neste passo, digite: %s%s%s (ou 'pular')\n",
               cmd_esp, arg_esp[0] ? " " : "", arg_esp);
    }
}

// Roda os 4 passos do tutorial. Retorna -1 se o jogador digitou 'sair', 0 se digitou 'pular'
// (o tutorial inteiro termina na hora) e 1 se ele completou todos os passos.
static int rodar_tutorial_tesouro(Jogador *j, LocalJogo loc[NUM_LOC]) {
    printf("\n=== TUTORIAL DO CACADOR DE TESOURO ===\n");
    printf("Vamos testar os comandos basicos (digite 'pular' se quiser ir direto).\n");

    // Cada etapa devolve -1 (sair), 0 (pular) ou 1 (concluida). Qualquer valor diferente
    // de 1 interrompe o tutorial aqui e e repassado para quem chamou.
    int r;
    if ((r = etapa_tutorial(j, loc, "Passo 1/4: veja o local atual. Digite: olhar", "olhar", "")) != 1) return r;
    if ((r = etapa_tutorial(j, loc, "Passo 2/4: pegue o mapa. Digite: pegar mapa", "pegar", "mapa")) != 1) return r;
    if ((r = etapa_tutorial(j, loc, "Passo 3/4: abra o mapa. Digite: usar mapa", "usar", "mapa")) != 1) return r;
    if ((r = etapa_tutorial(j, loc, "Passo 4/4: explore a casa. Digite: explorar", "explorar", "")) != 1) return r;

    printf("\n[TUTORIAL] Pronto! Use 'dica' sempre que precisar de ajuda.\n");
    return 1;
}

// Laco principal do Tesouro: le comando -> executa -> verifica vitoria/derrota.
// O cronometro so comeca depois do tutorial; o resultado so e salvo se o jogador vencer.
static void rodar_tesouro(Scoreboard *sb) {
    LocalJogo loc[NUM_LOC];
    criar_locais(loc);

    // Inicializacao designada (.campo = valor): os campos nao citados comecam em 0.
    Jogador j = { .local = LOC_CASA, .agua = AGUA_INICIAL, .agua_max = AGUA_INICIAL };
    char linha[64], cmd[20], arg[20];

    printf("\n=== TESOURO DO NORDESTE ===\n");
    printf("Quer fazer o tutorial guiado? (s/n): ");
    ler_linha(linha, sizeof linha);
    minusculas(linha);

    // Se o jogador quiser o tutorial: 'sair' (-1) volta ao menu; 'pular' (0) ou nao querer
    // o tutorial mostram a lista de comandos e o local, como no inicio normal do jogo.
    int tutorial_completo = 0;
    if (linha[0] == 's') {
        int r = rodar_tutorial_tesouro(&j, loc);
        if (r < 0) return;
        tutorial_completo = (r == 1);
    }
    if (!tutorial_completo) {
        printf("Comandos: ir <dir> | olhar | explorar | pegar <item> | usar <item> | descansar | dica | inventario | cores | sair\n");
        mostrar_local(&j, loc);
        mostrar_dica(&j, loc);
    }

    // Guarda o instante inicial; ao vencer, difftime(agora, inicio) da os segundos jogados.
    time_t inicio = time(NULL);
    int venceu = 0;

    while (1) {
        // Condicao de vitoria: estar na gruta com o bau aberto (usar a chave).
        if (j.local == LOC_TESOURO && j.bau_aberto) {
            cor("1;92"); printf("\n*** PARABENS! Voce abriu o bau e achou o tesouro! ***\n"); fim_cor();
            venceu = 1;
            break;
        }
        if (j.agua <= 0) {
            cor("1;91"); printf("\nSua agua acabou... Fim de jogo.\n"); fim_cor();
            break;
        }

        printf("\n> ");
        ler_linha(linha, sizeof linha);
        // Divide a linha digitada em comando + argumento: 'ir norte' -> cmd = 'ir', arg = 'norte'.
        // O limite %19s impede estourar os vetores de 20 posicoes. Se so houver o comando,
        // arg continua vazio (por isso a linha acima zera os dois).
        cmd[0] = arg[0] = '\0';
        sscanf(linha, "%19s %19s", cmd, arg);
        minusculas(cmd);
        minusculas(arg);

        if (!executar_comando_tesouro(&j, loc, cmd, arg)) break;
    }

    if (venceu) {
        double seg = difftime(time(NULL), inicio);
        int pts = calcular_pontos(0, 0, 0, seg);
        salvar_resultado(sb, "Tesouro do Nordeste", pts, seg);
    }
}

/* =========================================================================
 * MENU PRINCIPAL
 * ========================================================================= */

// main(): carrega o placar e mostra o menu em laco ate o jogador escolher 0 (Sair).
int main(void) {
    Scoreboard sb;
    char op[16];

    // Semente do gerador aleatorio (usado no evento do umbu). Sem ela, a sequencia de
    // numeros 'aleatorios' seria sempre a mesma a cada execucao.
    srand((unsigned int)time(NULL));
    // Carrega o placar salvo (se existir) antes de abrir o menu.
    carregar_scoreboard(&sb);

    while (1) {
        printf("\n========================================\n");
        cor("1;93"); printf("      CENTRAL DE JOGOS - NORDESTE\n"); fim_cor();
        printf("========================================\n");
        printf("[1] Quiz do Nordeste\n");
        printf("[2] Tesouro do Nordeste\n");
        printf("[3] Ver Scoreboard (%d)\n", sb.qtd);
        printf("[4] Limpar Scoreboard\n");
        printf("[0] Sair\n");
        printf("Escolha: ");

        ler_linha(op, sizeof op);
        if (strcmp(op, "1") == 0) {
            rodar_quiz(&sb);
        } else if (strcmp(op, "2") == 0) {
            rodar_tesouro(&sb);
        } else if (strcmp(op, "3") == 0) {
            mostrar_scoreboard(&sb, MAX_SCORES);
            esperar_enter("ENTER para voltar...");
        } else if (strcmp(op, "4") == 0) {
            sb.qtd = 0;
            salvar_scoreboard(&sb);
            printf("Scoreboard apagado.\n");
        } else if (strcmp(op, "0") == 0) {
            printf("Ate a proxima!\n");
            break;
        } else {
            printf("Opcao invalida.\n");
        }
    }

    return 0;
}