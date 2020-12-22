#include "client_registry.h"
#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "debug.h"

typedef struct client_registry {

    int fileDescriptor[1024];
    unsigned int client;
    sem_t muutteexxForCR;


}CLIENT_REGISTRY;


CLIENT_REGISTRY *creg_init()
{
    CLIENT_REGISTRY *newRegistry = Malloc(sizeof(CLIENT_REGISTRY));
    Sem_init(&((*newRegistry).muutteexxForCR),0,1);

    for (int i=0; i< 1024; i++)
    {
        newRegistry -> fileDescriptor[i] = -1;
        newRegistry -> client = 0;
    }
    return  newRegistry;
}

void creg_fini(CLIENT_REGISTRY *cr){
    free(cr->fileDescriptor);
    free(cr);
}

int  creg_register(CLIENT_REGISTRY *cr, int fd){

    P(&((*cr).muutteexxForCR));
    int returnedValue = -1;

    for (int i=0; i< 1024; i++)
    {
        if(cr->fileDescriptor[i] == -1){
            cr -> fileDescriptor[i] = fd;
            cr -> client++;
            debug("%d", cr -> client);
            returnedValue = 0;
            break;
        }
    }

    V(&((*cr).muutteexxForCR));
    return returnedValue;
}

int  creg_unregister(CLIENT_REGISTRY *cr, int fd)
{
    P(&((*cr).muutteexxForCR));
    int returnedValue = -1;


    for (int i=0; i< 1024; i++)
    {
        if(cr->fileDescriptor[i] == fd){
            cr -> fileDescriptor[i] = -1;
            cr -> client--;
            returnedValue = 0;
            break;
        }
    }

    V(&((*cr).muutteexxForCR));
    return returnedValue;

}

void creg_wait_for_empty(CLIENT_REGISTRY *cr)
{
    P(&((*cr).muutteexxForCR));
    if(!((*cr).client)){}
    V(&((*cr).muutteexxForCR));
}

void creg_shutdown_all(CLIENT_REGISTRY *cr)
{

    for (int i=0; i< 1024; i++)
    {
        if(cr->fileDescriptor[i] != -1){
            shutdown(cr->fileDescriptor[i], SHUT_RD);
            cr -> client--;

        }
    }

}

