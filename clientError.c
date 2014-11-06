#include <stdio.h>
#include <stdlib.h>
#include "clientError.h"

/**
 * Based on erro code, print error message
 * and exit client.
 * It takes the exit_code as only parameters
 * This function return nothing
 */
void exit_client(ExitCode code) {
    switch(code) {
        case GOOD:
            exit(0);
        case USAGE:
            fprintf(stderr, "Usage: client name game_name port host\n");
            exit(1);
        case PLAYERNAME:
            fprintf(stderr, "Invalid player name\n");
            exit(2);
        case GAMENAME:
            fprintf(stderr, "Invalid game name\n");
            exit(3);
        case PORTNO:
            fprintf(stderr, "Invalid server port\n");
            exit(4);
        case CONNFAIL:
            fprintf(stderr, "Server connection failed\n");
            exit(5);
        case GAMEINFO:
            fprintf(stderr, "Invalid game information received from server\n");
            exit(6);
        case BADMESG:
            fprintf(stderr, "Bad message from server\n");
            exit(7);
        case LOSS:
            fprintf(stderr, "Unexpected loss of server\n");
            exit(8);
        case END:
            fprintf(stderr, "End of player input\n");
            exit(9);
        case SYSCALL:
            fprintf(stderr, "Unexpected system call failure\n");
            exit(20);
    }
}

