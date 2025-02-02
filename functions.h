#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#define TELAX 1200
#define TELAY 600
#define FATOR_ESCALA 20 // Necessário para se fazer a escala do arquivo mapa 25x60 para o jogo 600x1200.
#define LINHAS 25
#define COLUNAS 60
#define BLOCO 20        // Tamanho dos Blocos de personagem, inimigo, obstáculos e bombas.
#define ENDERECO 25
#define TRUE 1
#define FALSE 0
#define NVIDAS 5
#define NINIMIGOS 5
#define MAX_BOMBAS 3
#define TEMPO_EXPLOSAO 120
#define TEMPO_FOGO 120
// garante que o maximo de fogos desenhaveis no mapa simultaneamente seja função das variaveis definidas e da explosão em cruz (4 direções)
#define MAX_FOGOS (MAX_BOMBAS*RAIO_EXPLOSAO*4+1)
#define RAIO_EXPLOSAO 2
#define TEMPO_INIMIGO 15
#define FONTE_MENU 50
#define TELA_MENU 500
#define PAREDE_INDESTRUTIVEL 'W'
#define CAIXA_S_CHAVE 'B'
#define PAREDE_DESTRUTIVEL 'D'
#define CAIXA_CHAVE 'K'
#define CHAVE 'C'  // Usado durante o jogo, após a explosão de uma caixa K
#define BOMBA 'L' // Usado durante o jogo, não no mapa inicial
#define INIMIGO 'E'
#define FOGO 'F'
#define JOGADOR 'J'

typedef struct{
    int linha, coluna;
}Posicao;

typedef struct {
    Posicao pos_bomba;
    int tempo_restante;
    int explodindo;
    int explodida;
}Bomba;

typedef struct{
    Posicao pos_jogador;
    int vida;
    char direcao;
    int bombas_ativas;
    Bomba bombas[MAX_BOMBAS];
    int pontos;
    int chaves;
    int nivel;
}Jogador;

typedef struct{
    Posicao pos_inimigo;
    int direcao;
    int tempo_movimento;
}Inimigo;

typedef struct {
    Posicao pos_fogo; // Posição do fogo
    int tempo_restante; // Tempo restante até o fogo desaparecer
    int visivel;
} Fogo;

// HEADERs das Funções utilizadas ao longo do jogo
Posicao le_mapa(char *nome_arquivo, char mapa[][COLUNAS], Inimigo inimigos[], int *num_inimigos);
void atualiza_mapa(char mapa[][COLUNAS], Jogador *jogador, Inimigo inimigos[], int num_inimigos);
void desenha_tela(char mapa[][COLUNAS], Jogador *jogador, int *pause, char *char_menu);
void desenha_inimigos(Inimigo inimigos[], int num_inimigos); // Adicionada para desenhar inimigos
void move_jogador(Jogador *jogador, char mapa[][COLUNAS], Inimigo inimigos[], int num_inimigos); // Header para a função de movimentação do jogador
void move_inimigos(Inimigo inimigos[], int num_inimigos, char mapa[][COLUNAS], Jogador *jogador);
char menu(int *pause); // Função que abre o menu do jogo, após esse ser pausado
int verifica_colisao(Posicao pos, char mapa[][COLUNAS], Jogador *jogador, Inimigo inimigos[], int num_inimigos, int cod); // o "cod" é o codigo identificador de colisão jogador-algo (0) ou inimigo-algo (1)
void verifica_explosao(Jogador *jogador, char mapa[][COLUNAS], Inimigo inimigos[], int *num_inimigos, Fogo fogos[], int *num_fogos);
void processa_explosao(Posicao origem, int sentido_linha, int sentido_coluna, char mapa[][COLUNAS], Fogo fogos[], int *num_fogos, Jogador *jogador, Inimigo inimigos[], int *num_inimigos);
void verifica_inimigos_fogo(Inimigo inimigos[], int *num_inimigos, Fogo fogos[], int num_fogos, char mapa[][COLUNAS]);
void adiciona_bomba(Jogador *jogador, char mapa[][COLUNAS]);
void atualiza_tempo_bombas(Jogador *jogador);
void atualiza_bomba(Jogador *jogador, char mapa[][COLUNAS]);
void atualiza_fogos(Fogo fogos[], int *num_fogos, char mapa[][COLUNAS]);
void atualiza_pontos(Jogador *jogador, int evento);
void desenha_fogos(Fogo fogos[], int num_fogos);
void inicializa(char *nome_arquivo);
int salva_jogo(char mapa[][COLUNAS], Jogador *dados_jogador, Inimigo dados_inimigos[], int num_inimigos);
int carrega_jogo(char mapa[][COLUNAS], Jogador *dados_jogador, Inimigo dados_inimigos[], int num_inimigos);
int jogo(Jogador *jogador, Inimigo inimigos[], int *num_inimigos, char *nome_arquivo, char mapa[][COLUNAS], int *pause, Fogo fogos[], int num_fogos);
int passa_nivel(Jogador *jogador, Inimigo inimigos[], int *num_inimigos, char mapa[][COLUNAS]);  // função para cuidar da passagem dos níveis
void novo_jogo(); // função que faz o jogo voltar para o seu início, apagando todo o progresso (mas sem afetar um arquivo de salvamento)

#endif // FUNCTIONS_H_INCLUDED
