#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "sError.h" 
#include "previous/deck.h"
#include "game.h"
#include "splayer.h"
#include "waitList.h"

#define MAXHOSTNAMELEN 128
#define DEFAULTPLAYERS 30
#define LISTENFAIL -1

/* mutex lock for locking game waiting list.*/
pthread_mutex_t lock;
/* A array store all players' records*/
Players* allPlayers;

void init_mutex();
int open_listen(int port, bool isAdmin);
void accept_connections(int* ports, int num);

/* data need to to start a thread */
typedef struct {
    int portId;
    int port;
    int fd;
    char *fileName;
    decks_t* deckFile;
    WaitList* queue;
} Info;

/**
 * Check the server invocation paramters
 * If the paramters, exit the server and print error message accordingly. 
 * It return nothing.
 */ 
void check_parameter(int argc, char* argv[]) {
    int adminPort, firstPort;
    int i;
    char* err;
    if(argc % 2 != 0) {
        exit_server(USAGE);
    }
    adminPort = (int)strtol(argv[1], &err, 10);
    if(err[0] != '\0') {
        exit_server(BADPORT);
    }
    
    if(adminPort <= 0 || adminPort > 65535) {
        exit_server(BADPORT);
    }

    if(argc > 2) {
        for(i = 2; i < argc; i += 2) {
            char* wrong;
            int port = (int)strtol(argv[i], &wrong, 10);
            if(wrong[0] != '\0' || port == adminPort) {
                exit_server(BADPORT);
            }
            if(port <= 0 || port > 65535) {
                exit_server(BADPORT);
            }
            if(i == 4 && firstPort == port) {
                exit_server(BADPORT);
            }
            if(i == 2) {
                firstPort = port;
            }
        }
    }
}

/**
 * Get the player name and game name from user input.
 * It takes the client's file streame as the only paramteres
 * It returns the names.
 */
char* receive_name(FILE* in) {
    int count = 0;
    int defaultLen = 20;
    char* buffer = malloc(sizeof(char) * defaultLen);
    int c;
    while(1) {
        c = fgetc(in);
        if(c == '\n') {
            break;
        }
        if(count == defaultLen - 1) {
            defaultLen = 2 * defaultLen;
            buffer = realloc(buffer, sizeof(char) * defaultLen);
        }
        buffer[count] = c;
        count++;
    }
    return buffer; 
}

/**
 * Start gthe game when that game is full. And sent game setup Information to
 * each players.
 */
void* start_game(void* g) {
    Game* game = (Game*) g;
    sort_players(game);
    int i, n;
    int complete;
    for(i = 0; i < game->numPlayers; i++) {
        sPlayer* player = game->players[i];
        fprintf(player->to, "%d %c\n", game->numPlayers, 'A' + i);
        fflush(player->to);
        for(n = 0; n < game->numPlayers; n++) {
            fprintf(player->to, "%s\n", game->players[n]->name);
            fflush(player->to);
        }
    }
    sleep(0.5); //give client time to respond
    complete = play_game(game);
    if(complete == 0) {
        pthread_mutex_lock(&lock); 
        store_new_player(allPlayers, game);
        pthread_mutex_unlock(&lock); 
    }
    free_game(game);
    pthread_exit(NULL);
    return NULL;
}

/**
 * Fill splayer property including name, output and 
 * input file stream.
 * This function return nothing.
 */
void fill_splayer(sPlayer* newPlayer, char*pName, FILE* in, FILE* out) {
    newPlayer->name = pName;
    newPlayer->from = in;
    newPlayer->to = out;
    newPlayer->score = 0;
}

/**
 * Once client manage to connect server, server search game they want to join. 
 * If the game doesn't exist, create a new game in the game waiting list. 
 * Once the game is full, remove the game from waiting list and start a thread
 * for game playing.
 * It return nothing.
 */
void* once_connect(void* package) {
    Info* pack = (Info*) package;
    char *name, *pName;
    int clientFd = pack->fd;
    int count = 0;
    bool full = false;
    Game* ready;
    FILE* in = fdopen(clientFd, "r");
    pthread_t id;
    pthread_mutex_lock(&lock);
    while(count < 2) {
        name = receive_name(in);
        if(count == 0) {  //player name
            pName = name;
        } else { // game name
            WaitList* queue = pack->queue;
            int num = parse_game_name(name);
            FILE* out = fdopen(clientFd, "w");
            sPlayer *newPlayer = malloc(sizeof(sPlayer));
            fill_splayer(newPlayer, pName, in, out);
            Node* node = search(queue, name);
            if(node == NULL) {    //create a new game
                Game *newGame = malloc(sizeof(*newGame));
                init_game(newGame, name, num);
                alloc_game(newGame);
                decks_t *deckLists = alloc_decklist();
                newGame->decks = pack->deckFile;
                FILE* deckfile = fopen(pack->fileName, "r");
                read_deck(deckLists, deckfile);
                newGame->decks = deckLists;
                insert_at_tail(queue, newGame);
                add_player(newGame, newPlayer);
            } else { //find a matched game that has room for a new player
                ready = node->game;
                add_player(ready, newPlayer);
                if(ready->currentNum == ready->numPlayers) {
                    remove_a_node(queue, node);
                    full = true;
                }
            }
        }
        count++;
    }
    pthread_mutex_unlock(&lock);
    if(full) {
        pthread_create(&id, NULL, &start_game, (void*)ready);
        pthread_detach(id);
    }
    pthread_exit(NULL);
    return NULL;
}

/**
 * This function accept client connection and start a thread for each client
 * It takes two paramters. The first one is the file decsriptor of the scoket.
 * And struct pack 
 * This function return nothing.
 */
void process_connections(int fdServer, Info* pack) {
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    while(1) {
        fromAddrSize = sizeof(struct sockaddr_in);
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
        pack->fd = fd;
        if(fd < 0) {
            //exit_server(SYSCALL);
            continue;
        }
        pthread_t id;
        /*new thread for new connection */
        if(pthread_create(&id, NULL, once_connect, (void*)pack) < 0) {
            exit_server(SYSCALL);
        }
    }
}

/**
 * The function start to process the client connection on non-admin port.
 */
void* welcome(void* pack) {
    Info* p = (Info*)pack;
    int pnum = p->port;
    int fd = open_listen(pnum, false);
    process_connections(fd, pack);
    pthread_exit(NULL);
    return 0;
}

/**
 * This is help function for qsort.
 */
int cmp(const void*a, const void *b) {
    const Gplayer** p1 = (const Gplayer**)a;
    const Gplayer** p2 = (const Gplayer**)b;
    return strcmp((*p1)->pName, (*p2)->pName);
}

/**
 * sort player based on their name
 * It takes Players as the only parameters
 * Thsi function return nothing.
 */
void sort_allplayers(Players* players) {
    qsort(players->allplayers, players->size, sizeof(Gplayer*), cmp);
}

/**
 * Start game running on admin port.
 * It takes three parameters. The first one is admin port 
 * The second one is file name of deck file.
 * The third is file stream for admin connection.
 * It return -1 if error happenens otherwise return 0
 */
int admin_game(int port, char* deckFile, FILE* out) {
    int newFd = open_listen(port, true); //bind socket to new port
    if(newFd == LISTENFAIL) {
        fprintf(out, "Unable to listen on port\n");
        fflush(out);
        return -1;
    }
    decks_t* decks = alloc_decklist();
    FILE* dfile = fopen(deckFile, "r");
    if(dfile == NULL) {
        fprintf(out, "Unable to access deckfile\n");
        fflush(out);
        return -1;
    }
    if(read_deck(decks, dfile) == -1) {
        //exit_server(BADDECK);
        fprintf(out, "Error reading deck\n");
        fflush(out);
        return -1;
    }

    Info* pack = malloc(sizeof(Info));
    WaitList* gameQueue = new_list(); //each port has their own waiting list
    pack->queue = gameQueue;
    pack->deckFile = decks;
    pack->fileName = deckFile;
    fprintf(out, "OK\n");
    fflush(out);
    process_connections(newFd, pack); //accpet connection

    return 0;
}

/**
 * Send tht table of client records to client.
 * It takes the connection file stream as only paramteer.
 * It return nothing.
 */
void print_admin_state(FILE *out) {
    //fprintf(stderr, "ready to print state\n");
    sort_allplayers(allPlayers);
    Gplayer** players = allPlayers->allplayers;
    for(int i = 0; i < allPlayers->size; i++) {
        fprintf(out, "%s,%d,%d,%d\n", players[i]->pName, players[i]->games, 
                players[i]->rounds, players[i]->winners);
    }
    fprintf(out, "OK\n");
    fflush(out);
}

/**
 * This function parse admin port command and repond command accordingly.
 * The takes two paramters.First one is user input.
 * The second paramter is the file stream that open for incoming connection
 */
int get_admin_cmd(char* command, FILE *out) {
    char mode = command[0];
    char* content = command + 1; 
    int newPort;
    if(mode == 'S' && strlen(content) == 0) {
        print_admin_state(out);
    }
    if(mode == 'P' && strlen(content) > 0) {
        char* tok, *err;
        char* deckFile;
        tok = strtok(content, " ");
        newPort = (int)strtol(tok, &err, 10);
        if((*err != ' ') && (*err != '\0')) {
            return -1;
        }
        tok = strtok(NULL, " ");
        deckFile = tok;
        admin_game(newPort, deckFile, out);
    }
    return 0;
}

/**
 * It accept the admin_port connection and get admin command.
 * It takes the sockets file descriptor as the only parameter.
 * This function return nothing.
 */
void process_admin(int adminFd) {
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    while(1) {
        fromAddrSize = sizeof(struct sockaddr_in);
        fd = accept(adminFd, (struct sockaddr*) &fromAddr, &fromAddrSize);
        if(fd < 0) {
            //perror("Error accepting connection");
            //exit_server(SYSCALL);
        }
        FILE* in = fdopen(fd, "r");
        FILE* out = fdopen(fd, "w");
        int defLen = 10;
        char* command = malloc(sizeof(char) * defLen);
        int count = 0;
        while(1) {
            int c;
            c = fgetc(in);
            if(c == EOF) { //accept new connection
                break;    
            }
            if(count >= defLen) {
                defLen = defLen * 2;
                command = realloc(command, sizeof(char) * defLen);
            }
            command[count] = c;
            if(c == '\n') {
                command[count] = '\0';
                get_admin_cmd(command, out);
                defLen = 10;
                command[0] = '\0';
                count = 0;
                continue;
            } 
            count++;
        }
    }
}

/**
 * admin_port is used start admin port connection process.
 * 
 */
void* admin_port(void* p) {
    int port = *(int*) p;
    int fd = open_listen(port, false); //open socket
    process_admin(fd);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    int i;
    pthread_t adminId;
    check_parameter(argc, argv);
    init_mutex();
    allPlayers = init_players(DEFAULTPLAYERS);
    int adminP = atoi(argv[1]);

    struct sigaction s;
    s.sa_handler = SIG_IGN;
    s.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &s, 0);
    pthread_create(&adminId, NULL, admin_port, &adminP);
    pthread_detach(adminId); 
    if(argc > 2) {
        int num = (argc - 2) / 2;
        pthread_t ids[num];
        for(i = 0; i < num; i++) {
            WaitList* gameQueue = new_list();  
            int index = (i * 2) + 2; //2 4 6 8
            int pnum = atoi(argv[index]);
            decks_t* decks = alloc_decklist();
            FILE* deckfile = fopen(argv[index + 1], "r");
            if(deckfile == NULL) {
                exit_server(DECKFILE);
            }
            if(read_deck(decks, deckfile) == -1) {
                exit_server(BADDECK);
            }
            Info* pack = malloc(sizeof(Info));
            pack->port = pnum;
            pack->deckFile = decks; //decklists
            pack->fileName = malloc(sizeof(char) * strlen(argv[index + 1]));
            strcpy(pack->fileName, argv[index + 1]);
            pack->queue = gameQueue;
            if(pthread_create(&(ids[i]), NULL, welcome, pack) != 0) {
                exit_server(SYSCALL);
            }
        } 
        for(i = 0; i < num; i++) {
            pthread_join(ids[i], NULL);
        }   
    }    
    pthread_mutex_destroy(&lock);
    pthread_exit(NULL);
    
    return 0;
}

/**
 * init mutex lock. 
 * It takes no parameters and return nothing.
 */
void init_mutex(void) {
    if(pthread_mutex_init(&lock, NULL) != 0) {
        exit_server(SYSCALL);
    }
}

/**
 * open a socket and bind that socket to the port
 * Take two parameters. One is the port server will listen on.
 * The other paramters is specify if the socket is open for admin port or not
 * return the socket file descriptor if no error happenens.
 * If isAdmin set to false, exit server accordingly.
 * If isAdmin set to true, return corresponding error code
 */
int open_listen(int port, bool isAdmin) {
    int fd;
    struct sockaddr_in serverAddr;
    int optVal;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        if(isAdmin) {
            return LISTENFAIL;
        } else {
            exit_server(LISTEN);
        }
    }
    optVal = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int)) < 0) {
        if(isAdmin) {
            return LISTENFAIL;
        } else {
            exit_server(LISTEN);
        }
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(fd, (struct sockaddr*)&serverAddr, 
            sizeof(struct sockaddr_in)) < 0) {
        if(isAdmin) {
            return LISTENFAIL;
        } else {
            exit_server(LISTEN);
        }
    }

    if(listen(fd, SOMAXCONN) < 0) {
        if(isAdmin) {
            return LISTENFAIL;
        } else {
            exit_server(LISTEN);
        }
    }
    return fd;
}

