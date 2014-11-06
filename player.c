#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "player.h"

/**
 * Create a player.
 * It take four parameters. 
 * The first two are string of player name and game name
 * The last two are file stream opend for read and write
 * This function return pointer to Player
 */
Player* create_player(char* name, char* game, FILE*in, FILE*out) {
    Player *p = malloc(sizeof(Player));
    p->active = true;
    p->protected = false;
    p->onHand[0] = '0';
    p->onHand[1] = '0';
    p->score = 0;
    p->name = name;
    p->read = in;
    p->write = out;
    p->game = game; 
    return p;
}

/**
 * Initaliaze some property of player.
 * It takes pointer to Player as the only paramter.
 * This function return nothing.
 */
void init_player(Player* p) { 
    p->active = true;
    p->protected = false;
    p->onHand[0] = '0';
    p->onHand[1] = '0';
}
