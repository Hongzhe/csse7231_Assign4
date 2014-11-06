#include <stdlib.h>
#include "game.h"

/* node in the doubly linked list */
typedef struct Node {
    Game* game;
    struct Node *next;
    struct Node *prev;
} Node;

/* a doubly linked list storing games */
typedef struct WaitList {
    Node* head;
    Node* tail;
    int size;
} WaitList;

WaitList* new_list();
void insert_at_tail(WaitList* list, Game* g);
void remove_a_node(WaitList* list, Node* node);
Node* search(WaitList* list, char* name);
void free_list(WaitList* list);
int print_list(WaitList* list);

