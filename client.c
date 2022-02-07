/* To compile this file in Linux, type:   gcc -o client main.c -lpthread -w*/

#include <stdio.h> /*Standard Input/Output library*/
#include <stdlib.h> /*Standard C library includes functions like type conversion, memory allocation, proccess control*/
#include <string.h> /*Include String functions*/
#include <unistd.h> /*Includes all definitions like null pointer exceptions*/
#include <arpa/inet.h> /*in_addr_t and in_port_t related definitions*/
#include <sys/types.h> /*It enables us to use Inbuilt functions such as clocktime, offset, ptrdiff_t for substracting two pointers*/
#include <netinet/in.h> /*Contains constants and structures needed for internet domain addresses*/
#include <sys/socket.h> /*Socket operations, definitions and its structures*/
#include <pthread.h> /* For creation and management of threads*/
#include <stdbool.h> /*For using boolean data type*/
#include <netdb.h>
#include "BattleShip.h"

/*Defines*/
#define BUFFER_LEN 500
#define NAME_LEN 50
#define STR_LEN 200


/*Global variables*/

char myName[NAME_LEN];
char enemyName[NAME_LEN];

pthread_t serverThreadID; /*thread_id que trata da comunicação com o servidor*/
pthread_t enemyThreadID; /*thread_id que trata de abrir a conexão com o oponente que escolheu desafiar*/

int serverSocket, /*Socket que faz a comunicacao com o servidor em sí*/
opponSocket, /*Socket que abre a comunicação com o oponente que desafio*/
serverSideSocket, /*Socket responsável por aceitar a conexao do oponente que o desafio*/
opponCliServerSide; /*Socket do oponente que enviou o desafio*/


int myPort; /*Porta única utilizada para os oponentes conseguirem se conectar*/


/*Global threads and function*/

/*Thread que trata da comunicação com o servidor. Comandos por exemplo*/
void *server_client_thread();

/*Thread usada quando desafio alguém,
 *nela é criado outro socket que abre
 *a comunicação com o oponente*/
void *enemy_thread_handle(void *arg);

/**
 * Função responsável pela lógica do jogo batalha naval em só
 * @param socket é o descritor do oponente
 * @param buffer utilizado para troca de mensagens. Coordenadas de ataque por exemplo.
 * @param playerID identificador que define quem será o primeiro ou segundo jogador
 *                 1 - Primeiro a fazer o ataque
 *                 2 - Primeiro a receber o ataque
 */
void playBattleShip(int socket, char *buffer, int playerID);

void closeSockets() {
    close(serverSocket);
    close(opponSocket);
    close(serverSideSocket);
    close(opponCliServerSide);
}

void getNickName() {
    do {
        printf("\nEntre com seu nome: ");
        fgets(myName, sizeof(myName), stdin);
        myName[strlen(myName) - 1] = '\0';
    } while (strlen(myName) < 1);
}

int main(int argc, char **argv) {
    struct hostent *hostTable; /*ponteiro para a tabela de entrada do host*/
    struct protoent *protocol; /*ponteiro pra entrada da tabela de protocolo*/
    int port; /*numero da porta*/
    char *host; /*ponteiro para o host*/
    struct sockaddr_in serverAddrs;
    char buffer[BUFFER_LEN];
    ssize_t len;

    srand(time(NULL));

    memset(&serverAddrs, 0, sizeof(serverAddrs)); /*limpa a estrutura sockaddr*/
    serverAddrs.sin_family = AF_INET; /*configura a familia de protocolos internet*/

    /*verificando se o programa foi utilizado da forma correta*/
    if (argc > 2) {
        port = atoi(argv[2]);
        host = argv[1];
    } else {
        fprintf(stderr, "Uso: %s \"ip/nome\" \"porta\"", argv[0]);
        exit(1);
    }
    /*Verificando validade da porta*/
    if (port > 0) {
        serverAddrs.sin_port = htons((u_short) port);
    } else {
        fprintf(stderr, "[!]ERRO: Numero de porta invalido!    (%s)\n", argv[2]);
        exit(1);
    }
    /*Pegando o host na tabela de hosts*/
    hostTable = gethostbyname(host);
    if (hostTable == NULL) {
        fprintf(stderr, "[!]ERRO: Host invalido!    %s\n", host);
        exit(1);
    }
    memcpy(&serverAddrs.sin_addr, hostTable->h_addr, hostTable->h_length);

    /*Mapeia o protocolo TCP para o número de porta*/
    protocol = getprotobyname("tcp");
    if (protocol == NULL) {
        perror("\n[!]ERRO: Falha ao mapear \"tcp\" para o numero de protocolo\n");
        exit(1);
    }
    /* Estabelece conexao com o servidor*/
    serverSocket = socket(AF_INET, SOCK_STREAM, protocol->p_proto);
    if (serverSocket < 0) {
        perror("\n[!]ERRO: Falha ao criar conexao com o servidor!\n");
        close(serverSocket);
        exit(1);
    }

    /*Receber o nickName do usuario*/
    getNickName();

    if (connect(serverSocket, (struct sockaddr *) &serverAddrs, sizeof(struct sockaddr)) < 0) {
        perror("\n[!]ERRO: Falha ao se conectar com o servidor...\n");
        close(serverSocket);
        exit(1);
    }

    if (send(serverSocket, myName, strlen(myName), 0) < 0) {
        perror("\n[!]ERRO: Falha ao enviar nome!\n");
        close(serverSocket);
        exit(1);
    }

    /*RECEBENDO PORTA*/
    if ((len = recv(serverSocket, buffer, sizeof(buffer), 0)) < 0) {
        perror("\n[!]ERRO: Falha ao receber porta!\n");
        close(serverSocket);
        exit(1);
    }
    buffer[len] = '\0';
    printf("%s", buffer);
    fflush(stdout);
    //PEGANDO O NÚMERO DA PORTA QUE FICA LOGO APOS O #
    strtok(buffer, "#");
    char *pch = strtok(NULL, "# ");
    if (pch != NULL) {
        myPort = atoi(pch);
    } else {
        //Se nao tiver a porta quer dizer que a capacidade maxima do servidor foi atingida e fecha o programa
        close(serverSocket);
        exit(1);
    }

    /*Conexao estabelecida. Utiliza uma Thread para lidar com ela separadamente*/
    pthread_create(&serverThreadID, NULL, server_client_thread, NULL);

    /***AQUI É A PARTE QUANDO O CLIENTE RECEBE UM DESAFIO DE OUTRO CLIENTE***/
    /* Quando é convidado para um desafio*/
    /*Lado servidor do cliente*/
    struct sockaddr_in challengerAddr; //Endereço do oponente que enviou o desafio
    struct sockaddr_in clientServ;  //Endereço de quem receber o desafio

    socklen_t socksize = sizeof(challengerAddr);
    memset(&clientServ, 0, sizeof(clientServ));

    clientServ.sin_family = AF_INET;
    clientServ.sin_addr.s_addr = INADDR_ANY;
    clientServ.sin_port = htons((u_short) myPort);

    if ((serverSideSocket = socket(AF_INET, SOCK_STREAM, protocol->p_proto)) < 0) {
        perror("\n[!]ERRO: Erro ao criar socket para comunicacao com desafiante\n");
        closeSockets();
        exit(1);
    }

    if (bind(serverSideSocket, (struct sockaddr *) &clientServ, sizeof(clientServ)) < 0) {
        perror("\n[!]ERRO: Falha ao bindar socket que faz comunicacao com desafiante\n");
        closeSockets();
        exit(1);
    }
    listen(serverSideSocket, 1); /*Só aceita a comunicacao com o desafiante*/

    if ((opponCliServerSide = accept(serverSideSocket, (struct sockaddr *) &challengerAddr, &socksize)) < 0) {
        perror("\n[!]ERRO: Falha ao aceitar conexao com o desafiante!\n");
        closeSockets();
        exit(1);
    }
    memset(buffer, 0x0, BUFFER_LEN);

    /* Se for recebido uma conexao (um desafio) entra no while*/
    while (opponCliServerSide > 0) {
        /*ENVIA PARA O SERVIDOR QUE ESTA EM JOGO*/
        if (send(serverSocket, "jogando", strlen("jogando"), 0) < 0) {
            perror("\n[!]ERRO: Falha ao enviar status para o servidor!\n");
            closeSockets();
            exit(1);
        }
        /*Aqui o serverThreadID tem o valor da thread que está cuidando da conexão com servidor
         É preciso cancelar ela para o input stream ser aqui agora. A conexão com o servidor nao é cancelado
         pois foi feita fora da thread*/
        pthread_cancel(serverThreadID);

        /* Recebe o nome do oponente e coloca no enemyName*/
        if ((len = recv(opponCliServerSide, buffer, BUFFER_LEN, 0)) < 0) {
            perror("\n[!]ERRO: Falha ao receber nome do oponente!\n");
            closeSockets();
            exit(1);
        }
        buffer[len] = '\0';
        strcpy(enemyName, buffer);

        //Se digitar qualquer coisa além de s/n vai recusar
        printf("\n%s desafiou voce. Aceitas? (S/n) ", enemyName);
        fflush(stdout);
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer) - 1] = '\0';

        if ((strcmp(buffer, "s") == 0) || (strcmp(buffer, "S") == 0)) {
            send(opponCliServerSide, buffer, strlen(buffer), 0);
            int playerID = 2; //Quando vc é desafiado, automaticamente vc é o jogador 2
            playBattleShip(opponCliServerSide, buffer, playerID);
        } else {
            send(opponCliServerSide, buffer, strlen(buffer), 0);
            printf("\nVoce recusou o desafio...\n");
        }
        fflush(stdout);

        /*Depois do jogo finalizado, é fechado a conexao com o desafiante e mantém a conexao com o server*/
        memset(enemyName, 0x0, NAME_LEN);
        /*ENVIA PARA O SERVIDOR QUE ESTA EM DISPONIVEL*/
        if (send(serverSocket, "disponivel", strlen("disponivel"), 0) < 0) {
            perror("\n[!]ERRO: Falha ao enviar status para o servidor!\n");
            exit(1);
        }
        /*cria a thread de tratar com os comandos do servidor novamente*/
        pthread_create(&serverThreadID, NULL, server_client_thread, (void *) &serverSocket);
        close(opponCliServerSide);
        /*preparado para aceitar outro desafio*/
        opponCliServerSide = accept(serverSideSocket, (struct sockaddr *) &challengerAddr, &socksize);
    }

    closeSockets();
    return 0;
}

/*VEFICA SE TODOS OS CARACTERES DE *s SÃO DIGITOS*/
int isDigit(char *s) {
    size_t i;
    for (i = 0; i < strlen(s); ++i) {
        if (!isdigit(s[i])) {
            return 0;
        }
    }
    return (int) i;
}

void *server_client_thread() {
    bool printUse = true;
    /* seta cancel state para enable para conseguir cancelar a thread fora dela.*/
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    char buffer[BUFFER_LEN]; /*troca de mensagens*/
    ssize_t len;

    /*roda o while até o usuário digitar sair ou cancelar a thread*/
    while (1) {
        /*Printa as instruções de uso e o nick e ID do usuario*/
        if (printUse) {
            printf("\nVOCE: %s #%d", myName, myPort);
            printf("\nComandos:\n-> listar\n-> desafiar {player id}\n-> limpar\n-> sair\n");
            printUse = false;
        }

        /*Le o comando e separa em 3 argumento: command, p_name e identifier
         * command: comando em sí (listar, desafiar, etc)
         * p_name: nome da entidade (desafiar "pedro"), "pedro" é o p_name
         * identifier: identificador do usuario (desafiar "pedro" 2222), 2222 é o identifier*/
        memset(buffer, 0x0, BUFFER_LEN);
        printf("\n> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer) - 1] = '\0';
        char command[STR_LEN], p_name[STR_LEN], identifier[STR_LEN];
        int n = sscanf(buffer, "%s %s %s", command, p_name, identifier);

        if ((strcmp("desafiar", command) == 0) && (strlen(p_name) >= 1) && (strlen(identifier) >= 1) && (n == 3) &&
            (isDigit(identifier))) {
            int id = atoi(identifier);

            /*Se o usuario tentar desafiar ele mesmo só reinicia a iteração*/
            if (strcmp(p_name, myName) == 0 && id == myPort) {
                printf("\n[!]ERRO: Voce esta desafiando voce mesmo?!\n");
                continue;
            }
            /*envia o comando para o servidor*/
            send(serverSocket, buffer, strlen(buffer), 0);
            memset(buffer, 0x0, BUFFER_LEN);
            /*RECEBE O IP E PORTA DO OPONENTE, CASO NAO TIVER ERRO*/
            len = recv(serverSocket, buffer, BUFFER_LEN, 0);
            buffer[len] = '\0';
            /*Caso nao tenha erro e o oponente nao seja o propio servidor, entra aqui*/
            if ((strstr(buffer, "ERRO") == NULL) && (id != 0 && strcmp(p_name, "Servidor") != 0)) {
                strcpy(enemyName, p_name);
                /*thread para iniciar conexao com o oponente. Essa thread vai tratar de criar uma conexao com o oponente*/
                pthread_create(&enemyThreadID, NULL, enemy_thread_handle, buffer);
                pthread_exit(0);/* Exit this thread. */
            }
                /*Caso o oponente for o servidor, não é necessario criar outra thread,
                 * pode iniciar o jogo passando o socket do servidor*/
            else if (id == 0 && strcmp(p_name, "Servidor") == 0) {
                strcpy(enemyName, p_name);
                printf("%s", buffer);
                playBattleShip(serverSocket, buffer, 1);
                printUse = true;
                /*ENVIA PARA O SERVIDOR QUE ESTA EM DISPONIVEL*/
                if (send(serverSocket, "disponivel", strlen("disponivel"), 0) < 0) {
                    perror("\n[!]ERRO: Falha ao enviar status para o servidor!\n");
                    closeSockets();
                    exit(1);
                }
            }
                /*ENTRA NO ELSE QUANDO OCORREU ALGUM ERRO, QUANDO JOGADOR NAO FOI ENCONTRADO*/
            else {
                printf("\n%s\n", buffer);
            }
        }
            /*lista todos os jogadores online*/
        else if (strcmp("listar", command) == 0) {
            send(serverSocket, buffer, strlen(buffer), 0);
            memset(buffer, 0x0, BUFFER_LEN);
            len = recv(serverSocket, buffer, BUFFER_LEN, 0);
            buffer[len] = '\0';
            printf("\n%s\n", buffer);
        } else if (strcmp("sair", command) == 0) {
            send(serverSocket, buffer, strlen(buffer), 0);
            memset(myName, 0x0, NAME_LEN);
            closeSockets();
            exit(0);
        } else if (strcmp("limpar", command) == 0) {
            system("clear");
            printUse = true;
        } else {
            printUse = true;
            memset(buffer, 0x0, BUFFER_LEN);
        }
    }
}

/*thread que vai abrir a conexao com o oponente que foi desafiado*/
void *enemy_thread_handle(void *arg) {
    arg = (char*) arg;
    struct protoent *protocol; /*ponteiro pra entrada da tabela de protocolo*/

    char ip[INET_ADDRSTRLEN]; /*Ip do oponente, INET_ADDRSRLEN é para IPv4 tamanho 16*/
    int port;   /*porta do oponente*/
    char buffer[BUFFER_LEN];
    ssize_t len;

    if (sscanf(arg, "%s %d", ip, &port) < 2) {
        printf("\n[!]ERRO: Falha ao mapear ip e porta do oponente!\n");
        closeSockets();
        exit(1);
    }
    /* Open connection with the IP address passed as arguement. */
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));

    dest.sin_addr.s_addr = inet_addr(ip);
    dest.sin_family = AF_INET;
    dest.sin_port = htons((u_short) port);


    protocol = getprotobyname("tcp");
    if (protocol == NULL) {
        perror("[!]ERRO: Falha ao mapear \"tcp\" para o numero de protocolo");
        closeSockets();
        exit(1);
    }
    if ((opponSocket = socket(AF_INET, SOCK_STREAM, protocol->p_proto)) < 0) {
        perror("\n[!]ERRO: Falha ao criar socket do oponente!\n");
        closeSockets();
        exit(1);
    }
    printf("\nEnviando desafio...\n");
    if (connect(opponSocket, (struct sockaddr *) &dest, sizeof(struct sockaddr)) < 0) {
        perror("\n[!]ERRO: Falha ao se conectar com o oponente...\n");
        closeSockets();
        exit(1);
    }
    /* Envia pro oponente o myName */
    send(opponSocket, myName, strlen(myName), 0);
    /* Recebe se o oponente aceita ou não o desafio */
    len = recv(opponSocket, buffer, BUFFER_LEN, 0);
    buffer[len] = '\0';
    if ((strcmp(buffer, "s") == 0) || (strcmp(buffer, "S") == 0)) {
        int playerID = 1;
        playBattleShip(opponSocket, buffer, playerID);

    } else {
        printf("\n%s recusou o desafio...\n", enemyName);
    }
    /* After game end, close the connection, recreate thread to start communication
     * with the server and close this thread.
     */
    memset(enemyName, 0x0, NAME_LEN);
    close(opponSocket);
    /*ENVIA PARA O SERVIDOR QUE ESTA EM DISPONIVEL*/
    if (send(serverSocket, "disponivel", strlen("disponivel"), 0) < 0) {
        perror("\n[!]ERRO: Falha ao enviar status para o servidor!\n");
        exit(1);
    }
    /*Cria a thread que faz a comunicacao direto com o servidor*/
    pthread_create(&serverThreadID, NULL, server_client_thread, (void *) &serverSocket);
    pthread_exit(0); /* Exit this thread. */
}

/**
 * Converte todos caracteres de str para lower case
 * @param str string que vai ser convertida
 */
void toLower(char *str) {
    int i = 0;
    while (str[i]) {
        str[i] = (char) tolower(str[i]);
        i++;
    }
}

/**
 * Verifica se o ataque é válido
 * @param row linha
 * @param collum coluna
 * @return 1 caso o ataque seja válido 0 caso contrário.
 */
int verifyAttack(int row, int collum) {
    char rowStr[2];
    /*transforma a coluna para string, para conseguir verificar se todos os caracteres dela sao digitos no isDigit*/
    sprintf(rowStr, "%d", row);
    if (isDigit(rowStr) && isalpha(collum)) {
        if ((row >= 1 && row <= 15) && (collum >= 'a' && collum <= 'o')) {
            return 1;
        }
    }
    return 0;
}

void playBattleShip(int socket, char *buffer, int playerID) {
    system("clear");
    Board myBoard; /*meu tabuleiro do batalha naval*/
    Board enemyBoard; /*tabuleiro do inimigo*/

    /*inicializa os dois tabuleiro*/
    bootUpBoard(&enemyBoard);
    bootUpBoard(&myBoard);

    FILE *ships; /*arquivo que vai guardar os barcos*/
    ships = fopen("barcos.txt", "r");
    if (ships == NULL) {
        perror("\n[!]ERRO: Falha ao carregar arquivo de barcos!"
               "\nInicializando tabuleiro aleatoriamente!\n");
        randomPositionShips(&myBoard);
        //fclose(ships);
    } else {
        setShips(ships);
        if (!positionShips(&myBoard)) {
            fprintf(stderr,
                    "\n[!]ERRO: Formato incorreto no arquivo de barcos!"
                    "\nIniciando tabuleiro aleatoriamente!\n");
            bootUpBoard(&myBoard);
            randomPositionShips(&myBoard);
        } else {
            fprintf(stdout, "\n[!]ATENCAO: Arquivo de barcos lido com sucesso!\n");
        }
        fclose(ships);
    }

    int datasocket = socket;
    ssize_t len = 1;                             /*forçar entrada no while         */
    int player = 1;                              /* NUMERO DO JOGADOR - 1 ou 2     */
    int winner = 0;                              /* JOGADOR VENCEDOR  - 1 caso for player 1,
                                                  * 2 caso player 2                */
    int row = 0;                                 /* LINHA DO TABULEIRO             */
    char column = 0;                             /* COLUNA DO TABULEIRO            */
    int n;                                       /* NUMERO DE ARGUMENTOS SO SSCANF */
    char attackCord[STR_LEN];                    /* COORDENADAS DO ATAQUE          */

    memset(buffer, 0x0, BUFFER_LEN);

    printf("\n****INICIANDO BATALHA NAVAL****\n");

    /*O loop é encerrado caso tive um vencedor,
     *ou caso o cliente fechar conexão, nesse caso len será < 0*/
    while (winner == 0 && len > 0) {

        /*Sai do do while quando n == 2 (entradas válidas) ou len < 0 (alguem desconectou do jogo)*/
        do {
            if (player == playerID) {
                printf("\nEntre com \"M\" para ver o mapa!\n");
                printf("\n%s, entre com a linha e coluna. Ex(1 a): ", myName);
                fgets(attackCord, STR_LEN, stdin);
                attackCord[strlen(attackCord) - 1] = '\0';

                if ((strcmp(attackCord, "M") != 0) && (strcmp(attackCord, "m") != 0)) {
                    printf("\n[!ATENCAO!]: Enviando ataque para o(a) %s\n", enemyName);
                    toLower(attackCord);
                    n = sscanf(attackCord, "%d %c", &row, &column);
                    send(datasocket, attackCord, strlen(attackCord), 0); /*Envia as coordenadas de ataque*/
                } else {
                    send(datasocket, "M", strlen("M"), 0); /*Envia as coordenadas de ataque*/
                    recv(datasocket, &enemyBoard, sizeof(enemyBoard), 0); /*Recebe o tabuleiro do inimigo*/
                    printf("\nTABULEIRO DO(A) %s:\n", enemyName);
                    showEnemyBoard(enemyBoard);
                    printf("\n\nMEU TABULEIRO:\n");
                    showMyBoard(myBoard);
                }
            } else {
                printf("\nEsperando por %s...\n", enemyName);
                len = recv(datasocket, attackCord, STR_LEN, 0); /*Recebe as coordenadas de ataque*/
                attackCord[len] = '\0';
                if (strcmp(attackCord, "M") != 0) {
                    n = sscanf(attackCord, "%d %c", &row, &column);
                    if (n == 2) {
                        printf("\n[!ATENCAO!]: O(a) jogador(a) %s atacou em %d-%c\n", enemyName, row, column);
                    }
                } else {
                    printf("\n[!ATENCAO!]: %s requisitou o tabuleiro. Enviando...\n", enemyName);
                    /*Enviando tabuleiro*/
                    send(datasocket, (const void *) &myBoard, sizeof(myBoard), 0);
                }
            }
            /*Enquanto n != 2, quer dizer que os argumentos passados
             *não foram lidos corretamente, ou seja, teve entrada inválida.
             *Se o len < 0, quer dizer que algum jogador se desconectou.*/
        } while (n != 2 && len > 0);

        if (player == playerID) {
            myBoard.nAttacks++;
            /*RECEBE FLAG SE ACERTOU, ERROU OU ATAQUE INVÁLIDO*/
            len = recv(datasocket, buffer, BUFFER_LEN, 0);
            buffer[len] = '\0';
            /*strstr retorna NULL, caso nao ache a string passada, nesse caso queremos testar se
             * não é um ataque inválido, por isso negar a função*/
            if (!strstr(buffer, "ATAQUE INVALIDO")) {
                if (strcmp(buffer, "ACERTOU") == 0) {
                    /*incrementa o meu numero de acerto*/
                    myBoard.nHits++;
                }
                printf("\n[!ATENCAO!]: Voce %s o ataque(%d-%c) no(a) jogador(a) %s!\n", buffer, row, column, enemyName);
            }
                /*Caso o ataque for inválido, printar a mensagem*/
            else {
                printf("[!ATENCAO!]: %s", buffer);
            }
        } else {
            /*Verifica se o ataque é válido*/
            if (verifyAttack(row, column)) {
                /*Caso for válido, faz o disparo e retorna se acertou ou errou*/
                if (shoot(&myBoard, row, (int) column)) {
                    sprintf(buffer, "ACERTOU");
                    /*ENVIANDO FLAG QUE ACERTOU O ALVO*/
                    send(datasocket, buffer, strlen(buffer), 0);
                    /*Incrementa o número de acertos do inimigo*/
                    enemyBoard.nHits++;
                } else {
                    sprintf(buffer, "ERROU");
                    /*ENVIANDO FLAG QUE ERROU O ALVO*/
                    send(datasocket, buffer, strlen(buffer), 0);
                }
                printf("\n[!ATENCAO!]: O(a) jogador(a) %s %s o ataque(%d-%c)!\n", enemyName, buffer, row, column);
            }
                /*Caso o jogador invente de fazer um ataque inválido, ele perde a vez*/
            else {
                printf("\n[!ATENCAO!]: O(a) jogador(a) %s fez um ataque invalido e perdeu a vez!\n", enemyName);
                sprintf(buffer, "ATAQUE INVALIDO, PERDEU A VEZ :(\n");
                /*ENVIANDO FLAG PQ TENTOU SACANEAR O PROGRAMADOR*/
                send(datasocket, buffer, strlen(buffer), 0);
            }
        }
        /*Verifica se o jogo acabou em algum dos dois tabuleiros.
         * Caso tenha acabado winner recebe o valor de player, que é a vez de determinado jogador
         *Um exemplo: Jogador 1 (playerID = 1), Jogador 2 (playerID = 2)
         *caso player = 2, winner = 2, isso quer dizer que o ID do vencedor
         *é 2, no caso, o Jogador 2*/
        if (gameOver(myBoard) || gameOver(enemyBoard)) {
            winner = player;
        }
        memset(attackCord, 0x0, STR_LEN);
        memset(buffer, 0x0, BUFFER_LEN);
        n = 0;
        /*Seleciona o novo jogador*/
        player = player == 1 ? 2 : 1;
    }
    /*Caso len > 0, quer dizer que saiu do loop sem erros e tivemos um vencedor
     * caso contrário algum jogador desconectou no meio da partida*/
    if (len > 0) {
        if (winner == playerID) {
            printf("\n[!ATENCAO!]: Parabens %s, VOCE EH O(a) VENCEDOR(a)!\n", myName);
            printf("\nSua precisao de acerto: %.2f%% (%d/%d)", ((float) myBoard.nHits / (float) myBoard.nAttacks) * 100,
                   myBoard.nHits, myBoard.nAttacks);
        } else {
            printf("\n[!ATENCAO!]: %s ganhou dessa vez...\n", enemyName);
            printf("\nSua precisao de acerto: %.2f%% (%d/%d)", ((float) myBoard.nHits / (float) myBoard.nAttacks) * 100,
                   myBoard.nHits, myBoard.nAttacks);
        }
    } else {
        printf("\n[!]OPPS: O(a) jogador(a) %s desconectou-se no meio da partida!\n", enemyName);
    }
    sleep(1);
}