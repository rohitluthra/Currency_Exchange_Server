    #include <stdlib.h>
    #include <string.h>
    #include "client_registry.h"
    #include "exchange.h"
    #include "trader.h"
    #include "debug.h"
    #include "server.h"
    #include "csapp.h"


extern EXCHANGE *exchange;
extern CLIENT_REGISTRY *client_registry;

static void terminate(int status);

void handler(int signal){
   if(signal == SIGHUP)
   {
    terminate(0);
}
}


int main(int argc, char* argv[]){

    client_registry = creg_init();
    exchange = exchange_init();
    trader_init();

    unsigned int portNumber = 0;

    int num =1;
    if(argc<3)
    {
        exit(0);
    }
    if(argc>5)
    {
        exit(0);
    }


    if(!strcmp(argv[num], "-p"))
    {
        if(num+1 < argc)
        {
            portNumber =(unsigned int) atoi(argv[num+1]);
        }
        else
        {
            exit(0);
        }
    }

    struct sigaction act = {0};
    act.sa_handler = &handler;
    sigfillset(&act.sa_mask);
    if (sigaction (SIGHUP, &act, NULL) < 0)
    {
     exit (-1);
     }

    int listenfd, *connfdp;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    listenfd = Open_listenfd(portNumber);
    while (1)
    {
        socklen_t clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        pthread_create(&tid, NULL, brs_client_service, connfdp);
    }
    terminate(EXIT_FAILURE);
}


    /*
     * Function called to cleanly shut down the server.
     */
static void terminate(int status) {
        // Shutdown all client connections.
        // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

        // Finalize modules.
    creg_fini(client_registry);
    exchange_fini(exchange);
    trader_fini();

    debug("Bourse server terminating");
    exit(status);
}



