// Soln for csse2310-ass3 By Joel Fenwick

#include <stdlib.h>
#include "deck.h"

int read_deck(decks_t* decks, FILE*f) {
   int nextchar;
   do{
       if(!add_deck(decks, f)) {
           return -1;
       }
       nextchar = fgetc(f);
       if(nextchar != EOF) {
           nextchar = ungetc(nextchar, f);
       }
   } while (nextchar != EOF);

   return 0;
}

deck_t* new_deck(FILE* f) {
    deck_t* d = malloc(sizeof(deck_t));
    d->size = 16;
    d->current = 1;
    for (int i = 0; i < 16; ++i) {
	int x = fgetc(f);
	if (x == EOF) {
	    free(d);
	    return 0;
	}
	if ((x < '1') || (x > '8')) {
	    free(d);
	    return 0;
	}
	d->cards[i] = (char)x;
    }
    int x = fgetc(f);
    if (x != '\n') {
        free(d);
	return 0;
    }
    d->aside = d->cards[0];
    d->next = 0;
    // now check if the cards in the deck are valid
    short want[8] = {5, 2, 2, 2, 2, 1, 1, 1};
    short counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 16; ++i) {
        counts[d->cards[i] - '1']++;
    }
    for (int i = 0; i < 8; ++i) {
        if (counts[i] != want[i]) {
            free(d);
	    return 0;
        }
    }
    return d;
}

void free_deck(deck_t* d) {
    free(d);
}

char draw_card(deck_t* d) {
    if (d->current < d->size) {
	return d->cards[d->current++];
    }
    return '0';
}

void reset_deck(deck_t* d) {
    d->current = 1;
    d->aside = d->cards[0];
}

char get_leftover(deck_t* d) {
    return d->aside;
}

bool deck_empty(deck_t* d) {
    return !((d->current) < (d->size));
}


decks_t* alloc_decklist(void) {
    decks_t* d = malloc(sizeof(decks_t));
    d->head = 0;
    d->tail = 0;
    d->current = 0;
    return d;
}

void free_decklist(decks_t* d) {
    deck_t* temp = d->head;
    deck_t* temp2;
    while (temp != 0) {
	temp2 = temp->next;
	free_deck(temp);
	temp = temp2;
    }
    free(d);
}

bool add_deck(decks_t* d, FILE* f) {
    deck_t* add = new_deck(f);
    if (add == 0) {
	return false;
    }
    if (d->head == 0) {
	d->head = add;
	d->tail = add;
	d->current = add;
    } else {
	d->tail->next = add;
	d->tail = add;
    }
    return true;
}

deck_t* get_deck(decks_t* d) {
    deck_t* t = d->current;
    if (t == 0) {	// you managed to call this on an empty list
	return 0;
    }
    d->current = t->next;
    if (d->current == 0) {
	d->current = d->head;
    }
    return t;
}

