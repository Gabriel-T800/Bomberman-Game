#include <stdio.h>
#include <raylib.h>
#include <string.h>
#include "functions.h"

// Função que cuida da inicialização dos recursos necessários, como a criação da tela de jogo, as varíaveis "globais" necessárias.
void inicializa(char *nome_arquivo) {
    char mapa[LINHAS][COLUNAS];
    int status = 0;
    int pause = FALSE;

    //inicialização do vetor de inimigos
    Inimigo inimigos[NINIMIGOS];
    int num_inimigos = NINIMIGOS;  // inicializa o número de inimigos
    Jogador jogador;  // declaração da variável jogador

    // Inicialização dos atributos do jogador
    jogador.vida = NVIDAS;
    jogador.pontos = 0;
    jogador.chaves = 0;
    jogador.nivel = 1;
    jogador.bombas_ativas = 0;

    // Declare e inicialize o array de fogos
    Fogo fogos[MAX_FOGOS]; // Defina MAX_FOGOS como o número máximo de fogos que você deseja
    int num_fogos = 0; // Inicializa o número de fogos

    InitWindow(TELAX, TELAY, "Bomberman");
    SetTargetFPS(60);

    // lê o mapa e guarda a posição do jogador
    jogador.pos_jogador = le_mapa(nome_arquivo, mapa, inimigos, &num_inimigos);

    while (!WindowShouldClose() && status!=-1)
    {
        status = jogo(&jogador, inimigos, &num_inimigos, nome_arquivo, mapa, &pause, fogos, num_fogos);
        // O teste abaixo cuida da passagem de nível, mas se retornar -1 irá fechar imediatamente o jogo (erro de nível inexistente)
        if(status==5)
            status=passa_nivel(&jogador, inimigos, &num_inimigos, mapa);
    }
    CloseWindow();
}

// FUNÇÃO na qual roda a maior parte do jogo, sendo desviada somente para outras subfunções.
int jogo(Jogador *jogador, Inimigo inimigos[], int *num_inimigos, char *nome_arquivo, char mapa[][COLUNAS], int *pause, Fogo fogos[], int num_fogos) {
    char char_menu='!'; // variavel que irá receber o retorno do menu de pausa. Inicializei como V para que ele não crie um loop depois que responder uma vez o menu com algo diferente de V
    int retorno;

    static int frame_counter = 0;

    if (IsKeyPressed(KEY_TAB))
        *pause = TRUE;

    if (*pause == FALSE) {
        move_jogador(jogador, mapa, inimigos, *num_inimigos); // trata da movimentação do jogador.
        atualiza_mapa(mapa, jogador, inimigos, *num_inimigos);
        retorno=jogador->chaves;

        // funções relacionadas às bombas e suas respectivas explosões
        if (IsKeyPressed(KEY_B)) {// trata do posicionamento das bombas
            adiciona_bomba(jogador, mapa);
        }

        atualiza_tempo_bombas(jogador);
        verifica_explosao(jogador, mapa, inimigos, num_inimigos, fogos, &num_fogos);

        // Verifica se algum inimigo deve ser removido ao colidir com o fogo
        verifica_inimigos_fogo(inimigos, num_inimigos, fogos, num_fogos, mapa);

        // faz os inimigo se mexerem a cada X frames (definido)
        frame_counter++;
        if (frame_counter >= TEMPO_INIMIGO) { // quanto menor o tempo, mas rápido se movem
            move_inimigos(inimigos, *num_inimigos, mapa, jogador);
            atualiza_mapa(mapa, jogador, inimigos, *num_inimigos);
            frame_counter = 0; // Reseta o contador
        }
    }

    // Desenho do jogo
    desenha_inimigos(inimigos, *num_inimigos); // Passa o número de inimigos como um inteiro
    desenha_tela(mapa, jogador, pause, &char_menu);
    if(char_menu!='!'){
        switch(char_menu){
            case 'Q':
                retorno=-1;
                break;
            case 'S':
                *pause=TRUE;
                int inimigos_vivos=*num_inimigos; // para mandar o valor por cópia, visto não ser necessário editar
                retorno = salva_jogo(mapa, jogador, inimigos, inimigos_vivos);
                WaitTime(10); // força o jogo a ficar congelado por 5 segundos, permitindo que salve tudo sem corromper dados
                if(retorno==-1)
                    printf("Salvamento corrompido!\n");
                break;
            case 'C':
                retorno = carrega_jogo(mapa, jogador, inimigos, inimigos_vivos);
                WaitTime(10);  // força o jogo a ficar congelado por 5 segundos, permitindo que carregue tudo sem corromper dados
                break;
            case 'N':
                novo_jogo();
                break;
            case 'V':
                char_menu='!';
                *pause=FALSE;
                frame_counter=0; // reseta para sincronizar com o tempo real do jogo
                break;
        }
    }
    return retorno;
}

// FUNÇÃO que faz a leitura do arquivo texto do nível e passa para a RAM, permitindo jogar apenas pela RAM.
Posicao le_mapa(char *nome_arq, char mapa[][COLUNAS], Inimigo inimigos[], int *num_inimigos){
    Posicao pos_jogador;
    FILE *arq;
    int i, j, cont_inimigo=0;

    arq = fopen(nome_arq, "r");
    if(arq==NULL){
        printf("Erro ao abrir o arquivo \"%s\"!\n", nome_arq);
        // salva a posição do jogador em posições fora da área de impressão do mapa
        pos_jogador.coluna=-1;
        pos_jogador.linha=-1;
    }
    else
    {
        for(i=0; i<LINHAS; i++)
        {
            for(j=0; j<COLUNAS; j++)
            {
                mapa[i][j]=fgetc(arq);
                if(mapa[i][j]==JOGADOR){
                    pos_jogador.linha=i*FATOR_ESCALA;
                    pos_jogador.coluna=j*FATOR_ESCALA;
                }
                else if(mapa[i][j]==INIMIGO){
                    inimigos[cont_inimigo].pos_inimigo.linha=i*FATOR_ESCALA;
                    inimigos[cont_inimigo].pos_inimigo.coluna=j*FATOR_ESCALA;
                    cont_inimigo++;
                }
            }
            fgetc(arq);
        }
        fclose(arq);
        *num_inimigos=cont_inimigo;
    }
    return pos_jogador;
}

// Função que de atualizar as posições dos inimigos e do jogador no mapa, alterando a letra correspondente de lugar
void atualiza_mapa(char mapa[][COLUNAS], Jogador *jogador, Inimigo inimigos[], int num_inimigos) {
    // Limpa o mapa
    for (int i = 0; i < LINHAS; i++) {
        for (int j = 0; j < COLUNAS; j++) {
            if (mapa[i][j] == JOGADOR || mapa[i][j] == INIMIGO) {
                mapa[i][j] = ' ';
            }
        }
    }

    // Atualiza posições no mapa
    mapa[jogador->pos_jogador.linha / BLOCO][jogador->pos_jogador.coluna / BLOCO] = JOGADOR;
    for (int i = 0; i < num_inimigos; i++) {
        mapa[inimigos[i].pos_inimigo.linha / BLOCO][inimigos[i].pos_inimigo.coluna / BLOCO] = INIMIGO;
    }
}

// Função que trata de todo o desenho do programa (desviada para a subfunção menu se for o caso)
void desenha_tela(char mapa[][COLUNAS], Jogador *jogador, int *pause, char *char_menu){
    int i, j;
    int pos_x, pos_y;
    int bombas_disp=(jogador->bombas_ativas-3)*(-1);

    BeginDrawing();
    DrawText(TextFormat("Vidas: %i", jogador->vida), 20, TELAY-80, FONTE_MENU, WHITE);
    DrawText(TextFormat("Bombas: %i", bombas_disp), 260, TELAY-80, FONTE_MENU, WHITE);
    DrawText(TextFormat("Pontos: %i", jogador->pontos), 560, TELAY-80, FONTE_MENU, WHITE);
    DrawText(TextFormat("Chaves: %i", jogador->chaves), 900, TELAY-90, FONTE_MENU/4*3, WHITE);
    DrawText(TextFormat("Nivel: %i", jogador->nivel), 900, TELAY-40, FONTE_MENU/4*3, WHITE);
    DrawRectangle(jogador->pos_jogador.coluna, jogador->pos_jogador.linha, BLOCO, BLOCO, WHITE);
    for(j=0; j<COLUNAS; j++)
    {
        for(i=0; i<LINHAS; i++)
        {
            pos_x=FATOR_ESCALA*(j);
            pos_y=FATOR_ESCALA*(i);
            switch(mapa[i][j]){
            case PAREDE_INDESTRUTIVEL:
                DrawRectangle(pos_x, pos_y, BLOCO, BLOCO, BLACK);
                break;
            case PAREDE_DESTRUTIVEL:
                DrawRectangle(pos_x, pos_y, BLOCO, BLOCO, GRAY);
                break;
            case CAIXA_CHAVE:
            case CAIXA_S_CHAVE:
                DrawRectangle(pos_x, pos_y, BLOCO, BLOCO, BEIGE);
                break;
            case BOMBA:
                DrawRectangle(pos_x, pos_y, BLOCO, BLOCO, DARKGRAY);
                break;
            case CHAVE:
                DrawRectangle(pos_x, pos_y, BLOCO, BLOCO, GOLD);
            }

        }
    }
    ClearBackground(GREEN);
    if(*pause==TRUE)
    {
        *char_menu = menu(pause);
    }
    EndDrawing();
}

// função apenas para desenhar os inimigos
void desenha_inimigos(Inimigo inimigos[], int num_inimigos) {
    for (int i = 0; i < num_inimigos; i++) {
        DrawRectangle(inimigos[i].pos_inimigo.coluna,
                      inimigos[i].pos_inimigo.linha,
                    BLOCO, BLOCO, MAROON);
    }
}

// FUNÇÃO para cuidar da movimentação do jogador. Movimentação é feita bloco a bloco (20x20) e acontece conforme clicadas na tecla correspondente.
void move_jogador(Jogador *jogador, char mapa[][COLUNAS], Inimigo inimigos[], int num_jogador) {
    int colisao;

    if (IsKeyDown(KEY_UP) || IsKeyPressed(KEY_W)) {
        jogador->direcao = 'W';
        jogador->pos_jogador.linha -= 20;
        colisao = verifica_colisao(jogador->pos_jogador, mapa, jogador,inimigos, num_jogador, 0);
        if (colisao != 0) {
            jogador->pos_jogador.linha += 20;
        }
    }

    else if (IsKeyDown(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        jogador->direcao = 'S';
        jogador->pos_jogador.linha += 20;
        colisao = verifica_colisao(jogador->pos_jogador, mapa, jogador,inimigos, num_jogador, 0);
        if (colisao != 0) {
            jogador->pos_jogador.linha -= 20;
        }
    }

    else if (IsKeyDown(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        jogador->direcao = 'D';
        jogador->pos_jogador.coluna += 20;
        colisao = verifica_colisao(jogador->pos_jogador, mapa, jogador, inimigos, num_jogador, 0);
        if (colisao != 0) {
            jogador->pos_jogador.coluna -= 20;
        }
    }

    else if (IsKeyDown(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        jogador->direcao = 'A';
        jogador->pos_jogador.coluna -= 20;
        colisao = verifica_colisao(jogador->pos_jogador, mapa, jogador,inimigos, num_jogador, 0);
        if (colisao != 0) {
            jogador->pos_jogador.coluna += 20;
        }
    }

     else if (IsKeyPressed(KEY_E)) { //apenas para a bomba
        jogador->direcao = 'E';
    }
}

//move o inimigo segundo a direção atual de movimento, além de escolher uma nova direção aleatoria periodicamente ou quando colide
void move_inimigos(Inimigo inimigos[], int num_inimigos, char mapa[][COLUNAS], Jogador *jogador) {
   int randomizar=FALSE;

    for (int i = 0; i < num_inimigos; i++) {
        do{
            Posicao nova_posicao = inimigos[i].pos_inimigo;
            switch (inimigos[i].direcao) {
                case 0:
                    nova_posicao.linha -= 20; // Move para cima
                    break;
                case 1:
                    nova_posicao.linha += 20; // Move para baixo
                    break;
                case 2:
                    nova_posicao.coluna -= 20; // Move para a esquerda
                    break;
                case 3:
                    nova_posicao.coluna += 20; // Move para a direita
                    break;
        }

        // Verifica se a nova posição é válida
        if (!verifica_colisao(nova_posicao, mapa, jogador, inimigos, num_inimigos, 1) && inimigos[i].tempo_movimento<5) {
            randomizar=FALSE;
            inimigos[i].tempo_movimento++;
            inimigos[i].pos_inimigo = nova_posicao; // Atualiza a posição do inimigo
        }
        else
        {
            int nova_direcao;
            randomizar=TRUE;
            inimigos[i].tempo_movimento=0;
            do
            {
                nova_direcao = GetRandomValue(0,3); // 0: cima, 1: baixo, 2: esquerda, 3: direita
            }while(nova_direcao==inimigos[i].direcao);
            inimigos[i].direcao=nova_direcao;
        }
        }while(randomizar==TRUE);
    }
}

// função que cuida da física do jogo. Retornar 0 significa sem colisão, e 1 significa colisão
int verifica_colisao(Posicao pos, char mapa[][COLUNAS], Jogador *jogador, Inimigo inimimigos[], int num_inimigos, int codigo) {
    int linha_mapa = pos.linha/BLOCO;
    int coluna_mapa = pos.coluna/BLOCO;

    // Verifica se as coordenadas estão dentro dos limites do mapa
    if (linha_mapa < 0 || linha_mapa >= LINHAS || coluna_mapa < 0 || coluna_mapa >= COLUNAS)
        return 1; // Colisão com os limites do mapa

    // testes de colisão pertinentes somente ao jogador e sua movimentação
    if(codigo==0)
    {
        if(mapa[linha_mapa][coluna_mapa]=='C')
        {
            jogador->chaves++;
            return 0; // Jogador pegou a chave
        }
    }

    // testes de colisão pertinentes para a movimentação dos inimigos
    if(codigo==1)
    {
        if(linha_mapa==jogador->pos_jogador.linha/BLOCO && coluna_mapa==jogador->pos_jogador.coluna/BLOCO)
            {
                jogador->vida--;
                atualiza_pontos(jogador, -100);
                // Reinicia o jogo se o jogador não tiver mais vidas
                if (jogador->vida <= 0)
                    novo_jogo(); // Chama a função para reiniciar o jogo
                return 1; // colisão de um inimigo com o jogador
            }

        else if(mapa[linha_mapa][coluna_mapa]=='C')
            return 1; // para evitar que inimigos peguem uma chave

        for(int i=0; i<num_inimigos; i++)
        {
            if(linha_mapa==inimimigos[i].pos_inimigo.linha && coluna_mapa==inimimigos[i].pos_inimigo.coluna)
                return 1; // Colisão do inimigo com outro dos inimigos vivos
        }
    }

    // Verifica se a posição no mapa corresponde a uma letra (obstáculo/explosão).
    switch (mapa[linha_mapa][coluna_mapa])
    {
    case PAREDE_INDESTRUTIVEL:
    case PAREDE_DESTRUTIVEL:
    case BOMBA:
    case CAIXA_S_CHAVE:
    case CAIXA_CHAVE:
    case JOGADOR:
    case INIMIGO:
        return 1; // Colisão com algo (obstáculos ou raio da bomba)
    default:
        return 0; // Sem colisão.
    }
}

// trata exclusivamente da colisão com o fogo
void verifica_colisao_fogo(Jogador *jogador, Fogo fogos[], int num_fogos) {
    for (int i = 0; i < num_fogos; i++) {
        // Verifica se a posição do jogador colide com a posição do fogo
        if (jogador->pos_jogador.linha == fogos[i].pos_fogo.linha && jogador->pos_jogador.coluna == fogos[i].pos_fogo.coluna) {
            jogador->vida--; // Decrementa a vida do jogador

            // Reinicia o jogo se o jogador não tiver mais vidas
            if (jogador->vida <= 0) {
                novo_jogo(); // Chama a função para reiniciar o jogo
            }
        }
    }
}

// Função para marcar os inimigos que foram atingidos pelo fogo para posterior remoção do mapa
void verifica_inimigos_fogo(Inimigo inimigos[], int *num_inimigos, Fogo fogos[], int num_fogos, char mapa[][COLUNAS]) {
    // Array para marcar inimigos a serem removidos
    int inimigos_para_remover[NINIMIGOS] = {0}; // Inicializa com 0
    int total_removidos = 0;

    for (int i = 0; i < *num_inimigos; i++) {
        for (int j = 0; j < num_fogos; j++) {
            // Verifica se a posição do inimigo coincide com a posição do fogo
            if (inimigos[i].pos_inimigo.linha == fogos[j].pos_fogo.linha &&
                inimigos[i].pos_inimigo.coluna == fogos[j].pos_fogo.coluna) {

                // Atualiza o mapa para remover o inimigo
                mapa[inimigos[i].pos_inimigo.linha][inimigos[i].pos_inimigo.coluna] = ' '; // Limpa a posição do inimigo

                // Marca o inimigo para remoção
                inimigos_para_remover[i] = 1; // Marca como 1 para indicar que deve ser removido
                total_removidos++;
                break; // Sai do loop de fogos, pois o inimigo já foi encontrado
            }
        }
    }
}

// Função chamada pela função acima, tratando exclusivamente da eliminação do inimigo do mapa e do array (além de atualizar o número de inimigos vivos)
void remove_inimigo(Inimigo inimigos[], int *num_inimigos, Posicao pos_explosao) {
    for (int j = 0; j < *num_inimigos; j++) {
        // Verifica se a posição do inimigo coincide com a posição da explosão
        if (inimigos[j].pos_inimigo.linha == pos_explosao.linha &&
            inimigos[j].pos_inimigo.coluna == pos_explosao.coluna) {
            // Remove o inimigo
            for (int k = j; k < *num_inimigos - 1; k++) {
                inimigos[k] = inimigos[k + 1]; // Move os inimigos uma posição a menos no array
            }
            (*num_inimigos)--; // Decrementa o número de inimigos
            break; // Sai do loop após remover o inimigo
        }
    }
}

// Função que cuida de adicionar a bomba a frente da última posição de movimento do jogador
void adiciona_bomba(Jogador *jogador, char mapa[][COLUNAS]) {
    Posicao bomba;
    bomba.linha = jogador->pos_jogador.linha;
    bomba.coluna = jogador->pos_jogador.coluna;

    // Ajusta a posição da bomba com base na direção do jogador
    switch (jogador->direcao) {
        case 'W':
            bomba.linha -= BLOCO; // Cima
            break;
        case 'S':
            bomba.linha += BLOCO; // Baixo
            break;
        case 'D':
            bomba.coluna += BLOCO; // Direita
            break;
        case 'A':
            bomba.coluna -= BLOCO; // Esquerda
            break;
        case 'E':
            bomba.linha = jogador->pos_jogador.linha; // Coloca a bomba na mesma linha
            bomba.coluna = jogador->pos_jogador.coluna; // Mantém a mesma coluna
            break;
    }

    // Verifica se a nova posição da bomba é válida
    if (bomba.linha >= 0 && bomba.linha < LINHAS * BLOCO &&
        bomba.coluna >= 0 && bomba.coluna < COLUNAS * BLOCO &&
        (mapa[bomba.linha / BLOCO][bomba.coluna / BLOCO] == ' ' ||
        (bomba.linha == jogador->pos_jogador.linha &&
        bomba.coluna == jogador->pos_jogador.coluna)) &&
        jogador->pos_jogador.linha >= 0 &&
        jogador->pos_jogador.coluna >= 0 &&
        jogador->bombas_ativas < MAX_BOMBAS) {

        // Coloca a bomba no mapa
        mapa[bomba.linha / BLOCO][bomba.coluna / BLOCO] = BOMBA;

        // Adiciona a bomba ao jogador
        jogador->bombas[jogador->bombas_ativas].pos_bomba.linha = bomba.linha;
        jogador->bombas[jogador->bombas_ativas].pos_bomba.coluna = bomba.coluna;
        jogador->bombas[jogador->bombas_ativas].tempo_restante = TEMPO_EXPLOSAO;
        jogador->bombas[jogador->bombas_ativas].explodindo = false;
        jogador->bombas[jogador->bombas_ativas].explodida = false; // Garante que explodida comece como false

        jogador->bombas_ativas++;
        }
}

void verifica_explosao(Jogador *jogador, char mapa[][COLUNAS], Inimigo inimigos[], int *num_inimigos, Fogo fogos[], int *num_fogos) {
    for (int i = jogador->bombas_ativas - 1; i >= 0; i--) {
        if (jogador->bombas[i].tempo_restante <= 0 && !jogador->bombas[i].explodida) {
            jogador->bombas[i].explodida = true;
            Posicao bomba = jogador->bombas[i].pos_bomba;
            mapa[bomba.linha/BLOCO][bomba.coluna/BLOCO] = ' ';

            // Processa as direções da explosão (cima, baixo, esquerda, direita) para parar ao encontrar uma parede indestrutível
            processa_explosao(bomba, -1, 0, mapa, fogos, num_fogos, jogador, inimigos, num_inimigos); // Cima
            processa_explosao(bomba, 1, 0, mapa, fogos, num_fogos, jogador, inimigos, num_inimigos);  // Baixo
            processa_explosao(bomba, 0, -1, mapa, fogos, num_fogos, jogador, inimigos, num_inimigos); // Esquerda
            processa_explosao(bomba, 0, 1, mapa, fogos, num_fogos, jogador, inimigos, num_inimigos);  // Direita

            // Remove a bomba após a explosão
            for (int j = i; j < jogador->bombas_ativas - 1; j++) {
                jogador->bombas[j] = jogador->bombas[j + 1];
            }
            jogador->bombas_ativas--;
        }
    }
}

// Função auxiliar para processar explosão em uma direção específica
void processa_explosao(Posicao origem, int sentido_linha, int sentido_coluna, char mapa[][COLUNAS], Fogo fogos[], int *num_fogos, Jogador *jogador, Inimigo inimigos[], int *num_inimigos) {
    for (int propagacao = 1; propagacao <= RAIO_EXPLOSAO; propagacao++) {
        Posicao pos_explosao;
        pos_explosao.linha = origem.linha + sentido_linha * propagacao * BLOCO;
        pos_explosao.coluna = origem.coluna + sentido_coluna * propagacao * BLOCO;

        // Verifica se está fora dos limites do mapa
        if (pos_explosao.linha < 0 || pos_explosao.linha >= LINHAS * BLOCO ||
            pos_explosao.coluna < 0 || pos_explosao.coluna >= COLUNAS * BLOCO) {
            break; // Interrompe a propagação
        }

        int linha = pos_explosao.linha / BLOCO;
        int coluna = pos_explosao.coluna / BLOCO;

        // Interrompe a propagação ao encontrar parede indestrutível ou chave
        if (mapa[linha][coluna] == PAREDE_INDESTRUTIVEL || mapa[linha][coluna] == CHAVE || mapa[linha][coluna] == BOMBA) {
            break;
        }

        // Trata remoção de inimigos
        if (mapa[linha][coluna] == INIMIGO) {
            remove_inimigo(inimigos, num_inimigos, pos_explosao);
            mapa[linha][coluna] = ' ';
            atualiza_pontos(jogador, 20);
            break; // Fogo não continua após eliminar inimigo
        }

        // Trata substituição da caixa com chave por uma chave
        if (mapa[linha][coluna] == CAIXA_CHAVE) {
            mapa[linha][coluna] = CHAVE;
            atualiza_pontos(jogador, 10);
            break; // Fogo para após atingir a caixa com chave
        }

        // Trata destruição de paredes destrutíveis ou caixas sem chave
        if (mapa[linha][coluna] == PAREDE_DESTRUTIVEL || mapa[linha][coluna] == CAIXA_S_CHAVE) {
            mapa[linha][coluna] = ' '; // Remove o objeto
            atualiza_pontos(jogador, 10);
            break; // Fogo para após destruir objeto
        }

        // Verifica dano ao jogador
        if (jogador->pos_jogador.linha == pos_explosao.linha && jogador->pos_jogador.coluna == pos_explosao.coluna) {
            jogador->vida--;
            atualiza_pontos(jogador, -100);
            if (jogador->vida <= 0) {
                novo_jogo(jogador);
            }
        }

    }
}

// Função para atualizar o tempo das bombas até que elas explodam
void atualiza_tempo_bombas(Jogador *jogador) {
    for (int i = 0; i < jogador->bombas_ativas; i++) {
        if (!jogador->bombas[i].explodida) {
            if (jogador->bombas[i].tempo_restante > 0) {
                jogador->bombas[i].tempo_restante--;
                if (jogador->bombas[i].tempo_restante <= 0)
                    jogador->bombas[i].explodindo = true;  // bomba de fato explodirá aqui
            }
        }
    }
}

// função apenas para tratar da atualização da pontuação
void atualiza_pontos(Jogador *jogador, int evento){
    /*
    destruir obstaculo = +10
    explodir inimigo = +20
    perder vida = -100
    passar de nível = +200
    */
    jogador->pontos+=evento;
    if(jogador->pontos<=0)
        jogador->pontos=0; // evita que a pontuação se torne negativa
}

// Função que imprime na tela o menu e, a partir da resposta, volta com a resposta para a desenha tela devolver ao loop do jogo poder reagir
char menu(int *pause){
    char comando;

    //Complexo de interpretar, mas desenha o fundo do menu de forma centralizada na tela, imprimindo também os textos dos comandos.
    DrawRectangle(TELAX/2-TELA_MENU/2, TELAY/2-TELA_MENU/2, TELA_MENU, TELA_MENU*2/3, YELLOW);
    DrawText("(N) Novo Jogo", TELAX/2-TELA_MENU/2+10, TELAY/2-TELA_MENU/2+50, FONTE_MENU, RED);
    DrawText("(C) Carregar Jogo", TELAX/2-TELA_MENU/2+10, TELAY/2-TELA_MENU/2+100, FONTE_MENU, BLUE);
    DrawText("(S) Salvar Jogo", TELAX/2-TELA_MENU/2+10, TELAY/2-TELA_MENU/2+150, FONTE_MENU, GREEN);
    DrawText("(V) Voltar ao Jogo", TELAX/2-TELA_MENU/2+10, TELAY/2-TELA_MENU/2+200, FONTE_MENU, BLACK);
    DrawText("(Q) Sair do Jogo", TELAX/2-TELA_MENU/2+10, TELAY/2-TELA_MENU/2+250, FONTE_MENU, WHITE);

    // Testa as diferentes possibilidades de comando no menu, e chama o recurso correspondente
    if(IsKeyPressed(KEY_N))
        comando='N';
    else if(IsKeyPressed(KEY_C))
    {
        DrawText("Carregando jogo! Aguarde 10s.", TELAX/2-TELA_MENU/2+50, TELAY/2-TELA_MENU/2+10, FONTE_MENU/2, BLUE);
        comando='C';
    }
    else if(IsKeyPressed(KEY_S))
    {
        DrawText("Salvando jogo! Aguarde 10s.", TELAX/2-TELA_MENU/2+50, TELAY/2-TELA_MENU/2+10, FONTE_MENU/2, GREEN);
        comando='S';
    }
    else if(IsKeyPressed(KEY_V))
    {
        comando='V';
        *pause=FALSE;
    }
    else if(IsKeyPressed(KEY_Q))
        comando='Q';

    return comando;
}

// função que (re)inicia o jogo, (re)inicializando-o (a partir de uma subfunção) com o mapa 1.
void novo_jogo(){
    char nome_arquivo[ENDERECO]="Mapa01.txt";
    inicializa(nome_arquivo);
}

// Função para salvar o jogo. Cria/Sobreescreve um arquivo binário
int salva_jogo(char mapa[][COLUNAS], Jogador *jogador, Inimigo inimigos[], int num_inimigos){

    char arquivo[ENDERECO]="Salvamento Manual.bin";
    int salvamento; // retorno para saber se foi bem sucedido ou não o salvamento do jogo.
    FILE *arq;

    arq = fopen(arquivo, "wb");
    if(arq==NULL){
        printf("Erro ao criar o salvamento!\n");
        salvamento=-1;
    }
    else
    {
        // escreve no arquivo o mapa inteiro de uma só vez na memória secundaria
        fwrite(mapa, sizeof(char), LINHAS*COLUNAS, arq);
        if(ferror(arq)!=0)
        {
            printf("Mapa corrompido durante o salvamento!\n");
            salvamento=-1;
        }
        // escreve todos os dados do jogador de uma só vez na memória secundaria.
        fwrite(jogador, sizeof(Jogador), 1, arq);
        if(ferror(arq)!=0)
        {
            printf("Jogador corrompido durante o salvamento!\n");
            salvamento=-1;
        }
        // escreve todos os dados de todos os inimigos de uma só vez na memória secundaria.
        fwrite(inimigos, sizeof(Inimigo), num_inimigos, arq);
        if(ferror(arq)!=0)
        {
            printf("Inimigos corrompidos durante o salvamento!\n");
            salvamento=-1;
        }
        fclose(arq);
        salvamento=0;  // salvamento bem sucedido
    }
    return salvamento;
}

// contraparte da função acima, carregando o arquivo binário criado anteriormente
int carrega_jogo(char mapa[][COLUNAS], Jogador *dados_jogador, Inimigo dados_inimigos[], int num_inimigos){

    char arquivo[ENDERECO]="Salvamento Manual.bin";
    int carregamento; // retorno para saber se foi bem sucedido ou não o carregamento do jogo.
    FILE *arq;

    arq = fopen(arquivo, "rb");
    if(arq==NULL){
        printf("Erro ao abrir o jogo salvo!\n");
        carregamento=-1;
    }
    else
    {
        // lê o mapa de uma só vez no arquivo e passa para a memória principal
        fread(mapa, sizeof(char), LINHAS*COLUNAS, arq);
        if(ferror(arq)!=0)
        {
            printf("Mapa corrompido durante o carregamento!\n");
            carregamento=-1;
        }
        // lê todos os dados do jogador e passa para a memória principal.
        fread(dados_jogador, sizeof(Jogador), 1, arq);
        if(ferror(arq)!=0)
        {
            printf("Jogador corrompido durante o carregamento!\n");
            carregamento=-1;
        }
        // lê todos os dados de todos os inimigos e joga na memória principal
        fread(dados_inimigos, sizeof(Inimigo), num_inimigos, arq);
        if(ferror(arq)!=0)
        {
            printf("Inimigos corrompidos durante o carregamento!\n");
            carregamento=-1;
        }
        fclose(arq);
        carregamento=0;  // carregamento bem sucedido
    }
    return carregamento;
}

// Função que trata a pssagem de nível. Chamada logo após o jogador pegar 5 chaves
int passa_nivel(Jogador *jogador, Inimigo inimigos[], int *num_inimigos, char mapa[][COLUNAS]){
    char nome_arquivo[ENDERECO];
    int status=0;

    jogador->nivel++;
    jogador->chaves=0;
    jogador->pontos+=200;

    // Com base no nível do jogador, chama o próximo mapa e pega a posição do jogador nesse mapa
    switch(jogador->nivel){
        case 2:
            strcpy(nome_arquivo, "Mapa02.txt");
            jogador->pos_jogador=le_mapa(nome_arquivo, mapa, inimigos, num_inimigos);
            break;
        case 3:
            strcpy(nome_arquivo, "Mapa03.txt");
            jogador->pos_jogador=le_mapa(nome_arquivo, mapa, inimigos, num_inimigos);
            break;
        case 4:
            strcpy(nome_arquivo, "Mapa04.txt");
            jogador->pos_jogador=le_mapa(nome_arquivo, mapa, inimigos, num_inimigos);
            break;
        case 5:
            strcpy(nome_arquivo, "Mapa05.txt");
            jogador->pos_jogador=le_mapa(nome_arquivo, mapa, inimigos, num_inimigos);
            break;
        case 6:
            strcpy(nome_arquivo, "Mapa06.txt");
            jogador->pos_jogador=le_mapa(nome_arquivo, mapa, inimigos, num_inimigos);
            break;
        default:
            status=-1;  // nesse caso, não se espera que haja mais do que 6 mapas (ou valores menores que 1)
    }
    return status;
}
