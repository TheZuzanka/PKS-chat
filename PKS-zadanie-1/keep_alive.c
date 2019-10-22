// Created by Susanka on 15/10/2019.

#include "header.h"

void *keep_alive(void *vargp) {
    CONFIGURATION *socket_with_config = (CONFIGURATION *) vargp;
    PACKET_HEADER* keep_alive_message = create_keep_alive();

    while(run_thread){
        send_message(socket_with_config, keep_alive_message);
        counter++;

        if(counter == 4){
            printf("Spojenie prerusene.. Zastavujem.. \n");
            exit(100);
        }

        sleep(10);
    }

    return NULL;
}

PACKET_HEADER* create_keep_alive(){
    return create_packet(T_KEEP_ALIVE, 1, 0, 0, NULL);
}