// Soln for csse2310-ass3 By Joel Fenwick

#ifndef DECK_H
#define DECK_H

#include <stdio.h>
#include <stdbool.h>

typedef struct deckstruct {
    char cards[20];
    unsigned size;
    unsigned current;
    char aside;	// card set aside at beginning
    struct deckstruct* next;
} deck_t;

typedef struct {
    deck_t* head;
    deck_t* tail;
    deck_t* current;
} decks_t;

int read_deck(decks_t* decks, FILE* f);
bool load_deck(deck_t* d, FILE* f);
void free_deck(deck_t* d);
char draw_card(deck_t* d);
char get_leftover(deck_t* d);
bool deck_empty(deck_t* d);
void reset_deck(deck_t* d);


decks_t* alloc_decklist();
   // missed opportunity to call this clear_decks
void free_decklist(decks_t* d);
bool add_deck(decks_t* d, FILE* f);
deck_t* get_deck(decks_t* d);

#endif
