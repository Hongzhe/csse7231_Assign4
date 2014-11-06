#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "waitList.h"

/**
 * Create a new waiting list. The waiting list is a doubly linked list.
 * It takes no parameters.
 * Return the newly create list.
 */ 
WaitList* new_list(void) {
    WaitList* list = malloc(sizeof(WaitList));
    Node* head = malloc(sizeof(Node));
    Node* tail = malloc(sizeof(Node));
    head->next = tail;
    tail->prev = head;
    head = NULL;
    tail = NULL;
    list->head = head;
    list->tail = tail;
    list->size = 0;
    return list;
}

/**
 * Insert one node at the tail of a doubly linked list. It takes two parameters
 * The first one is the pointer to the wating list.
 * The second one is the pointer to game that you want to insert.
 * This function return nothing.
 */ 
void insert_at_tail(WaitList* list, Game *g) {
    Node* new = malloc(sizeof(Node));
    new->game = g;

    if(list->tail == NULL) { //first node;
        list->head = new;
        list->tail = new;
    } else {
        list->tail->next = new;
        new->prev = list->tail;
        new->next = NULL;
        list->tail = new;
    }
    list->size += 1;
}

/**
 * Remove one node from the doubly linked list.
 * It takes two parameters. The first one is pointer to the waiting list.
 * The second one is pointer to the node that you want to remove.
 * This function return nothing.
 */
void remove_a_node(WaitList* list, Node* node) {
    if(node == list->head && node == list->tail) {
        list->head = NULL;
        list->tail = NULL;
    } else if(node == list->head) {
        list->head = node->next;
        list->head->prev = NULL;
    } else if(node == list->tail) {
        list->tail = node->prev;
        list->tail->next = NULL;
    } else {
        Node *before = node->prev;
        Node *after = node->next;
        before->next = after;
        after->prev = before;
    }
    list->size -= 1;
    //fprintf(stderr, "remove game %s\n", node->game->gameName);
}

/**
 * Search game in the linked list.
 * Otherwise, return Node contains that game. 
 * It takes two parameters. The first one is the pointer to waiting list. 
 * The second parameter is game name server seaerch for.
 * If the game is not in the linked list, return NUll
 */
Node* search(WaitList* list, char* target) {
    Node* temp = list->head;
    Node* result = NULL;
    if(temp == NULL) {  //empty list
        return NULL;
    }

    while(temp != NULL) {
        Game* game = temp->game;
        if(strcmp(game->gameName, target) == 0 && 
                game->numPlayers != game->currentNum) {
            result = temp;
            break;
        }
        temp = temp->next;
    }  
    return result;
}

/**
 * print out game name.
 * For test purpose only.
 */
int print_list(WaitList* list) {
    Node* temp = list->head;
    if(temp == NULL) {
        return 0;
    }
    while(temp != NULL) {
        temp = temp->next;
    }
    return 0;
}

/**
 * Free the waiting list of game
 * It takes a waitlist as the only paramters
 * This function return nothing.
 */
void free_list(WaitList* list) {
    Node* h = list->head;
    while(h != NULL) {
        free(h);
        h = h->next;
    }
}
