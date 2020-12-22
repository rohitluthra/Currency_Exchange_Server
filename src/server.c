#include "server.h"
#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <unistd.h>
#include <string.h>
#include "protocol.h"
#include <exchange.h>
#include <pthread.h>
#include "trader.h"
#include <inttypes.h>

void *brs_client_service(void *arg)
{
    int alreadyLooggedin = 0;
    int incomingArgument = *(int *)arg;
    Free(arg);
    Pthread_detach(pthread_self());
    TRADER *newTarderObject;
    creg_register(client_registry, incomingArgument);


    BRS_PACKET_HEADER *newbrsph;
    while(1)
    {
        newbrsph  = Malloc(sizeof(BRS_PACKET_HEADER));
        char * temporary = NULL;

        proto_recv_packet(incomingArgument, newbrsph, (void **)&temporary);

        if(newbrsph->type == BRS_LOGIN_PKT && !alreadyLooggedin)
        {
            BRS_STATUS_INFO *newbrsstatusinfo  = Malloc(sizeof(BRS_STATUS_INFO));
            newbrsstatusinfo->balance = 0;

            debug("Login Package Received");
            alreadyLooggedin = 1;
            newTarderObject = trader_login(incomingArgument, temporary);

            if(trader_send_ack(newTarderObject, newbrsstatusinfo) < 0){

            }
            Free(newbrsstatusinfo);

        }
        else if((newbrsph->type == BRS_LOGIN_PKT && alreadyLooggedin))
        {
            if(trader_send_nack(newTarderObject) < 0){
                break;
            }
        }

        else if(alreadyLooggedin)
        {
            if(newbrsph->type == BRS_STATUS_PKT)
            {
                BRS_STATUS_INFO *newbrsstatusinfo  = Malloc(sizeof(BRS_STATUS_INFO));

                debug("Status Package Received");
                exchange_get_status(exchange, newbrsstatusinfo);
                if(trader_send_ack(newTarderObject,newbrsstatusinfo) < 0){
                    //break;
                }
              Free(newbrsstatusinfo);

            }
            else if(newbrsph->type == BRS_DEPOSIT_PKT)
            {
                BRS_STATUS_INFO *newbrsstatusinfo  = Malloc(sizeof(BRS_STATUS_INFO));
                BRS_FUNDS_INFO *fundInfo = Malloc(sizeof(BRS_FUNDS_INFO));

                fundInfo  = (BRS_FUNDS_INFO *)temporary;
                debug("Deposit Package Received %d",ntohl(fundInfo->amount) );
                int num = ntohl(fundInfo->amount);
                if(num>= 0)
                {
                    newbrsstatusinfo->balance = (newbrsstatusinfo->balance) + ntohl(fundInfo->amount);

                    debug("%d", newbrsstatusinfo->balance);
                    trader_increase_balance(newTarderObject, newbrsstatusinfo->balance );
                    exchange_get_status(exchange, newbrsstatusinfo);
                    if(trader_send_ack(newTarderObject,newbrsstatusinfo) < 0){
                        break;
                    }
                    Free(newbrsstatusinfo);
                    Free(fundInfo);
                }
                else
                {
                    trader_send_nack(newTarderObject);
                    Free(newbrsstatusinfo);
                    Free(fundInfo);
                    // break;
                }
            }
            else if(newbrsph->type == BRS_WITHDRAW_PKT)
            {
              BRS_STATUS_INFO *newbrsstatusinfo  = Malloc(sizeof(BRS_STATUS_INFO));

              exchange_get_status(exchange, newbrsstatusinfo);

              BRS_FUNDS_INFO *fundInfo = Malloc(sizeof(BRS_FUNDS_INFO));
              fundInfo  = (BRS_FUNDS_INFO *)temporary;
              debug("%d %d",newbrsstatusinfo->balance, ntohl(fundInfo->amount));
              int num = ntohl(fundInfo->amount);

              if(num >= 0)
              {

                debug("Withdraw Package Received");
                if(trader_decrease_balance(newTarderObject, ntohl(fundInfo->amount))==-1)
                {
                    Free(fundInfo);
                    trader_send_nack(newTarderObject);
                     break;
                }

                if(trader_send_ack(newTarderObject,newbrsstatusinfo) < 0){
                    // break;
                }
            Free(newbrsstatusinfo);
                                Free(fundInfo);


            }
            else
            {
                trader_send_nack(newTarderObject);

            }

        }
        else if(newbrsph->type == BRS_ESCROW_PKT)
        {
           BRS_STATUS_INFO *newbrsstatusinfo  = Malloc(sizeof(BRS_STATUS_INFO));

           BRS_ESCROW_INFO *quantityInfo = Malloc(sizeof(BRS_ESCROW_INFO));
           quantityInfo  = (BRS_ESCROW_INFO *)temporary;
           int num = ntohl(quantityInfo->quantity);
           if(num>=0)
           {
           debug("Escrow Package Received");
           trader_increase_inventory(newTarderObject,ntohl(quantityInfo->quantity));
           exchange_get_status(exchange, newbrsstatusinfo);
           if(trader_send_ack(newTarderObject,newbrsstatusinfo) < 0){
             //break;
        }
                            Free(newbrsstatusinfo);
                            Free(quantityInfo);

    }

    }
    else if(newbrsph->type == BRS_RELEASE_PKT)
    {
       BRS_STATUS_INFO *newbrsstatusinfo  = Malloc(sizeof(BRS_STATUS_INFO));

       exchange_get_status(exchange, newbrsstatusinfo);

       BRS_ESCROW_INFO *quantityInfo = Malloc(sizeof(BRS_ESCROW_INFO));
       quantityInfo  = (BRS_ESCROW_INFO *)temporary;

       debug("Release Package Received");
       if(trader_decrease_inventory(newTarderObject,ntohl(quantityInfo->quantity)) == -1)
       {
        trader_send_nack(newTarderObject);
        break;
    }

    if(trader_send_ack(newTarderObject,newbrsstatusinfo) < 0){
           // break;
    }
    free(quantityInfo);

        free(newbrsstatusinfo);
}
else if(newbrsph->type == BRS_BUY_PKT)
{
   // BRS_STATUS_INFO *newbrsstatusinfo  = Malloc(sizeof(BRS_STATUS_INFO));


    BRS_ORDER_INFO *orderinfo = (BRS_ORDER_INFO *)temporary;

    orderid_t id = exchange_post_buy(exchange, newTarderObject, ntohl(orderinfo->quantity),  ntohl(orderinfo->price));
    debug("Buy Package Received");

    if(id)
    {
        BRS_STATUS_INFO *orderupdate = Malloc(sizeof(BRS_STATUS_INFO ));
       exchange_get_status(exchange, orderupdate);

        orderupdate->orderid = htonl(id);

        if(trader_send_ack(newTarderObject,orderupdate) < 0){
               // break;
        }
        free(orderupdate);
    }
    else
        trader_send_nack(newTarderObject);

    //free(orderinfo);

}
else if(newbrsph->type == BRS_SELL_PKT)
{

   debug("Sell Package Received");

   BRS_ORDER_INFO *orderinfo = Malloc(sizeof(BRS_ORDER_INFO));
   orderinfo = (BRS_ORDER_INFO *)temporary;

   orderid_t id = exchange_post_sell(exchange, newTarderObject, ntohl(orderinfo->quantity),  ntohl(orderinfo->price));

   if(!id)
   {

    BRS_STATUS_INFO *orderupdate = Malloc(sizeof(BRS_STATUS_INFO ));
    exchange_get_status(exchange, orderupdate);
    orderupdate->orderid = htonl(id);
    //q htonl

    if(trader_send_ack(newTarderObject,orderupdate) < 0){
               // break;
    }
    free(orderupdate);
}
else
    trader_send_nack(newTarderObject);

}
else if(newbrsph->type == BRS_CANCEL_PKT)
{
    debug("Cancel Package Received");

//    BRS_CANCEL_INFO *orderinfo = Malloc(sizeof(BRS_CANCEL_INFO));
//    orderinfo = (BRS_CANCEL_INFO *)temporary;

//    BRS_STATUS_INFO *orderupdate = Malloc(sizeof(BRS_STATUS_INFO ));
//    exchange_get_status(exchange, orderupdate);

//    int num = exchange_cancel(exchange, newTarderObject,  ntohl(orderinfo->order), order );

//    if(!num)
//    {
//     trader_increase_balance(newTarderObject, 10000000);
//     if(trader_send_ack(newTarderObject,orderupdate) < 0){
//                // break;
//     }
//     free(orderupdate);
// }
// else
//     trader_send_nack(newTarderObject);


}


free(newbrsph);
}
else
{
trader_send_nack(newTarderObject);
break;
}
}

return (void *)NULL;

}