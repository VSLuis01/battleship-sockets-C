/* To compile this file in Linux, type:   gcc -o server main.c -lpthread -w*/

#include <sys/types.h> /*It enables us to use Inbuilt functions such as clocktime, offset, ptrdiff_t for substracting two pointers*/
#include <sys/socket.h> /*Socket operations, definitions and its structures*/
#include <netinet/in.h> /*Contains constants and structures needed for internet domain addresses*/
#include <netdb.h> /* Contains definition of network database operations*/
#include <arpa/inet.h>
#include <pthread.h> /* For creation and management of threads*/
#include <stdio.h> /*Standard library of Input/output*/
#include <string.h> /*Contains string function*/
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "BattleShip.h"
#include <signal.h>

/*DEFINES*/
#define BUFFER_LEN 500
#define MAX_CLIENTS 3 //CAPACIDADE MÁXIMA DE 2
#define STR_LEN 200

/*Global variables*/
static int clientCount = 0;

/*Estrutura que guarda os dados dos clientes*/
typedef struct {
    struct sockaddr_in address;
    int socket;
    int ID;
    int port;
    int inGame;
    char name[50];
} Clients;

Clients *clients[MAX_CLIENTS]; /*clientes */
Clients *serverCli;            /*variavel de clients especial para o servidor*/

pthread_mutex_t mutex;         /*mutex*/
pthread_t threadServer;        /*tid*/

/***
 * CAPTURA O CTRL + C PARA TER CERTEZA SE O USUARIO QUER FINALIZAR O SERVIDOR
 * ANTES DE FECHAR O SERVIDOR LIBERA TODOS ESPAÇOS ALOCADO NA MEMORIA E FECHA TODOS OS SOCKETS CASO TIVER ALGUM
 * Parece um destrutor
 * @param sig
 */
void intHandler(int sig) {
    char c;
    signal(sig, SIG_IGN);
    printf("\nDeseja mesmo encerrar o servidor? [S/n]: ");
    c = (char) getchar();
    if (isspace(c) || c == 'S' || c == 's') {
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] != NULL) {
                close(clients[i]->socket);
                free(clients[i]);
            }
        }
        exit(0);
    } else {
        signal(SIGINT, intHandler);
    }
    getchar(); // Get new line character
}

/**
 * Função que vai gerar uma porta aleatória de 10.000 - 11.999
 * @return uma porta nesse intervalo que não esteja sendo usada por outro cliente
 */
int gen_port() {
    int gen;
    int ok = 1; //VERIFICA SE ESTÁ OK LIBERAR A PORTA, ISSO SIGNIFICA QUE NENHUM CLIENTE POSSUI ELA.
    pthread_mutex_lock(&mutex);
    do {
        /*GERA UMA PORTA NO ALCANCE DE 10.000 - 11.999*/
        gen = rand() % 2000 + 10000;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i] != NULL) {
                if (clients[i]->port == gen) {
                    ok = 0;
                    break;
                }
            }
        }
    } while (!ok);
    pthread_mutex_unlock(&mutex);
    return gen;
}

/**
 * Adiciona um cliente em uma posição vaga no array de clientes
 * @param cli ponteiro do cliente a ser inserido
 */
void add_player(Clients *cli) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] == NULL) {
            clients[i] = cli;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
}

/**
 * Remove um cliente do array de clientes
 * @param id identificador único do cliente (porta) para remove-lo
 */
void remove_player(int id) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL) {
            if (clients[i]->ID == id) {
                clients[i] = NULL;
                break;
            }
        }
    }
}

/**
 * Essa função irá recuperar um cliente e escrever no buffer o seu ip e porta
 * @param id identificador do cliente
 * @param buf buffer que vai receber o endereço e a porta do cliente
 */
void get_player(int id, char *buf) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL) {
            if (clients[i]->ID == id) {
                sprintf(buf, "%s %d", inet_ntoa(clients[i]->address.sin_addr), clients[i]->port);
                break;
            }
        }
    }
}

/**
 * Irá retornar se existe o cliente ou não pelo seu identificador
 * @param id identificador único
 * @return retorna um valor maior ou igual a 0 caso o cliente exista na estrutura, ou -1 caso contrário
 */
int player_exist(int id) {
    int ret = -1;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL) {
            if (clients[i]->ID == id) {
                ret = i;
                break;
            }
        }
    }
    return ret;
}

/**
 * Essa função será responsável por fazer a contatenação de todos clientes online no servidor
 * @param buf buffer que irá receber a string da lista de todos clientes online
 */
void list_players(char *buf) {
    char aux[BUFFER_LEN];
    sprintf(buf, "\nJogadores online: ");
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] != NULL) {
            sprintf(aux, "\n-> %s #%d%s", clients[i]->name, clients[i]->port,
                    clients[i]->inGame == 1 ? " (in-game)" : "");
            strcat(buf, aux);
            memset(aux, 0x0, BUFFER_LEN);
//            strcat(buf, "\n-> ");
//            strcat(buf, clients[i]->name);
//            strcat(buf, " #");
//            sprintf(id, "%d", clients[i]->ID);
//            strcat(buf, id);
//            clients[i]->inGame == 1 ? strcat(buf, " (in-game)") : strcat(buf, "");
//            memset(id, 0x0, STR_LEN);
        }
    }
}

/**
 * Thread responsável por fazer a comunicação direta com todos clientes
 * @param arg
 * @return
 */
void *serverThread(void *arg);

int main(int argc, char **argv) {
    struct protoent *protocol;     /*ponteiro para tabela de protocolos*/
    /*Estrutura que contém o nome e o número dos protocolos*/

    struct sockaddr_in serverAddress;     /*Estrutura para guardar o endereço do servidor*/
    struct sockaddr_in clientAddress;     /*Estrutura para guardar o endereço do cliente*/
    int serverMainSocket, clientSocket;   /*Descritores do servidor e do cliente*/
    int port;                             /*porta do servidor*/
    ssize_t len;                          /*tamanho da mensagem recebida*/
    char buf[BUFFER_LEN];                 /*buffer para troca de dados*/

    srand(time(NULL));         /*seed do rand*/

    pthread_mutex_init(&mutex, NULL); /*iniciando o mutex*/

    memset(&serverAddress, 0, sizeof(serverAddress.sin_zero)); /* clear sockaddr structure*/
    serverAddress.sin_family = AF_INET;            /*familia Internet*/
    serverAddress.sin_addr.s_addr = INADDR_ANY;    /*server está preparado para aceitar qualquer endereço*/

    if (argc > 1) { /*verifica se argumento especificado é valido*/
        port = atoi(argv[1]); /*converte o argumento para binario*/
    } else {
        fprintf(stderr, "\nUso: %s \"numero da porta\"", argv[0]);
        exit(1);
    }
    if (port > 0) { /*verificacao de valores invalidos*/
        serverAddress.sin_port = htons((u_short) port);
    } else { /*imprime uma mensagem de erro e sai*/
        fprintf(stderr, "\nNumero de porta invalido: %s\n", argv[1]);
        exit(1);
    }
    /* Ignore pipe signals */
    signal(SIGPIPE, SIG_IGN);

    /*Mapeia o protocolo TCP para o número de porta*/
    protocol = getprotobyname("tcp");
    if (protocol == NULL) {
        perror("[!]ERRO: Falha ao mapear \"tcp\" para o numero de protocolo\n");
        exit(1);
    }

    /* Cria IPv4 socket */
    serverMainSocket = socket(AF_INET, SOCK_STREAM, protocol->p_proto);
    if (serverMainSocket < 0) {
        perror("\n[!]ERRO: Falha ao criar socket do servidor!\n");
        close(serverMainSocket);
        exit(1);
    }

    /* Bind a local address to the socket */
    if (bind(serverMainSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("\n[!]ERRO: Falha no bind do servidor!\n");
        close(serverMainSocket);
        exit(1);
    }

    /* Specify a size of request queue */
    if (listen(serverMainSocket, MAX_CLIENTS) < 0) {
        perror("\n[!]ERRO: Falha no listen!\n");
        close(serverMainSocket);
        exit(1);
    }

    /*Estrutura do "cliente" do servidor que vai estar sempre online*/
    serverCli = (Clients *) malloc(sizeof(Clients));
    serverCli->ID = 0;
    serverCli->socket = serverMainSocket;
    serverCli->address = serverAddress;
    serverCli->port = 0;
    serverCli->inGame = 0;
    add_player(serverCli);
    strcpy(serverCli->name, "Servidor");

    signal(SIGINT, intHandler);
    printf("\n***SERVIDOR INICIADO***\n");
    printf("\nOuvindo na porta: %d\n", port);

    /*loop infinito, vai estar sempre aceitando novas conexões contanto que tenha espaço*/
    while (1) {
        socklen_t cAddrLength = sizeof(clientAddress);                /* length of address */
        /*Aceita conexão do cliente*/
        clientSocket = accept(serverMainSocket, (struct sockaddr *) &clientAddress, &cAddrLength);

        /*Verifica se a capacidade máxima foi atingida
         * Se foi atingida, envia uma mensagem para o cliente indicando isso e fecha a conexão com ele
         * e faz a interação novamente, esperando outra conexão*/
        if ((clientCount + 1) == MAX_CLIENTS) {
            printf("\n[!]ERRO: Capacidade maxima atingida. Conexao rejeitada: %s::%d\n",
                   inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
            sprintf(buf, "\n[!]ERRO: Capacidade maxima do servidor atingida. Conexao rejeitada!\n");
            send(clientSocket, buf, strlen(buf), 0);
            shutdown(clientSocket, SHUT_RDWR);
            close(clientSocket);
            continue;
        }

        /*Recebe o nome do cliente*/
        if ((len = recv(clientSocket, buf, BUFFER_LEN, 0)) < 0) {
            perror("\n[!]ERRO: Falha ao receber o nome!\nFechando conexao!\n");
            close(clientSocket);
        }
            /*Nome recebido com sucesso, então inicia toda a sua estrutura*/
        else {
            buf[len] = '\0';
            /*Configurações do cliente*/
            Clients *cli = (Clients *) malloc(sizeof(Clients));
            cli->address = clientAddress;
            cli->socket = clientSocket;
            cli->port = gen_port();
            cli->ID = cli->port; /*O ID é a porta, pois a porta será única também*/
            cli->inGame = 0;     /*Essa flag indica se o jogador está em jogo ou não, para caso tiver, não aceitar outras conexoes*/
            strcpy(cli->name, buf);
            /*Adiciona um cliente na estrutura e cria uma thread separada para ele*/
            add_player(cli);
            clientCount++;
            /*passa o cliente como argumento para a thread*/
            pthread_create(&threadServer, NULL, serverThread, (void *) cli);
        }
        memset(buf, 0x0, BUFFER_LEN);
        /* Reduce CPU usage */
        sleep(1);
    }
}

/**
 * Lógica do jogo batalha naval, utilizado aqui caso o cliente deseje jogar contra o servidor
 * @param cli cliente que deseja jogar contra o servidor
 * @param buffer buffer para a troca de mensagens
 * @param playerID id do jogador que no caso determina quem será o primeiro a atacar,
 * no caso o servidor sempre será o segundo
 */
void playBattleShip(Clients *cli, char *buffer, int playerID);

/*Thread que lida com a comunicação direta com os clientes*/
void *serverThread(void *arg) {
    Clients *cli = (Clients *) arg;
    char buffer[BUFFER_LEN]; /*buffer para troca de mensagens*/
    int id; /*id do cliente, que também é a porta*/
    int indexCli; /*index que o cliente está no array de clientes*/

    ssize_t len; /*tamanho da mensagem recebida*/

    printf("\n[LOG]: O(a) jogador(a) %s #%d conectou-se\n", cli->name, cli->ID);
    fflush(stdout);
    sprintf(buffer, "\nO(a) jogador(a) %s #%d conectou-se\n", cli->name, cli->ID);

    /*Envia para o cliente que ele está conectado ao servidor*/
    send(cli->socket, buffer, strlen(buffer), 0);

    memset(buffer, 0x0, BUFFER_LEN);

    /*Loop infinito até um cliente fechar a conexão, nesse caso o len será -1 e sairá do loop*/
    while ((len = recv(cli->socket, buffer, BUFFER_LEN, 0)) > 0) {
        buffer[len] = '\0';

        /*Le o comando e separa em 3 argumento: command, p_name e identifier
         * command: comando em sí (listar, desafiar, etc)
         * p_name: nome da entidade (desafiar "pedro"), "pedro" é o p_name
         * identifier: identificador do usuario (desafiar "pedro" 2222), 2222 é o identifier*/
        char command[STR_LEN], p_name[STR_LEN], identifier[STR_LEN];
        sscanf(buffer, "%s %s %s", command, p_name, identifier);
        if ((strcmp(command, "desafiar") == 0) && (strlen(p_name) >= 1) && (strlen(identifier) >= 1)) {
            pthread_mutex_lock(&mutex);

            id = atoi(identifier);
            indexCli = player_exist(id); /*retorna o indice em que o cliente se encontra, ou -1 caso não exista*/

            /*VERIFICA SE O NOME PASSADO CORRESPONDE AO NOME DO ID*/
            if (indexCli != -1) {
                if (strcmp(clients[indexCli]->name, p_name) != 0) indexCli = -1;
            }

            if (indexCli != -1) {
                if ((id != 0) && (clients[indexCli]->inGame == 0)) {
                    memset(buffer, 0x0, BUFFER_LEN);
                    get_player(id, buffer); /*escreve o IP e porta dele no buffer*/
                    printf("\n[LOG]: O(a) cliente %s #%d desafiou %s #%d\n", cli->name, cli->ID, p_name, id);
                    cli->inGame = 1; /*seta o cliente que desafio para in-game*/
                    printf("\n[LOG]: %s esta ocupado!\n", cli->name);
                }
                    /*Caso o id for 0, quer dizer que está desafiando o servidor*/
                else if (id == 0) {
                    /*envia pro cliente que está iniando uma batalha contra o servidor*/
                    sprintf(buffer, "\nINICIANDO BATALHA CONTRA SERVIDOR\n");
                    send(cli->socket, buffer, strlen(buffer), 0);
                    cli->inGame = 1;
                    pthread_mutex_unlock(&mutex);
                    printf("\n[LOG]: O(a) cliente %s iniciou uma batalha contra o Servidor.\n", cli->name);
                    printf("\n[LOG]: %s esta ocupado!\n", cli->name);
                    playBattleShip(cli, buffer, 2);
                }
                    /*Caso o jogador que se deseja desafiar esteja ocupado (em jogo)*/
                else if (clients[indexCli]->inGame == 1) {
                    printf("\n[LOG]: O(a) cliente %s #%d desafiou %s #%d. Porem ele nao esta disponivel\n", cli->name,
                           cli->ID, p_name, id);
                    sprintf(buffer, "\n[!]ERRO: O jogador %s #%d esta em jogo!\n", p_name, id);
                }
            } else {
                sprintf(buffer, "[!]ERRO: Jogador %s #%d nao encontrado!", p_name, id);
            }
            pthread_mutex_unlock(&mutex);
            send(cli->socket, buffer, strlen(buffer), 0);
        }
            /*Listar todos jogadores online*/
        else if (strcmp(command, "listar") == 0) {
            pthread_mutex_lock(&mutex);
            list_players(buffer);
            pthread_mutex_unlock(&mutex);
            printf("\n[LOG]: %s requisitou lista de players.\n", cli->name);
            send(cli->socket, buffer, strlen(buffer), 0);
        }
            /*Enviado para o servidor quando uma partida acaba*/
        else if (strcmp(command, "disponivel") == 0) {
            pthread_mutex_lock(&mutex);
            cli->inGame = 0;
            pthread_mutex_unlock(&mutex);
            printf("\n[LOG]: %s esta disponivel!\n", cli->name);
        }
            /*Enviado para o servidor quando uma partida começa*/
        else if (strcmp(command, "jogando") == 0) {
            pthread_mutex_lock(&mutex);
            cli->inGame = 1;
            pthread_mutex_unlock(&mutex);
            printf("\n[LOG]: %s esta ocupado!\n", cli->name);
        }
        memset(buffer, 0x0, BUFFER_LEN);
    }
    /*QUANDO JOGADOR FECHA CONEXAO, ELE É REMOVIDO DO ARRAY DE CLIENTES E LIBERA A MEMORIA*/
    pthread_mutex_lock(&mutex);
    printf("\n[LOG]: O(a) Jogador(a) %s #%d desconectou-se.\n", cli->name, cli->ID);
    fflush(stdout);
    remove_player(cli->ID);
    clientCount--;
    close(cli->socket);
    free(cli); //liberado o espaço da memoria
    pthread_mutex_unlock(&mutex);
    //pthread_exit(0);
    pthread_detach(pthread_self());
    return NULL;
}

/**
 * Verifica se todos os dígitos da string são digitos
 * @param s string que será verificada
 * @return  torna um número diferente de zero caso todos os caracteres sejam dígitos, 0 caso contrário
 */
int isDigit(char *s) {
    size_t i;
    for (i = 0; i < strlen(s); ++i) {
        if (!isdigit(s[i])) {
            return 0;
        }
    }
    return (int) i;
}

/**
 * Verifica se o ataque recebido é válido
 * @param row linha
 * @param collum coluna
 * @return retorna 1 caso o ataque recebido for válido, 0 caso contrário
 */
int verifyAttack(int row, int collum) {
    char rowStr[2];
    sprintf(rowStr, "%d", row);
    if (isDigit(rowStr) && isalpha(collum)) {
        if ((row >= 1 && row <= 15) && (collum >= 'a' && collum <= 'o')) {
            return 1;
        }
    }
    return 0;
}

/**
 * Lógica do jogo batalha naval, utilizado aqui caso o cliente deseje jogar contra o servidor
 * @param cli cliente que deseja jogar contra o servidor
 * @param buffer buffer para a troca de mensagens
 * @param playerID id do jogador que no caso determina quem será o primeiro a atacar,
 * no caso o servidor sempre será o segundo
 */
void playBattleShip(Clients *cli, char *buffer, int playerID) {
    Board myBoard; /*tabuleiro do servidor*/
    Board enemyBoard; /*tabuleiro do inimigo*/
    /*inicia ambos tabuleiros*/
    bootUpBoard(&enemyBoard);
    bootUpBoard(&myBoard);
    /*tabuleiro do servidor sempre será iniciado aleatoriamente*/
    randomPositionShips(&myBoard);

    int datasocket = cli->socket;
    ssize_t len = 1; /*forçar a entrada no loop*/
    int player = 1;                              /* NUMERO DO JOGADOR - 1 ou 2
                                                  * inicia com 1 pois o primeiro jogador sempre será o 1 */
    int winner = 0;                              /* JOGADOR VENCEDOR               */
    int row = 0;                                 /* LINHA DO TABULEIRO             */
    char column = 0;                             /* COLUNA DO TABULEIRO            */
    int n;                                       /* NUMERO DE ARGUMENTOS           */
    char attackCord[STR_LEN];                    /* COORDENADAS DO ATAQUE          */
    memset(buffer, 0x0, BUFFER_LEN);

    printf("\n[LOG]: Iniciando batalha naval com o(a) %s\n", cli->name);

    /*O loop é encerrado caso tive um vencedor, ou caso o cliente fechar conexão, nesse caso len será < 0*/
    while (winner == 0 && len > 0) {
        /*Sai do do while quando n == 2 (entradas válidas) ou len < 0 (alguém desconectou do jogo)*/
        do {
            if (player == playerID) {
                pthread_mutex_lock(&mutex);
                //MUTEX
                row = randomRow();
                column = (char) randomCollum();
                sprintf(attackCord, "%d %c", row, column);
                n = 2; //os ataques do servidor sempre serão válidos, por isso força o n == 2
                //MUTEX
                pthread_mutex_unlock(&mutex);
                /*Já que os servidor ataca muito rápido, da um sleep para dar tempo do cliente processar as informações*/
                sleep(2);
                printf("\n[LOG]:(BATALHA CONTRA %s) servidor atacou em %d-%c\n", cli->name, row, column);
                send(datasocket, attackCord, strlen(attackCord), 0); /*Envia as coordenadas de ataque*/
            } else {
                printf("\n[LOG]:(BATALHA CONTRA %s) esperando o ataque de %s...\n", cli->name, cli->name);
                len = recv(datasocket, attackCord, STR_LEN, 0); /*Recebe as coordenadas de ataque*/
                attackCord[len] = '\0';
                pthread_mutex_lock(&mutex);
                //MUTEX
                if ((strcmp(attackCord, "M") != 0) && (strcmp(attackCord, "m") != 0)) {
                    n = sscanf(attackCord, "%d %c", &row, &column);
                    if (n == 2) {
                        printf("\n[LOG]:(BATALHA CONTRA %s) o(a) jogador(a) %s atacou em %d-%c\n", cli->name, cli->name,
                               row, column);
                    }
                } else {
                    printf("\n[LOG]:(BATALHA CONTRA %s) o(a) %s solicitou o tabuleiro...\n", cli->name, cli->name);
                    send(datasocket, (const void *) &myBoard, sizeof(myBoard), 0);
                }
                //MUTEX
                pthread_mutex_unlock(&mutex);
            }
        } while (n != 2 && len > 0);

        if (player == playerID) {
            /*RECEBE FLAG SE ACERTOU OU ERROU*/
            len = recv(datasocket, buffer, BUFFER_LEN, 0);
            buffer[len] = '\0';
            pthread_mutex_lock(&mutex);
            //MUTEX
            if (strcmp(buffer, "ACERTOU") == 0) {
                myBoard.nHits++;
            }
            printf("\n[LOG]:(BATALHA CONTRA %s) %s o ataque(%d-%c).\n", cli->name, buffer, row, column);
            //MUTEX
            pthread_mutex_unlock(&mutex);
        } else {
            pthread_mutex_lock(&mutex);
            //MUTEX
            /*Verifica se o ataque do cliente foi válido*/
            if (verifyAttack(row, column)) {
                if (shoot(&myBoard, row, (int) column)) {
                    sprintf(buffer, "ACERTOU");
                    enemyBoard.nHits++;
                } else {
                    sprintf(buffer, "ERROU");
                }
                printf("\n[LOG]:(BATALHA CONTRA %s) O(a) jogador(a) %s %s o ataque(%d-%c).\n", cli->name, cli->name,
                       buffer, row, column);
            } else {
                printf("\n[LOG]:(BATALHA CONTRA %s) O(a) jogador(a) %s fez um ataque invalido e perdeu a vez!\n",
                       cli->name, cli->name);
                sprintf(buffer, "ATAQUE INVALIDO, PERDEU A VEZ >:(\n");
            }
            //MUTEX
            pthread_mutex_unlock(&mutex);
            /*ENVIANDO FLAG SE ACERTOU OU ERROU OU FEZ ATAQUE INVALIDO*/
            send(datasocket, buffer, strlen(buffer), 0);
        }
        pthread_mutex_lock(&mutex);
        //MUTEX
        /*Verifica se o jogo acabou em algum dos dois tabuleiros.
         * Caso tenha acabado winner recebe o valor de player, que é a vez de determinado jogador*/
        if (gameOver(myBoard) || gameOver(enemyBoard)) {
            winner = player;
        }
        memset(attackCord, 0x0, STR_LEN);
        memset(buffer, 0x0, BUFFER_LEN);
        n = 0;
        /*Seleciona o novo jogador*/
        player = player == 1 ? 2 : 1;
        //MUTEX
        pthread_mutex_unlock(&mutex);
    }

    /*Caso len > 0, quer dizer que saiu do loop sem erros e tivemos um vencedor
     * caso contrário algum jogador desconectou no meio da partida*/
    if (len > 0) {
        if (winner == playerID) {
            printf("\n[LOG]:(BATALHA CONTRA %s) O servidor venceu a partida!\n", cli->name);
        } else {
            printf("\n[LOG]:(BATALHA CONTRA %s) O(a) %s venceu a partida!\n", cli->name, cli->name);
        }
    } else {
        printf("[LOG]:(BATALHA CONTRA %s) O(a) jogador %s desconectou-se no meio da partida!\n", cli->name, cli->name);
    }
}
