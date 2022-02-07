//
// Created by vsluis on 7/25/21.
//

#ifndef TRABALHO1RC_BATALHA_NAVAL_BATTLESHIP_H
#define TRABALHO1RC_BATALHA_NAVAL_BATTLESHIP_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#define TAM 15
#define ROW_MATRIX_LENGTH 13 /*Tamanho máximo que cada linha da matriz de barcos pode ter*/
#define MATRIX_ROWS 11  /*Total são 11 barcos
                         *1 porta-aviões
                         *2 navios-tanque
                         *3 torpedeiros
                         *5 submarinos*/

/**
 * Estrutura do tabuleiro
 * board é tabuleiro em sí
 * nAttack é o número de attacks feito.
 * nHits é o numero de acertos feito.
 */
typedef struct board_ship {
    int board[TAM][TAM];
    int nAttacks;
    int nHits;
} Board;

/**
 * Ordem de inicialização do tabuleiro
 */

/**
 * Inicializa a estrutura board_ship, board será iniciado com '~' que é o oceano,
 * nAttacks = 0 e nHits = 0.
 * @param b ponteiro para o tabuleiro que será inicializado.
 */
void bootUpBoard(Board *b);
/**
 * Vai inicializar a matriz dos barcos "shipsMatrix" que se encontra na implementação do .h
 * @param arqShipsPositions arquivo com os barcos
 */
void setShips(FILE* arqShipsPositions);
/**
 * Posiciona os barcos lidos no arquivo na estrutura Board
 * @param b ponteiro para o tabuleiro que será posicionado os barcos
 * @return 1 caso tenha posicionado corretamente, 0 caso algum erro tenha ocorrido.
 */
bool positionShips(Board *b);

/**
 * Posiciona os barcos aleatoriamente, sem precisar de um arquivo.
 * @param b ponteiro para o tabulerio que será posicionado os barcos
 */
void randomPositionShips(Board *b);
/**
 * Retorna uma linha válida no tabuleiro
 * @return linha do tabuleiro
 */
int randomRow();
/**
 * Retorna uma coluna válida no tabuleiro
 * @return coluna do tabuleiro
 */
int randomCollum();

/**
 * Mostra o tabuleiro, com todos os barcos posicionados, e também os tiros do inimigo.
 * @param b tabuleiro que deseja printar
 */
void showMyBoard(Board b);
/**
 * Mostra o tabuleiro sem mostrar a posição dos barcos.
 * @param b tabuleiro que deseja printar
 */
void showEnemyBoard(Board b);
/**
 * Realiza um disparo no tabuleiro e atualiza o tabuleiro.
 * @param b ponteiro para o tabuleiro que deseja atirar.
 * @param row linha do tabuleiro
 * @param collum coluna do tabuleiro
 * @return 1 caso tenha acertado algum barco, 0 caso contrário
 */
bool shoot(Board *b, int row, int collum);
/**
 * Verifica se houve um fim de jogo
 * @param b tabuleiro a ser verificado
 * @return 1 caso o jogo tenha acabado, 0 caso contrário.
 */
bool gameOver(Board b);



#endif //TRABALHO1RC_BATALHA_NAVAL_BATTLESHIP_H
