#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "debug.h"
#include "protocol.h"
#include "csapp.h"

int proto_send_packet(int fd, BRS_PACKET_HEADER *hdr, void *payload)
{

    BRS_PACKET_HEADER *incomingHDR = hdr;
    void* incomingPayload = payload;
    int p_size = htons(incomingHDR->size);

    if((rio_writen(fd, incomingHDR, sizeof(BRS_PACKET_HEADER))) < 0)
    {
        errno = EINTR;
        return -1;
    }
    if(p_size > 0)
    {
        if((rio_writen(fd,incomingPayload,p_size)) <= 0)
        {
            errno = EINTR;
            return -1;
        }
    }
    return 0;

}


int proto_recv_packet(int fd, BRS_PACKET_HEADER *hdr, void **payloadp){

    if ((rio_readn(fd, hdr, sizeof(BRS_PACKET_HEADER))) <= 0)
        return -1;

    hdr->size = ntohs(hdr->size);
    hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
    hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

    if ((hdr->size) > 0)
    {
        *payloadp = Malloc(hdr->size);
        if((rio_readn(fd, *payloadp, hdr->size))<=0)
        {
            return -1;
        }
    }
    return 0;
}

