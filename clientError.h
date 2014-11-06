
typedef enum {
    GOOD = 0,
    USAGE = 1,
    PLAYERNAME = 2,
    GAMENAME = 3,
    PORTNO = 4,
    CONNFAIL = 5,
    GAMEINFO = 6,
    BADMESG = 7,
    LOSS = 8,
    END = 9,
    SYSCALL = 20
} ExitCode; 

void exit_client(ExitCode);
