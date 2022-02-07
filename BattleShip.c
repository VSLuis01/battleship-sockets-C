//
// Created by vsluis on 7/25/21.
//
#include "BattleShip.h"

/*Matriz com todas as posições de cada navio*/
char shipsMatrix[MATRIX_ROWS][ROW_MATRIX_LENGTH];

/**
 * Estados do tabuleiro.
 * Representação gráfica:
 * ocean = '~'   oceano
 * missHit = '*' errou o tiro
 * strike = 'X'  acertou o tiro
 */
enum boardStates {
    ocean = -1, missHit, strike
};

/**
 * Barcos possíveis e o seu tamanho
 * submarino = 2 tamanho
 * torpedeiro = 3 tamanho
 * navio-tanque = 4 tamanho
 * porta-aviões = 5 tamanho
 */
typedef enum ships {
    submarine = 2, torpedoBoat, tankShip, aircraftCarrier
} Ships;

/**
 * Direções possíveis para o posicionamento dos navios no tabuleiro.
 */
typedef enum direction {
    horizontal, vertical
} Direction;

/**
 * Sentidos possíveis:
 * horizontal para direita, horizontal para esquerda, vertical para cima e vertical para baixo.
 */
typedef enum way {
    horizontalRight, horizontalLeft,
    verticalTop, verticalBottom
} Way;

/**
 * Inicia toda a matriz boar com ocean, e também as variáveis com 0
 * @param b board
 */
void bootUpBoard(Board *b) {
    for (int i = 0; i < TAM; ++i) {
        for (int j = 0; j < TAM; ++j) {
            b->board[i][j] = ocean;
        }
    }
    b->nHits = 0;
    b->nAttacks = 0;
}

/**
 * Lê o arquivo de barcos e coloca na matriz shipsMatrix.
 * @param arq arquivo de barcos
 */
void readArq(FILE *arq) {
    int i = 0;
    while (!feof(arq)) {
        fgets(shipsMatrix[i], ROW_MATRIX_LENGTH, arq);
        i++;
    }
}

void setShips(FILE *arqShipsPositions) {
    readArq(arqShipsPositions);
}

/**
 * Verifica se alguma linha da shipsMatrix está inválida.
 * Por exemplo, verifica se o tamanho do navio corresponde com o inicio e fim das linhas ou colunas passadas.
 * @param ship algum navio
 * @param startRow inicio da linha
 * @param endRow final da linha
 * @param startCollum inicio da coluna
 * @param endCollum final da coluna
 * @param dir direção
 * @return 1 caso a entrada seja válida, 0 caso contrário
 */
bool inputVerify(int ship, int startRow, int endRow,
                 int startCollum, int endCollum, int dir) {

    int rowVer;
    int collumVer;
    //Valor absoluto do começo e final da linha. Em outras palavras aqui são obtidos o tamanho do navio
    rowVer = abs(startRow - endRow) + 1;
    collumVer = abs(startCollum - endCollum) + 1;

    /*verifica se as coordenadas passadas para cada navio, corresponde com o tamanho de cada um
     * Se dir for na vertical, compara com rowVer.
     * Se dir for na horizontal, compara com a collumVer*/
    switch (ship) {
        case aircraftCarrier:
            if (dir == vertical) {
                if (rowVer != aircraftCarrier) return false;
                /*além da diferença das linhas terem que ser iguais ao tamanho do navio,
                 * as colunas precisam ser iguais*/
                else return startCollum == endCollum ? true : false;
            } else if (dir == horizontal) {
                if (collumVer != aircraftCarrier) return false;
                /*além da diferença das colunas terem que ser iguais ao tamanho do navio,
                 * as linhas precisam ser iguais*/
                else return startRow == endRow ? true : false;
            }
            break;
            /*O mesmo explicado ali em cima vale para todos os cases*/
        case tankShip:
            if (dir == vertical) {
                if (rowVer != tankShip) return false;
                else return startCollum == endCollum ? true : false;
            } else if (dir == horizontal) {
                if (collumVer != tankShip) return false;
                else return startRow == endRow ? true : false;
            }
            break;
        case torpedoBoat:
            if (dir == vertical) {
                if (rowVer != torpedoBoat) return false;
                else return startCollum == endCollum ? true : false;
            } else if (dir == horizontal) {
                if (collumVer != torpedoBoat) return false;
                else return startRow == endRow ? true : false;
            }
            break;
        case submarine:
            if (dir == vertical) {
                if (rowVer != submarine) return false;
                else return startCollum == endCollum ? true : false;
            } else if (dir == horizontal) {
                if (collumVer != submarine) return false;
                else return startRow == endRow ? true : false;
            }
            break;
        default:
            return false;
    }
    return true;
}

/**
 * Uma gambiarra para conseguir pegar cada linha do shipsMatrix e colocar em variáveis separadas.
 * Deve dar para fazer de outro jeito mas estou sem tempo.
 * @param startR Start Row
 * @param endR End Row
 * @param startC Start Collum
 * @param endC End Collum
 * @param line linha que está lendo da matriz shipsMatrix
 * @return retorna a representação do navio. S, P, etc.
 */
char formatMatrix(int *startR, int *endR, int *startC, int *endC, int line) {
    char *pch;
    int i = 0;
    pch = strtok(shipsMatrix[line], " -");
    while (pch != NULL) {
        pch = strtok(NULL, " -");
        ++i;
        if (i == 1) {
            *startR = atoi(pch);
        } else if (i == 2) {
            *startC = (int) pch[0];
        } else if (i == 3) {
            *endR = atoi(pch);
        } else if (i == 4) {
            *endC = (int) pch[0];
        }
    }
    return shipsMatrix[line][0];
}

/**
 * Função responsável por encontrar o sentido e a direção que está posicionado o navio.
 * De acordo com o começo e final da linha ou começo e final da coluna
 * @param direction direção
 * @param way sentido
 * @param startRow começo da linha
 * @param endRow final da linha
 * @param startCollum começo da coluna
 * @param endCollum final da coluna.
 */
void findWayAndDirection(int *direction, int *way, int startRow, int endRow, int startCollum, int endCollum) {
    /*se o começo e fim da linha são iguais, isso significa que o navio está posicionado na horizontal
     * já que o tamanho será definido pelas colunas*/
    if (startRow == endRow) {
        *direction = horizontal;
        /*Agora o sentido é só verificar se o começo é maior que o fim, se for, então o navio vai para a esquerda
         * por exemplo, 'g' > 'a'*/
        if (startCollum > endCollum) {
            *way = horizontalLeft;
        } else {
            *way = horizontalRight;
        }
    }
        /*o mesmo explicado lá em cima vale aqui, análogo*/
    else if (startCollum == endCollum) {
        *direction = vertical;
        if (startRow > endRow) {
            *way = verticalTop;
        } else {
            *way = verticalBottom;
        }
    }
}

/**
 * Aqui é onde de fato coloca os navio no tabuleiro b passado. É verificado se o navio vai passar das bounds
 * do tabuleiro e também se vai "atropelar" outro navio
 * @param b tabuleiro onde vai colocar os navios
 * @param ship navio q ser colocado
 * @param direction direção
 * @param way sentido
 * @param startRow inicio da linha
 * @param startCollum inicio da coluna
 * aqui só é preciso o inicio da coluna e da linha pois já temos o sentido e a direção.
 * @return 1 caso tenha colocado o barco com sucesso no tabuleiro, 0 caso contrário
 */
bool place(Board *b, int ship, int direction,
           int way, int startRow, int startCollum) {

    /*Seleciona a direção e o sentido para colocar o navio corretamente no tabuleiro*/
    switch (direction) {
        case horizontal:
            switch (way) {
                /*caso for horizontal para direita*/
                case horizontalRight:
                    /*verifica se o navio saiu das bordas do tabuleiro*/
                    if (startCollum + ship > TAM - 1) {
                        return false;
                    }
                    /*Verifica se o navio vai "atropelar" outro navio*/
                    for (int j = 0; j < ship; ++j) {
                        if (b->board[startRow][startCollum + j] != ocean)
                            return false;
                    }
                    /*Se passar por todas aquelas verificações, finalmente coloca o navio no tabuleiro*/
                    for (int j = 0; j < ship; ++j)
                        b->board[startRow][startCollum + j] = ship;
                    break;
                case horizontalLeft:
                    /*verifica se o navio saiu das bordas do tabuleiro*/
                    if (startCollum - ship < 0) {
                        return false;
                    }
                    /*Verifica se o navio vai "atropelar" outro navio*/
                    for (int j = 0; j < ship; ++j)
                        if (b->board[startRow][startCollum - j] != ocean) return false;

                    for (int j = 0; j < ship; ++j)
                        b->board[startRow][startCollum - j] = ship;
                    break;
                default:
                    break;
            }
            break;
        case vertical:
            switch (way) {
                case verticalTop:
                    /*verifica se o navio saiu das bordas do tabuleiro*/
                    if (startRow - ship < 0) {
                        return false;
                    }
                    /*Verifica se o navio vai "atropelar" outro navio*/
                    for (int j = 0; j < ship; ++j)
                        if (b->board[startRow - j][startCollum] != ocean) return false;

                    for (int j = 0; j < ship; ++j)
                        b->board[startRow - j][startCollum] = ship;
                    break;
                case verticalBottom:
                    /*verifica se o navio saiu das bordas do tabuleiro*/
                    if (startRow + ship > TAM - 1) {
                        return false;
                    }
                    /*Verifica se o navio vai "atropelar" outro navio*/
                    for (int j = 0; j < ship; ++j)
                        if (b->board[startRow + j][startCollum] != ocean) return false;

                    for (int j = 0; j < ship; ++j)
                        b->board[startRow + j][startCollum] = ship;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return true;
}

/**
 * Verifica se os valores passados estão no intervalo do tabuleiro
 * @param row
 * @param collum
 * @return 1 caso todos valores estejam no intervalo, 0 caso contrário
 */
int checkInterval(int row, int collum) {
    if ((row >= 1 && row <= 15) && (collum >= 'a' && collum <= 'o')) {
        return 1;
    }

    return 0;
}

/**
 * É chamado quando for posicionar os navios na tabuleiro
 * @param b ponteiro para o tabuleiro que será colocado os navios
 * @return 1 caso todos navios tenham sidos posicionados corretamento, 0 caso contrário
 */
bool positionShips(Board *b) {
    int i = 0; //linha da matriz com contém as posições dos navios shipsMatrix
    int startRow;
    int endRow;
    int startCollum;
    int endCollum;
    char ship;

    int direction, way;

    while (i < MATRIX_ROWS) {
        /*Formata a matriz shipsMatrix e atribui a startRow, endRow... os seus valores,
         * depois retorna qual navio que foi lido*/
        ship = formatMatrix(&startRow, &endRow, &startCollum, &endCollum, i);

        /*verifica se as entradas então no intervalo correto*/
        if(!checkInterval(startRow, startCollum)) return false;
        if(!checkInterval(endRow, endCollum)) return false;


        /*Dependendo do navio lido, atribui em ship o tamanho do navio*/
        switch (ship) {
            case 'P':
                ship = aircraftCarrier;
                break;
            case 'N':
                ship = tankShip;
                break;
            case 'T':
                ship = torpedoBoat;
                break;
            case 'S':
                ship = submarine;
                break;
            default:
                /*Se não foi lido nenhum desses navios, então a entrada é inválida*/
                return false;
        }
        /*Descobre a direção e o sentido do navio, e atribui em direction e way*/
        findWayAndDirection(&direction, &way, startRow, endRow, startCollum, endCollum);

        /*Verifica se a entrada dos navios foi correta e corresponde ao tamanho de cada um deles*/
        if (!inputVerify(ship, startRow + 1, endRow + 1, startCollum, endCollum, direction)) {
            return false;
        }

        /*subtrai 1 da linha para colocar na matriz, no tabuleiro*/
        startRow -= 1;
        endRow -= 1;
        /*subtrai 97 de startCollum, pois já q é passado um char, precisa converter para um valor válido no tabuleiro*/
        /*a função place verifica se o navio ultrapassa os bounds do tabuleiro ou se atropela algum outro navio*/
        if (!place(b, ship, direction, way, startRow, startCollum - 97)) {
            return false;
        }
        /*lê mais uma linha da matriz shipsMatrix*/
        ++i;
    }
    /*Depois de tantas verificações retorna que tudo foi colocado corretamente.
     * E certeza que ainda vai ter um bug que eu não vi.*/
    return true;
}

/**
 * Função enorme, me desculpe. Mas aqui será posicionado os navios pseudo-aleatoriamente.
 * Será sorteado um navio aleatório. 5 navios na vertical e 6 na horizontal, a direção será aleatória.
 * @param b tabuleiro a posicionar os navios.
 * Sim, essa implementação ta horrível, mas não consegui pensar em mais nada.
 */
void randomPositionShips(Board *b) {
    //srand(time(NULL));
    bool inserted = true;

    /*quantidades possíveis de cada navio*/
    int P = 1, N = 2, T = 3, S = 5;

    int verticalDir, horizontalDir;
    int randShip; /*navio sorteado*/
    int randRowStart; /*linha sorteada*/
    int randCollumStart; /*coluna sorteada*/

    int i = 0;
    /*Esse primeiro while sorteia 5 navios na vertical*/
    while (i < 5) {
        /*Se um navio foi inserido corretamente, pode sortear outro navio
         * Porém se não foi inserido corretamente, não precisa sortear outro navio,
         * apenas sorteia outras coordenadas.*/
        if (inserted) randShip = submarine + (rand() % (aircraftCarrier - 1));
        randCollumStart = rand() % TAM;
        randRowStart = rand() % TAM;
        verticalDir = rand() % 2; //0 é top, 1 é bottom

        /*Não vai adicionar mais submarinos se já acabou os disponíveis*/
        if (randShip == submarine && S != 0) {
            /*Insere na vertical pra cima*/
            if (verticalDir == 0) {
                inserted = place(b, submarine, vertical, verticalTop, randRowStart, randCollumStart);
            }
            /*Insere na vertical pra baixo*/
            else {
                inserted = place(b, submarine, vertical, verticalBottom, randRowStart, randCollumStart);
            }
            /*Se o navio foi inserido corretamente, então pode subtrair 1 do total de submarinos
             * e também adicionar 1 no i, sendo a quantidade de navios posicionados*/
            if (inserted) {
                --S;
                ++i;
            }
            /*A MESMA LÓGICA DOS COMENTÁRIO A CIMA SERVE PARA OS OUTROS NAVIOS, ÚNICA COISA QUE MUDA É O NAVIO*/
        } else if (randShip == torpedoBoat && T != 0) {
            if (verticalDir == 0) {
                inserted = place(b, torpedoBoat, vertical, verticalTop, randRowStart, randCollumStart);
            } else {
                inserted = place(b, torpedoBoat, vertical, verticalBottom, randRowStart, randCollumStart);
            }
            if (inserted) {
                --T;
                ++i;
            }
        } else if (randShip == tankShip && N != 0) {
            if (verticalDir == 0) {
                inserted = place(b, tankShip, vertical, verticalTop, randRowStart, randCollumStart);
            } else {
                inserted = place(b, tankShip, vertical, verticalBottom, randRowStart, randCollumStart);
            }
            if (inserted) {
                --N;
                ++i;
            }

        } else if (randShip == aircraftCarrier && P != 0) {
            if (verticalDir == 0) {
                inserted = place(b, aircraftCarrier, vertical, verticalTop, randRowStart, randCollumStart);
            } else {
                inserted = place(b, aircraftCarrier, vertical, verticalBottom, randRowStart, randCollumStart);
            }
            if (inserted) {
                --P;
                ++i;
            }
        }
    }

    /*Aqui será posicionado o restante dos navios na horizontal*/
    i = 0;
    inserted = true;
    while (i < 6) {
        /*Se um navio foi inserido corretamente, pode sortear outro navio
         * Porém se não foi inserido corretamente, não precisa sortear outro navio,
         * apenas sorteia outras coordenadas.*/
        if (inserted) randShip = submarine + (rand() % (aircraftCarrier - 1));

        /*Essa parte aqui é total gambiarra, pois quando só tem
         * um tipo de navio disponível, o rand pode ficar um tempão sorteando outros
         * que não podem mais serem posicionais. Então, quando só resta um navio que pode ser sorteado
         * é atribuido em randShip esse navio.*/
        if (P == 0 && N == 0 && T == 0) randShip = submarine;
        else if (P == 0 && N == 0 && S == 0) randShip = torpedoBoat;
        else if (P == 0 && T == 0 && S == 0) randShip = tankShip;
        else if (N == 0 && T == 0 && S == 0) randShip = aircraftCarrier;

        randCollumStart = rand() % TAM;
        randRowStart = rand() % TAM;
        horizontalDir = rand() % 2;

        /*AQUI É APLICADO A MESMA LÓGICA DOS COMENTÁRIOS EXPLICADOS NA VERTICAL
         * A DIFERENÇA SÓ É QUE AGORA ESTÁ SENDO POSICIONA NA HORIZONTAL*/
        if (randShip == submarine && S != 0) {
            if (horizontalDir == 0) {
                inserted = place(b, submarine, horizontal, horizontalRight, randRowStart, randCollumStart);
            } else {
                inserted = place(b, submarine, horizontal, horizontalLeft, randRowStart, randCollumStart);
            }
            if (inserted) {
                --S;
                ++i;
            }
        } else if (randShip == torpedoBoat && T != 0) {
            if (horizontalDir == 0) {
                inserted = place(b, torpedoBoat, horizontal, horizontalRight, randRowStart, randCollumStart);
            } else {
                inserted = place(b, torpedoBoat, horizontal, horizontalLeft, randRowStart, randCollumStart);
            }
            if (inserted) {
                --T;
                ++i;
            }
        } else if (randShip == tankShip && N != 0) {
            if (horizontalDir == 0) {
                inserted = place(b, tankShip, horizontal, horizontalRight, randRowStart, randCollumStart);
            } else {
                inserted = place(b, tankShip, horizontal, horizontalLeft, randRowStart, randCollumStart);
            }
            if (inserted) {
                --N;
                ++i;
            }

        } else if (randShip == aircraftCarrier && P != 0) {
            if (horizontalDir == 0) {
                inserted = place(b, aircraftCarrier, horizontal, horizontalRight, randRowStart, randCollumStart);
            } else {
                inserted = place(b, aircraftCarrier, horizontal, horizontalLeft, randRowStart, randCollumStart);
            }
            if (inserted) {
                --P;
                ++i;
            }
        }
    }
}

/**
 *
 * @return gera uma linha aleatória
 */
int randomRow() {
    return rand() % TAM + 1;
}
/**
 *
 * @return gera uma coluna aleatória
 */
int randomCollum() {
    return rand() % 15 + 97;
}

/**
 * Atualiza o tabuleiro com a flag hit
 * @param b tabuleiro a ser modificado
 * @param row linha
 * @param collum coluna
 * @param hit flag
 */
void modifyBoard(Board *b, int row, int collum, int hit) {
    if(b->board[row][collum] != strike) {
        b->board[row][collum] = hit;
    }
}

/**
 * Verifica se o ataque acertou ou não algum navio
 * @param b tabuleiro a ser verificado
 * @param row linha
 * @param collum coluna
 * @return 1 caso tenha acertado, 0 caso contrário
 */
bool hit(Board b, int row, int collum) {
    return
            (b.board[row][collum] != ocean) &&
            (b.board[row][collum] != missHit) &&
            (b.board[row][collum] != strike) ? true : false;
}

/**
 * É feito o tiro no tabuleiro
 * @param b ponteiro para o tabuleiro que foi feito o tiro
 * @param row linha
 * @param collum coluna
 * @return 1 caso o tiro tenha sido certeiro, 0 caso contrário.
 */
bool shoot(Board *b, int row, int collum) {
    row -= 1;
    collum -= 97;
    if (hit(*b, row, collum)) {
        modifyBoard(b, row, collum, strike);
        return true;
    } else {
        modifyBoard(b, row, collum, missHit);
        return false;
    }
}

/**
 * Verifica se o jogo acabou. Se houve 32 acertos, é pq acertou todas as partes de todos navios
 * @param b tabuleiro a ser verificado se acertou
 * @return 1 caso tenha acabado o jogo, 0 caso contrario
 */
bool gameOver(Board b) {
    return b.nHits == 32 ? true : false;
}

/**
 * showMyBoard quer dizer que mostrará o tabuleiro com todas as flags, ou seja
 * mostrará os navios, os acertos e erros.
 * @param b
 */
void showMyBoard(Board b) {
    printf("\ta \tb \tc \td \te \tf \tg \th \ti \tj \tk \tl \tm \tn \to");
    printf("\n");
    for (int row = 0; row < TAM; row++) {
        printf("%d", row + 1);
        for (int collum = 0; collum < TAM; collum++) {
            if (b.board[row][collum] == missHit) {
                printf("\t*");
            } else if (b.board[row][collum] == strike) {
                printf("\tX");
            } else if (b.board[row][collum] == submarine) {
                printf("\tS");
            } else if (b.board[row][collum] == torpedoBoat) {
                printf("\tT");
            } else if (b.board[row][collum] == tankShip) {
                printf("\tN");
            } else if (b.board[row][collum] == aircraftCarrier) {
                printf("\tP");
            } else {
                printf("\t~");
            }
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * showEnemyBoard quer dizer que mostrará o tabuleiro apenas com as flags de acertos ou erros.
 * @param b
 */
void showEnemyBoard(Board b) {
    printf("\ta \tb \tc \td \te \tf \tg \th \ti \tj \tk \tl \tm \tn \to");
    printf("\n");
    for (int row = 0; row < TAM; row++) {
        printf("%d", row + 1);
        for (int collum = 0; collum < TAM; collum++) {
            if (b.board[row][collum] == missHit) {
                printf("\t*");
            } else if (b.board[row][collum] == strike) {
                printf("\tX");
            } else {
                printf("\t~");
            }
        }
        printf("\n");
    }
    printf("\n");
}
