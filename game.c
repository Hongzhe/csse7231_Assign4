#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "splayer.h"
#include "previous/cards.h"

#define NOTARGET '-'
#define NOCARD '-'

void play_round(Game* g);

/**
 * create and initalize a new array of player's statistics. 
 * It takes a int as the only paramteres.
 * It return pointer to Players.
 */
Players* init_players(int capacity) {
    Players * players = malloc(sizeof(Players));
    players->allplayers = malloc(sizeof(Gplayer*) * capacity);
    players->capacity = capacity;
    for(int i = 0; i < capacity; i++) {
        players->allplayers[i] = malloc(sizeof(Gplayer));
        players->allplayers[i]->pName = malloc(sizeof(char) * 5);
        players->allplayers[i]->pName[0] = '\0';
        players->allplayers[i]->games = 0;
        players->allplayers[i]->rounds = 0;
        players->allplayers[i]->winners = 0;
    }
    players->size = 0;
    return players;
}

/**
 * This function free memory allocated to all players.
 * It takes one pointer to Players as the only parameter.
 * This function return nothing.
 */
void free_allplayers(Players* players) {
    int count;
    if(players->size < players->capacity) {
        count = players->capacity;
    } else {
        count = players->size;
    }
    for(int i = 0; i < count; i++) {
        free(players->allplayers[i]);
    }
    free(players);
}

/**
 * Update the player's statistics. 
 * It takes two parameters. One is the pointer to Gaplyer.
 * The other is pointer to sPlayer.
 * This function return nothing.
 */
void update_players_record(Gplayer* p, sPlayer* player) {
    p->games += 1;
    p->rounds += player->score;
    p->winners += player->winner;
}

/**
 * Search player in the array of player's statistic.
 * It takes two paramters. One is pointer to sPlayer. The other is
 * pointer to Players.
 * if the player can be founded, then return its index. Otherwise return -1.
 */
int search_player(sPlayer* p, Players* players) {
    int index;
    index = -1;
    for(int i = 0; i < players->size; i++) {
        if(strcmp(players->allplayers[i]->pName, p->name) == 0) {
            index = i;
            break;
        }
    }
    return index;
}

/**
 * Store a new player to the array of player's statisitic.
 * It takes two paramters. One is pointer to Players. The otther is 
 * pointer to Game.
 * This function return nothing.
 */
void store_new_player(Players* players, Game* g) {
    int index;
    int gap = 0;
    if((players->size + 1) == players->capacity) {
        players->capacity = players->capacity * 2;
        gap = players->capacity - players->size;
        players->allplayers = realloc(players, 
                sizeof(Gplayer*) * players->capacity);
        for(int t = 0; t < gap; t++) {
            int m = players->size + t;
            players->allplayers[m] = malloc(sizeof(Gplayer));
            players->allplayers[m]->pName = malloc(sizeof(char) * 2);
            players->allplayers[m]->pName[0] = '\0';
            players->allplayers[m]->games = 0;
            players->allplayers[m]->rounds = 0;
            players->allplayers[m]->winners = 0;
        }
    } 
    
    for(int i = 0; i < g->numPlayers; i++) {
        //Gplayer *gplayer = malloc(sizeof(Gplayer));
        index = search_player(g->players[i], players);
        int id = (index == -1) ? players->size : (index);
        Gplayer* gplayer = players->allplayers[id];
        sPlayer* sp = g->players[i];
        if(index != -1) {
            update_players_record(gplayer, sp);
        } else {
            gplayer->pName = malloc(sizeof(char) * (strlen(sp->name)));
            strcpy(gplayer->pName, sp->name);
            players->size++;
            update_players_record(gplayer, sp);
        }
    }
}

/**
 * Initalize the few properity of a game.
 * It takes two parameters. One is pointer to a Gamem. THe other is a 
 * string of game name. The third is a int number
 * This function return nothing. 
 */
void init_game(Game* g, char* name, int num) {
    g->currentNum = 0;
    g->gameName = malloc(sizeof(char) * strlen(name));
    strcpy(g->gameName, name);
    //g->gameName = name;
    g->numPlayers = num;
    g->cplayer = 0;
    g->out = 0;
}

/**
 * Allocate memory to a game.
 * It takes the pointer to Game as the only parameter.
 * This function return nothing.
 */
void alloc_game(Game* g) {
    int num = g->numPlayers;
    g->players = malloc(sizeof(sPlayer) * num);
    for(int i = 0; i < num; i++) {
        g->players[i] = malloc(sizeof(sPlayer));
        g->players[i]->winner = 0;
    }
}

/**
 * free memeory asslocated to a game.
 * It takes the pointer to Game as only parameters
 * It returns nothing. 
 */
void free_game(Game* g) {
    int i;
    free(g->gameName);
    //free(g->players);
    for(i = 0; i < g->numPlayers; i++) {
        fclose(g->players[i]->to);
        fclose(g->players[i]->from);
        free(g->players[i]);
    }
    free(g->players);
    free_decklist(g->decks);
}

/**
 * This is comparsion function for qsort.
 */
int cmpfunc(const void *a, const void *b) {
    const sPlayer** sa = (const sPlayer**) a;
    const sPlayer** sb = (const sPlayer**) b;
    return strcmp((*sa)->name, (*sb)->name);
}

/**
 * Sort players based on the player name.
 * It takes the pointer to Game as the only parameter.
 * This function return nothing.
 */
void sort_players(Game* g) {
    qsort(g->players, g->numPlayers, sizeof(sPlayer*), cmpfunc);
    /*set player id after sort*/
    for(int i = 0; i < g->numPlayers; i++) {
        g->players[i]->index = i;
    }
}

/**
 * add new players to a game.
 * It takes two parmaters. One is pointer to Game the other is 
 * pointer to sPlay.
 * This function return nothing.
 */
void add_player(Game* g, sPlayer* p) {
    g->players[g->currentNum] = p;
    g->currentNum++;
}

/**
 * This function is used to parse gamename to get
 * the number of players that game require.
 * It takes the gamename as the only paramters.
 * It return the numbers of players. 
 */
int parse_game_name(char* gameName) {
    char c = gameName[0];
    int num;
    switch(c) {
        case '2':
            num = 2;
            break;
        case '3':
            num = 3;
            break;
        default:
            num = 4;
            break;
    }
    return num;
}

/**
 * This function is used to repond players' move.
 * If player's move is Ok, then send 'YES' back to player otherwise send
 * 'NO'.
 * This function return nothing.
 */
void respond_move(Game* g, bool isReject) {
    sPlayer* player = g->players[g->cplayer];
    if(isReject) {
        fprintf(player->to, "YES\n");
        fflush(player->to);
    } else {
        fprintf(player->to, "NO\n");
        fflush(player->to);
    }
}

/**
 * The function is used to handled siutation that the game is over early.
 * It takes two pointers. One is the ponter to Game The other is 
 * index of disconnected player.
 */
void over_early(Game*g, int disconnect) {
    for(int i = 0; i < g->numPlayers; i++) {
        if(disconnect == i) {
            continue;
        }
        fprintf(g->players[i]->to, "gameover\n");
        fflush(g->players[i]->to);
    }
}

/* The blow code are pretty much the same as Joel's solution, 
 * I changed little things*/

/**
 * Send the gameover message to all players.
 * It takes pointer to Game as the only paramters.
 * It return nothing.
 */
void announce_gameover(Game* g) {
    for(int i = 0; i < g->numPlayers; i++) {
        fprintf(g->players[i]->to, "gameover\n");
        fflush(g->players[i]->to);
    }
}

/**
 * Reset soem properity of gaem when each new round start.
 * It takes the pointer to Game as the only paramters
 * This function return nothing.
 */
void round_reset(Game *g) {
    for(int i = 0; i < g->numPlayers; i++) {
        g->players[i]->active = true;
        g->players[i]->protected = false;
    }
    g->cplayer = 0;
    g->activecount = g->numPlayers;
    g->deck = get_deck(g->decks);
    reset_deck(g->deck);
}

/**
 * distibute card at beginning of new round
 * It takes a pointer to Game as the only paramters.
 * This function return nothing.
 */
void distribute_card(Game* g) {
    for(int i = 0; i < g->numPlayers; i++) {
        char c = draw_card(g->deck);
        g->players[i]->cards[0] = c;
        fprintf(g->players[i]->to, "newround %c\n", c);
        fflush(g->players[i]->to);
    }
}

/**
 * This function enable game playing. 
 * It takes pointer to Game.
 * If program running without errro. It return 0;
 */
int play_game(Game* g) {
    bool playing = true;
    do {
        play_round(g);
        // now check for a winner
        for (int i = 0; i < g->numPlayers; ++i) {
            if (g->players[i]->score >= 4) {
                g->players[i]->winner = 1;
                announce_gameover(g);
                playing = false;
                return 0;
            }
        }
    } while (playing);
    return 0;
}

/**
 * Switch the current player to the next available one.
 * It takes the pointer to Game as the only paramters
 * It return nothing.
 */
void next_player(Game* g) { 
    do {
        g->cplayer = (g->cplayer + 1) % (g->numPlayers);
    } while(!g->players[g->cplayer]->active);
}

/**
 * send card to each players.
 * It takes three paramteres. The firet is poiner to Gaem,
 * The second is index of current player and card server send to
 * This function return nothing.
 */
void give_card(Game* g, int index, char card) {
    //fprintf(stderr, "the card is %c\n", card);
    g->players[index]->cards[1] = card;
    fprintf(g->players[g->cplayer]->to, "yourturn %c\n", card);
    fflush(g->players[g->cplayer]->to);
}

/**
 * To find out if player can target itself.
 * It takes a char as the only parameter.
 * If card is '5' then return true, otherwise return false;
 */
bool self_target(char c) {
    return c == C5;
}

/**
 * Validate if player choose the right target. 
 * It takes three parameters. The first one is the pointer to Game. 
 * The second one is a int to indicate index of player and card player played.
 * If the there is valide target return true, otherwise return false; 
 */
bool valid_target(Game* g, int id, char card) {
    if (id == NOTARGET) {  // only allowed if there are no valid targets
        if (self_target(card)) {
            return false;   // if you can target self, you must
        }
    // we know the current player can't be protected
        int count = 0;
        for (int i = 0; i < g->numPlayers; ++i) {
            if ((g->players[i]->active) && (!g->players[i]->protected)) {
                count++;
            }
        }
        if (count > 1) {
            return false;    // there was a target available try again
        }
        return true;
    }
    if (id == g->cplayer) {
        return self_target(card);
    }
    // all that remains is to see if they are protected
    return g->players[id]->active && (!g->players[id]->protected);
}

/**
 * Once a player discard 8, eliminate that players.
 * It takes two parameters. One is the pointer to Game, the other is a int 
 * that indicate index of that player.
 */
void eliminate(Game* g, int id) {
    g->players[id]->active = false;
    g->out = id;
    g->activecount--;
    g->dropcard = g->players[id]->cards[0];
    g->dropplayer = (char)('A' + id);
}

/**
 * This function is used to enable drop and draw. 
 * The two parameters. One is a pointer to Game.
 * The scond paramteres is the target user.
 * This function return nothing.
 */
void drop_and_draw(Game* g, int target) {
    for(int i = 0; i < g->numPlayers; i++) {
        if (i != g->cplayer) {
        }
    }

    if (g->players[target]->cards[0] == C8) {
        eliminate(g, target);
    } else {
    // so we weren't holding C8
        char oldcard = g->players[target]->cards[0];
        char newcard;
        g->dropplayer = (char)('A' + target);
        g->dropcard = oldcard;
        if (!deck_empty(g->deck)) {
            newcard = draw_card(g->deck);
        } else {
            newcard = g->deck->aside; 
        }
        g->players[target]->cards[0] = newcard;
        fprintf(g->players[target]->to, "replace %c\n", newcard);
        fflush(g->players[target]->to);
    }
}

/**
 * This function is used to enable swap cards.
 * It takes three parameter. The Game, index of player who are playing 
 * and target players.
 * This function return nothing.
 */
void swap(Game* g, int id, int target) {
    char c1 = g->players[id]->cards[0];
    char c2 = g->players[target]->cards[0];
    fprintf(g->players[id]->to, "replace %c\n", c2);
    fflush(g->players[id]->to);
    fprintf(g->players[target]->to, "replace %c\n", c1);
    fflush(g->players[target]->to);
    g->players[id]->cards[0] = c2;
    g->players[target]->cards[0] = c1;
}

/**
 * This function send thishappened message to all players. 
 * This function takes five parameters. 
 * It takes five parameters. The first one is Game .
 * The second one is a string store card, a int
 * store target player and a string store teh guess card.
 * This function return nothing. 
 */
void print_this_happened(Game* g, char card, int target, char guess) {
    char targchar = (target == NOTARGET) ? '-' : (char)('A' + target);
    char outchar = (g->out == '-') ? '-' : (char)('A' + g->out);
    for (int i = 0; i < g->numPlayers; ++i) {
        fprintf(g->players[i]->to, "thishappened %c%c%c%c/%c%c%c\n", 
                (char)(g->cplayer + 'A'), card, 
                targchar, guess, g->dropplayer, g->dropcard, outchar);
        fflush(g->players[i]->to);
    }
}

/**
 * Based on player move, repond accordingly.
 * This function takes five parameters. 
 * It takes five parameters. The first one is Game , the second one is a int 
 * indicating whihc player is playing. The third one is a string store card.
 * a int store target player and a string store teh guess card.
 * This function return nothing.
 */
void process_move(Game* g, int id, char card, int target, char guess) {
    g->out = NOTARGET;
    g->dropcard = '-';
    g->dropplayer = '-'; 
    if (g->players[id]->cards[0] == card) {
        g->players[id]->cards[0] = g->players[id]->cards[1];
        g->players[id]->cards[1] = NOCARD;
    } else {
        g->players[id]->cards[1] = NOCARD;
    }
    switch (card) {
        case C1:
            if (target == NOTARGET) {
                break;
            }
            if (g->players[target]->cards[0] == guess) {
                eliminate(g, target);
            }
            break;
        case C3:   
            if (target == NOTARGET) {
                break;
            }
            if(g->players[id]->cards[0] > g->players[target]->cards[0]) {
                eliminate(g, target);
            } else if (g->players[id]->cards[0] < 
                    g->players[target]->cards[0]) {
                eliminate(g, id);
            }
            break;
        case C4:
            g->players[id]->protected = true;
            break;
        case C5:
            drop_and_draw(g, target);
            break;
        case C6:
            if (target == NOTARGET) {
                break;
            }
            swap(g, id, target);
            break;
        case C7:
            break;
        case C8:
            eliminate(g, id);
            break;          
    }
    print_this_happened(g, card, target, guess);
}

/**
 * Validate the card that player is attempt to play.
 * It return false if the player move is invalid.
 * Oterwise return false;
 */
bool valid(Game* g, int index, char card, int target, char guess) {
    sPlayer* player = g->players[index];
    int cards[2];
    for(int i = 0; i < 2; i++) {
        cards[i] = player->cards[i];
    }
    //fprintf(stderr, "onhand cards are %c %d %d\n",card, cards[0], cards[1]);
    if((card != cards[0]) && (card != cards[1])) {
        //fprintf(stderr, "no on hand\n");
        return false;   
    }
    if(((card == '5') || (card == '6')) && 
            ((cards[0] == '7') || (cards[1] == '7'))) {
        //fprintf(stderr, "shold discard 7\n");
        return false;
    }

    return true;
}

/**
 * This function parse the message sent from players. 
 * Get the card player played, target player and guess card.
 * It takes five parameters. The first one is Game , the second one is a int 
 * indicating whihc player is playing. The third one is a string store card, 
 * a int store target player and a string store teh guess card.
 * It return false if the message is invalid. Otherwise return true.
 */
bool get_move(Game* g, int index, char* card, int* target, char* guess) {
    char buffer[20];
    char en;
    sPlayer* player = g->players[index];
    player->protected = false;
    if(fgets(buffer, 19, player->from) == NULL) {
        player->disconnect = true;
        over_early(g, index);        
        /*gaem over*/
    }
    char tchar;
    if((sscanf(buffer, "%c %c %c%c", card, &tchar, guess, &en)) != 4 
            || (en != '\n')) {
        return false;
    }
    if((*guess != '-') && ((*guess < '2') || (*guess > '8'))) {
        return false;
    }
    if((tchar != '-') && ((tchar < 'A') || 
            (tchar > 'A' + g->numPlayers - 1))) {
        return false;
    }
    *target = (tchar == '-') ? '-' : (tchar - 'A');
    if(!valid(g, index, *card, *target, *guess)) {
        return false;
    }
    return true;
}

/**
 * Send scores to each players.
 * It takes Game as the only parameter.
 * This function return nothing.
 */
void send_scores(Game *g) {
    for (int i = 0; i < g->numPlayers; ++i) {
        fprintf(g->players[i]->to, "scores");
        for (int j = 0; j < g->numPlayers; ++j) {
            fprintf(g->players[i]->to, " %d", g->players[j]->score);      
        }
        fprintf(g->players[i]->to, "\n");
        fflush(g->players[i]->to);
    }
}

/**
 * Play one round of game
 * It takes the pointer to Game as the only parameter
 * This function return nothing.
 */
void play_round(Game* g) {
    round_reset(g);
    distribute_card(g);
    while((g->activecount > 1) && (!deck_empty(g->deck))) {
        char card;
        int target, index;
        char guess;
        bool isReject;
        index = g->cplayer;
        give_card(g, index, draw_card(g->deck));
        do {
            isReject = get_move(g, index, &card, &target, &guess); 
            respond_move(g, isReject);
        } while(!isReject);
        process_move(g, index, card, target, guess);
        next_player(g);
    }
    char maxcard = '1';
    for (int i = 0; i < g->numPlayers; ++i) {
        if ((g->players[i]->active) && (g->players[i]->cards[0] > maxcard)) {
            maxcard = g->players[i]->cards[0];
        }      
    }
    for(int i = 0; i < g->numPlayers; i++) {
        if((g->players[i]->active) && (g->players[i]->cards[0] == maxcard)) {
            g->players[i]->score += 1;
        } 
    }
    send_scores(g);
}
