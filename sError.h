typedef enum {
    GOOD = 0,
    USAGE = 1,
    DECKFILE = 2,
    BADDECK = 3,
    BADPORT= 4,
    LISTEN = 5,
    SYSCALL = 9
} SerExitCode;

void exit_server(SerExitCode);
