#ifndef SPLAYER_H
#define SPLAYER_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    char* name;
    int index;
    pthread_t clientId;
    FILE* to;
    FILE* from;
    char cards[2];
    int socres;
    int winner;  //1 = winner otherwise 0
    bool protected;
    bool active;
    bool disconnect;
    int score;
} sPlayer;

#endif
