
#ifndef GAME_H
#define GAME_H

#include <pthread.h>
#include <stdlib.h>
#include "previous/deck.h"
#include "splayer.h"

/* Game state */
typedef struct {
    char* gameName;
    int numPlayers;
    int currentNum;
    int port;
    bool disconnect;  //indicate if one player is disconnected
    int cplayer;
    int activecount;
    int out;
    char dropplayer;
    char dropcard;
    pthread_t gameId;
    sPlayer** players;
    decks_t* decks;
    deck_t* deck;
} Game;
/* gaem player statistics */
typedef struct {
    char* pName;
    int games;
    int rounds;
    int winners;
} Gplayer;

/* container of each player statisitics*/
typedef struct {
    Gplayer** allplayers;
    int size;
    int capacity;
} Players;

int play_game(Game* g);
void init_game(Game*g, char*name, int num);
void alloc_game(Game* g);
void free_game(Game* g);
int cmpfunc(const void* a, const void* b);
void sort_players(Game* g);
void add_player(Game*g, sPlayer* p);
int parse_game_name(char* gameName);
void store_new_player(Players* players, Game* g);
Players* init_players(int num);
#endif
