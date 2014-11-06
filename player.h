#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

/* game players */
typedef struct {
    char* name;
    char* game;
    char onHand[2];
    FILE* read;
    FILE* write;
    bool active;
    bool protected;
    int score;
    char letter;
} Player;

Player* create_player(char* name, char* game, FILE*in, FILE*out);
void init_player(Player* p);
