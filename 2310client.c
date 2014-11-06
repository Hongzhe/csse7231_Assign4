#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "clientError.h"
#include "player.h"
#define NOTARGET -1

/* game status */
typedef struct {
    int index;
    int pCount;
    char* name;
    char* gameName;
    char** names;  //players name
    char** played; // cards each player have played
    char* msg; // move ---
    int inplay[8];
    char cards[2]; //cards on hand
    bool* protected;
    bool* active;
    int prompt; //0- card 1-target 2-guess  
    FILE* read;
    FILE* write;
    int scores[4];
} Gamestate;

/* this happened message */
typedef struct {
    int player;
    char pcard;
    int target;
    char guess;
    int dropPlayer;
    char dropCard;
    int outPlayer;
} Happend;

/**
 * This function is used to check is player name is valide.
 * It takes te string of name as paramters
 * If the name is valid then return true. Otherwise return false;
 */
bool check_name(char* name) {
    int i;
    int len = (int)strlen(name);
    if(len == 0) {
        return false;
    }
    for(i = 0; i < len; i++) {
        if(name[i] == '\n') {
            return false;
        }
    }
    return true;
}

/**
 * Check if the paramters is valide or not.
 * If the paramter is invlaid, then print error message to stderr
 * and exit server.
 * This function return nothing.
 */
void check_parameters(int argc, char* argv[]) {
    if(argc > 5 || argc < 4) {
        exit_client(USAGE);
    }
    char* player = argv[1];
    if(!check_name(player)) {
        exit_client(PLAYERNAME);
    }
    if(!check_name(argv[2])) {
        exit_client(GAMENAME);
    }
    int port = atoi(argv[3]);
    if(port <= 0 || port > 65535) {
        exit_client(PORTNO);
    }
}

/**
 * This function convert hostname to ip address
 * This function takes the string of hostname as
 * only paramters.
 * It return pointer to struct in_addr*
 */
struct in_addr* name_to_ip(char* hostname) {
    int error;
    struct addrinfo* addressInfo;

    error = getaddrinfo(hostname, NULL, NULL, &addressInfo);
    if(error) {
        return NULL;
    }

    return &(((struct sockaddr_in*)(addressInfo->ai_addr))->sin_addr);
}

/**
 * Open socket for connecting server.
 * This function return file descriptor of the socket.
 */
int open_socket(struct in_addr* ipAddress, int port) {
    int fd;
    struct sockaddr_in serverAddr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        exit_client(CONNFAIL);
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = ipAddress->s_addr;

    if(connect(fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        exit_client(CONNFAIL);
    }

    return fd; 
}

/**
 * If the game is full, then send game setup info to all players.
 * This function take pointer to Gamestate.
 * This function return nothing.
 */
void join(Gamestate *g) {
    //fprintf(stderr, "pname is %s the gname is %s\n", g->name, g->gameName);
    fprintf(g->write, "%s\n", g->name);
    fflush(g->write);
    fprintf(g->write, "%s\n", g->gameName);
    fflush(g->write);
}

/**
 * Initalize the property of  gamestate.
 * It takes the pointer to Gamestate as only paramters
 * This function return nothing.
 */
void init_game(Gamestate* g) {
    int size = g->pCount;
    g->names = malloc(sizeof(char*) * size);
    g->played = malloc(sizeof(char*) * size);
    g->protected = malloc(sizeof(bool) * size);
    g->active = malloc(sizeof(bool) * size);
    g->msg = malloc(sizeof(char) * 5); 
    for(int i = 0; i < size; i++) {
        g->played[i] = malloc(sizeof(char) * 20);
        g->played[i][0] = '\0';
        g->scores[i] = 0; 
    } 
}

/**
 * Free the memory allocated to a Gamestate.
 * It takes the pointer to Gamestate as the only paramter.
 * This function return nothing.
 */
void clean_game_state(Gamestate *g) {
    free(g->protected);
    free(g->active);
    //free(g->move);
    for(int i = 0; i < g->pCount; i++) {
        free(g->played[i]);
        free(g->names[i]);
    }
    free(g->names);
    free(g->played);
}

/**
 * This function is used to parse the game setup message.
 * If the message is invalid, then return false.
 * Otherwise return true;
 */
bool parse_start_msg(char* msg, Gamestate *g) {
    char letters[4] = {'A', 'B', 'C', 'D'};
    int num = msg[0] - '0';
    if(num != g->pCount) {
        return false;
    }
    
    if(msg[1] != ' ') {
        return false;
    }
    if(msg[2] > letters[num - 1]) {
        return false; 
    }
    if(msg[2] < 65 || msg[2] > 68) {
        return false;
    }
    //fprintf(stderr, "letter is %c\n", msg[4]);
    for(int i = 0; i < (msg[0] - '0'); i++) {
        if(letters[i] == msg[2]) {
            g->index = i;
            //fprintf(stderr, "index is %d\n", g->index);
            break;
        }
    }
    g->pCount = msg[0] - '0';
    init_game(g);
    return true;
}

/**
 * Parse the thishappened message sent from server.
 * If the thishappened message is invalid, then return false.
 * Otherwise, return false;
 */
bool unpack_this_happened(Gamestate* g, const char*buff, Happend* hMsg) {
    char lastplayer = 'A' + g->pCount - 1;
    if(strlen(buff) != 8) {
        return false;
    }
    if(buff[4] != '/') {
        return false;
    }
    int player = buff[0] - 'A';
    if(player < 0 || player > g->pCount) {
        return false;
    }
    char pcard = buff[1];
    if(pcard < '1' || pcard > '8') {
        return false;
    }
    int target = (buff[2] == '-') ? '-' : (buff[2] - 'A');
    char guess = buff[3];
    if((guess != '-') && ((guess < '1') || (guess > '8'))) {
        return false;
    }
    if ((buff[5] != '-') && ((buff[5] < 'A') || (buff[5] > lastplayer))) {
        return false;   // invalid player and not -
    }
    int dropplayer = (buff[5] == '-') ? '-' : (buff[5] - 'A');       
    char dropcard = buff[6];
    if ((dropcard != '-') && ((dropcard < '1') || (dropcard > '8'))) {
        return false;
    }
    if ((buff[7] != '-') && ((buff[7] < 'A') || (buff[7] > lastplayer))) {
        return false;   // invalid player and not -
    }
    int outPlayer = (buff[7] == '-') ? '-' : (buff[7] - 'A');   
    if(outPlayer == g->index) {
        g->cards[0] = '-';
        g->cards[1] = '-';
    } 
    hMsg->player = player;
    hMsg->pcard = pcard;
    hMsg->target = target;
    hMsg->guess = guess;
    hMsg->dropPlayer = dropplayer;
    hMsg->dropCard = dropcard;
    hMsg->outPlayer = outPlayer;
    return true;
}

/*This is mind blowning:*/
typedef void (*procfn)(Gamestate*, const char*);

/**
 *  Print current player status information to stdout.
 *  It takes the pointer to Gamestate as the only pareamter.
 *  This function return nothing.
 */
void status(Gamestate* g) {
    for(int i = 0; i < g->pCount; i++) {
        //fprintf(stderr, "index is %d\n",i);
        char p = ' ';
        if(g->active[i] == false) {
            p = '-';
        } else if (g->protected[i]) {
            p = '*';
        }
        fprintf(stdout, "%c(%s)%c:%s\n", 'A' + i, g->names[i], p, 
                g->played[i]);
        fflush(stdout);
    }
    fprintf(stdout, "You are holding:%c%c\n", g->cards[0], g->cards[1]);
    fflush(stdout);
}

/**
 * Print a prompt on the stdout.
 * It takes the pointer to Gamestate as the only paramters
 * This function return nothing.
 */
void print_prompt(Gamestate* g) {
    const char* display[3] = {"card", "target", "guess"};
    fprintf(stdout, "%s>", display[g->prompt]);
    fflush(stdout);
}

/**
 * Parse the server responding message.
 * It takes the pointer to Gamesatate as only parameters.
 * It message is 'YES' then return true.
 * Otherwise return false.
 */
bool server_respond(Gamestate* g) {
    char buffer[5];
    fgets(buffer, 5, g->read);
    int len = (int)strlen(buffer);
    if(buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    //fprintf(stderr, "server repsond %s\n", buffer);
    if(strcmp(buffer, "YES") == 0) {
        return true;
    }
    if(strcmp(buffer, "NO") != 0) {
//        fprintf(stderr, "wrong respond \n");
        exit_client(BADMESG);
    }
    return false;
}

/**
 * Valide card that player discard.
 * If the discarded card is valid then return true
 * Otherwise return false;
 */
bool valide_discard(Gamestate* g, char* input) {
    char min;
    char card = input[0];
    if(input[1] != '\n') {
        return false;
    }
    if(card != g->cards[0] && card != g->cards[1]) {
        return false;
    }
    min = g->cards[0];
    if(min > g->cards[1]) {
        min = g->cards[1];
    }
    if((min == '5' || min == '6') && (g->cards[0] == '7' 
            || g->cards[1] == '7')) {
        min = '7';
    }
    return true;
}

/**
 * Enable palyer to play card.
 * It takes two paramters.
 * One is the pointer to Gamestate.
 * The other is the string of user input.
 * If player play a invalid card, they have to type again.
 * if card is valid then return true.
 */
bool play_card(Gamestate* g, char* input) {
    bool respond;
    int count = 0;
    while(1) {
        input[0] = '\0';
        print_prompt(g);
        while(1) {
            char ch = fgetc(stdin);
            if(ch == EOF) {
                exit_client(END);
            }
            input[count] = ch;
            if(ch == '\n') {
                break;
            }
            count++;
        }
        if(valide_discard(g, input)) {
            respond = true;
            break;
        }
        count = 0;
    }
    
    return respond;
}

/**
 * Enable player to discard card.
 * It takes two parameters.
 * One is pointer to Gamestate.
 * The second one is char of card.
 * This function return nothing.
 */
void discard_card(Gamestate* g, char card) {
    if(g->cards[0] == card) {
        g->cards[0] = g->cards[1];
    }
    g->cards[1] = '-';
}

/**
 * To find if there is a player that can be targeted.
 * If a target is exist, then return the index of that player.
 * Otherwise return -1;
 */
int has_target(Gamestate*g, bool allowself) {
    for(int i = 0; i < g->pCount - 1; i++) {
        int id = (g->index + i + 1) % g->pCount;
        if(g->active[id] && !g->protected[id]) {
            return id;
        }
    }
    if(allowself) {
        return g->index;
    }
    return -1;
}

/**
 * Enable player to pick target.
 * It takes two parameter.
 * One is that pointer to Gamestate
 * The second one is card that player played.
 * If target is exit,then return the target player symbol.
 */
char process_target(Gamestate* g, char card) {
    char input[3];
    bool myself = false;
    bool over = false;
    if(card == '5') {
        myself = true;
    }
    int target = has_target(g, myself);
    if(target != -1) {
        int count;
        g->prompt = 1;
        while(1) {
            input[0] = '\0';
            count = 0;
            print_prompt(g);
            while(1) {
                char ch;
                over = false;
                ch = fgetc(stdin);
                if(ch == EOF) {
                    exit_client(END);
                }
                input[count] = ch;
                if(ch == '\n') {
                    break;
                }
                if(count == 1) {
                    while((ch = getchar()) != '\n' && ch != EOF) {    
                    }
                    over = true;
                    break;
                }
                count++;
            }
            if(over) {
                continue;
            }
            if((target + 'A') == input[0]) {
                break;
            }
        }
    }
    return input[0];
}

/**
 * Process the guess card.
 * It takes pointer to Gamestate as only paremter.
 * Return the guess card.
 */
char process_guess(Gamestate* g) {
    char input[3];
    bool over = false;
    if(has_target(g, false) != -1) {
        g->prompt = 2;
        int count;
        while(1) {
            print_prompt(g);
            input[0] = '\0';
            count = 0;
            while(1) {
                char ch;
                ch = fgetc(stdin);
                if(ch == EOF) {
                    exit_client(END);
                }
                input[count] = ch;
                over = false;
                if(ch == '\n') {
                    break;
                }
                if(count == 1) {
                    char c;
                    while((c = getchar()) != '\n' &&
                            c != EOF) {
                    }
                    over = true;
                    break;
                }
                count++;
            }
            if(over) {
                continue;
            }
            if(input[0] != '1') {
                break;
            }
        }
    }
    return input[0];
}

/**
 * Update player's move record.
 * It takes three parameters.
 * One is the pointer to Gamestate.
 * The second is player's index
 * THE Third one is the card player played.
 * This function return nothing.
 */
void update_move_record(Gamestate* g, int index, char card) {
    int len = (int)strlen(g->played[index]);
    g->played[index][len] = card;
    g->played[index][len + 1] = '\0';
    g->inplay[card - '1']--;
    if(card == '4') {
        g->protected[index] = true;
    } else {
        g->protected[index] = false;
    }
}

/**
 * Process the yourturn.
 * It takes two parameters.
 * One is the pointer to Gamestate.
 * The second is message from server.
 * This function retrun nothing.
 */
void yourturn(Gamestate* g, const char* buff) {
    char c = buff[0];
    char input[3], card;
    bool isValid;
    char target, guess;
    c = buff[0];
    if ((c == '\0') || (buff[1] != '\0')) {
        exit_client(BADMESG);
    }
    if((c < '1') || (c > '8')) {
        exit_client(BADMESG);
    }
    g->cards[1] = c;
    g->protected[g->index] = false;
    status(g);
    int pt;
    while(1) {
        g->prompt = 0; 
        isValid = play_card(g, input);
        guess = target = '-';
        if(isValid) {
            card = input[0];
            switch(card) {
                case '1':
                    pt = has_target(g, false);
                    if(pt != -1) {
                        target = process_target(g, card);
                        guess = process_guess(g);
                    }
                    break;
                case '3':
                case '5':
                case '6':
                    target = process_target(g, card);
                    break;
                case '8':
                    g->active[g->index] = false;
                    break;
            }
            g->protected[g->index] = (card == '4') ? (true) : (false);
            fprintf(g->write, "%c%c%c\n", card, target, guess);
            fflush(g->write);
            if(server_respond(g)) {
                break;
            }     
        }
    }
    discard_card(g, card);
    update_move_record(g, g->index, card);
}

/**
 * Initalize some data for new round.
 * This function return nothing.
 */
void new_round(Gamestate* g) {
    g->inplay[0] = 5;
    g->inplay[1] = 2;
    g->inplay[2] = 2;
    g->inplay[3] = 2;
    g->inplay[4] = 2;
    g->inplay[5] = 1;
    g->inplay[6] = 1;
    g->inplay[7] = 1;
    for(int i = 0; i < g->pCount; ++i) {
        g->played[i][0] = '\0';
        g->protected[i] = false;
        g->active[i] = true;
    }
    g->cards[0] = '-';
    g->cards[1] = '-';
}

/**
 * Respond newround message from server.
 * It takes two paramets.
 * One is the pointer to Gamestate.
 * The other one is string of message from server.
 * This function return nothing.
 */
void newround(Gamestate* g, const char*buff) {
    char c;
    c = buff[0];
    if ((c == '\0') || (buff[1] != '\0')) { // Message is wrong length
        printf("buff[1] is%c\n buff[0] is%c.\n", buff[1], buff[0]);
        exit_client(BADMESG);
    }
    if ((c >= '1') && (c <= '8')) {
        new_round(g);
        g->cards[0] = c;
    } else {
        //fprintf(stderr, "newround wrong card\n");
        exit_client(BADMESG);
    }
}

/**
 * Print thishappened message to stdout.
 * It takes pointer to Happend as only paramter.
 * This function return nothing.
 */
void print_this_happened(Happend* hMsg) {
    printf("Player %c discarded %c", hMsg->player + 'A', hMsg->pcard);
    if((char)hMsg->target != '-') {
        printf(" aimed at %c", hMsg->target + 'A');
        if(hMsg->guess != '-') {
            printf(" guessing %c", hMsg->guess);
        }
    }
    printf(".");
    if((char)hMsg->dropPlayer != '-') {
        printf(" This forced %c to discard %c.", hMsg->dropPlayer + 'A', 
                hMsg->dropCard);
    }
    if((char)hMsg->outPlayer != '-') {
        printf(" %c was out.", hMsg->outPlayer + 'A');
    }
    printf("\n");
    fflush(stdout);
}

/**
 * Respond to thishappened message.
 * It takes two parameters.
 * One is pointer to Gamestate.
 * The other parameter is string of thishappened message.
 * This function return nothing.
 */
void happened(Gamestate* g, const char* buff) {
    Happend hMsg;
    if(!unpack_this_happened(g, buff, &hMsg)) {
        //fprintf(stderr, "unpack mesg is wrong\n");
        exit_client(BADMESG);
    }
    if(hMsg.player != g->index) {
        update_move_record(g, hMsg.player, hMsg.pcard);
    }
    if(hMsg.dropPlayer != '-') {
        update_move_record(g, hMsg.dropPlayer, hMsg.dropCard);
    }
    if(hMsg.outPlayer != '-') {
        g->active[hMsg.outPlayer] = false;
    }
    print_this_happened(&hMsg);
}

/**
 * Resoond to replace message from server
 * It has two paramters.
 * One is the pointer to Gamestate.
 * The second one is replace message. 
 * This function return nothing.
 */
void replace(Gamestate*g, const char* buff) {
    char c = buff[0];
    c = buff[0];
    if((c < '1') || (c > '8')) {
        //fprintf(stderr, "wrong rplace card %c\n", c);
        exit_client(BADMESG);
    }
    g->cards[0] = c;
}

/**
 * Respond to scores message.
 * It has two parameters.
 * One is the pointer to the Gamtestae.
 * The other one is string of scores message
 * This message return nothing.
 */
void scores(Gamestate*g, const char*buff) {
    char* err;
    const char* b = buff;
    int i;
    for(i = 0; i < g->pCount; i++) {
        long l = strtol(b, &err, 10);
        if((*err != '\0') && (*err != ' ')) {
            exit_client(BADMESG);
        }
        if(l < 0 || l > 4) {
            exit_client(BADMESG);
        }
        g->scores[i] = (int)l;  
        b = err;
    }
    printf("Scores:");
    for(i = 0; i < g->pCount; i++) {
        printf(" %s=%d", g->names[i], g->scores[i]);
    }
    printf("\n");
    fflush(stdout);

}

/**
 * Respond to the gameover message.
 * This function has two parameter.
 * One is the pointer to Gamestate.
 * The second one is string of gameover message.
 */
void gameover(Gamestate*g, const char* buff) {
    if(buff[0] != '\0') {
        //fprintf(stderr, "wrong gameover \n");
        exit_client(BADMESG);
    }
    printf("Game over\n");
    fflush(stdout);
    clean_game_state(g);
    exit_client(GOOD);
}

/**
 * Based on command, invoke correponding function to 
 * handle.
 * If everthin is oK, then return 0;
 */
int get_cmd(Gamestate*g, const char* buffer) {
    const char* commands[] = {"newround ", "yourturn ", "thishappened ",
            "replace ", "scores ", "gameover"};
    procfn functions[] = {newround, yourturn, happened, replace, scores, 
            gameover};
    for(int i = 0; i < 6; i++) {
        int len = strlen(commands[i]);
        if(strncmp(commands[i], buffer, len) == 0) {
            functions[i](g, buffer + len);
            return 0;
        }
    }
    exit_client(BADMESG);
    return 0;
}

/**
 * Get the server message and trigger command handler.
 * It takes the pointer to Gamestate as only parameter.
 * This function return nothing.
 */
void event_handler(Gamestate *g) {
    char buffer[30];
    while(1) {
        if(fgets(buffer, 30, g->read) == NULL) {
            exit_client(LOSS);
            break;
        }
        int len = strlen(buffer);
        if(buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        get_cmd(g, buffer);
    }
}

/**
 * After player join a game, wait for server send 
 * game setup message back
 * It takes the pointer to Gamestate as the only paramters
 * If the game setup message is valid, then return true.
 * Otherwise, it will return false;
 */
bool stand_by(Gamestate* g) {
    char firstMsg[5];
    char *name; 
    bool error;
    int i, num, count;
    int defaultNameSize = 10;
    if(fgets(firstMsg, 5, g->read) == NULL) {
        exit_client(GAMEINFO);
    }
    firstMsg[3] = '\0';
    error = parse_start_msg(firstMsg, g);
    if(!error) {
        exit_client(GAMEINFO);
    }
    num = g->pCount; //numbers of players
    i = count = 0;
    name = malloc(sizeof(char) * defaultNameSize);
    for(i = 0; i < num; i++) {
        char c;
        name = malloc(sizeof(char) * defaultNameSize);
        while(1) {
            c = fgetc(g->read);
            if(c == EOF) {
                exit_client(GAMEINFO);
            }
            if(c == '\n') {
                break;
            }
            if(count == defaultNameSize) {
                defaultNameSize = 2 * defaultNameSize;
                name = realloc(name, sizeof(char) * defaultNameSize);
            }
            name[count] = c;
            count++;
        }
        if(strcmp(name, g->name) == 0 && i != g->index) {
            exit_client(BADMESG);
        }
        name[count] = '\0';
        count = 0;
        g->names[i] = name;
        g->names[i] = malloc(sizeof(char) * (strlen(name) + 1));
        strcpy(g->names[i], name);
    }
    return true;
}

/**
 * Based on program invocation parameters,
 * get the numbers of players the game requires.
 * It takes two parameters.
 * One is first character of gamename.
 * The second is pointer to Gamesate.
 * This function return nothings.
 */
void get_num(char argv, Gamestate* game) {
    switch(argv) {
        case '4':
            game->pCount = 4;
            break;
        case '2':
            game->pCount = 2;
            break;
        case '3':
            game->pCount = 3;
            break;
        default:
            game->pCount = 4;
            break;
    }
}

int main(int argc, char* argv[]) {
    struct in_addr* ipAddress; 
    int fd, port; 
    FILE *in, *out;
    char* host = malloc(sizeof(char) * 10);
    Gamestate game;
    check_parameters(argc, argv); 
    get_num(argv[2][0], &game);
    port = atoi(argv[3]);
    if(argc == 4) {
        host = "localhost";
    } else {
        host = argv[4];
    }
    ipAddress = name_to_ip(host);
    if(!ipAddress) {
        exit_client(CONNFAIL);
    }
    fd = open_socket(ipAddress, port);
    in = fdopen(fd, "r");
    out = fdopen(fd, "w");
    game.read = in;
    game.write = out;
    game.name = argv[1];
    game.gameName = argv[2];
    join(&game); 
    
    if(stand_by(&game)) {
        event_handler(&game);
    }
    return 0;
}
