#include "server.h"
#include "protocol.h"
#include "helpers.h"
#include <pthread.h>
#include <signal.h>




int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
            case 'h':
                fprintf(stderr, USAGE_MSG);
                exit(EXIT_FAILURE);
        }
    }

    // 3 positional arguments necessary
    if (argc != 4) {
        fprintf(stderr, USAGE_MSG);
        exit(EXIT_FAILURE);
    }
    unsigned int port_number = atoi(argv[1]);
    char * poll_filename = argv[2];
    char * log_filename = argv[3];


    // Creating the listening Socket for the Server
    open_listenfd(port_number);

    // Initializing the Poll statistics struct 
    stats_t serverStats;
    serverStats.clientCnt = 0;
    serverStats.threadCnt = 0;
    serverStats.totalVotes = 0;
    serverStatsPtr =&serverStats;
    Sem_init(&statsMutex,0, 1);

    // reading in the Polls data
    readInPollInfo(poll_filename);



    

    //INSERT CODE HERE

    return 0;
}
