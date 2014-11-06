#include <stdio.h>
#include <stdlib.h>
#include "sError.h"

/**
 * Print error message and exit server
 * It takes error code as only paramters
 * This function return nothing
 */
void exit_server(SerExitCode code) {
    switch(code) {
        case GOOD:
            exit(0);
            break;
        case USAGE:
            fprintf(stderr, "Usage: 2310serv adminport [[port deck]...]\n");
            exit(1);
            break;
        case DECKFILE:
            fprintf(stderr, "Unable to access deckfile\n");
            exit(2);
            break;
        case BADDECK:
            fprintf(stderr, "Error reading deck\n");
            exit(3);
            break;
        case BADPORT:
            fprintf(stderr, "Invalid port number\n");
            exit(4);
            break;
        case LISTEN:
            fprintf(stderr, "Unable to listen on port\n");
            exit(5);
            break;
        case SYSCALL:
            fprintf(stderr, "System error\n");
            exit(9); 
    }
}
