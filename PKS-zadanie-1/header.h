// Created by Susanka on 24/09/2019.


#ifndef PKS_ZADANIE_1_HEADER_H
#define PKS_ZADANIE_1_HEADER_H
#endif //PKS_ZADANIE_1_HEADER_H


#include<stdio.h>
#include <string.h>
#include <stdlib.h>
#include<winsock2.h>
#include <unistd.h>
#include <pthread.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_IP_LENGTH 15

//typy sprav
#define T_TEXT 't'
#define T_FILE 'f'
#define T_INITIAL_MESSAGE 'i'
#define T_RESPONSE 'r'
#define T_KEEP_ALIVE 'k'
#define T_KEEP_RESPONSE 'a'
#define T_EXIT 'e'

int wait;                                   //komunikacia medzi vlaknami. "Dostal som odpoved"
int run_thread;                             //premenna, ktora spusta a zastavuje while cykly vo vsetkych vlaknach
int *broken_packets;                        //pole chybnych packetov
int broken_packets_size;                    //kolko prvkov je v poli chybnych packetov
int max_size_of_packet;                     //velkost fragmentu (meni pouzivatel)
int counter;                                //pocitadlo kolkokrat neprisla odpoved na keep-alive

typedef struct configuration {
    SOCKET socket;
    struct sockaddr_in socket_config;       //ip adresa, port, family, informacia o sockete"
} CONFIGURATION;

typedef struct packet_header {
    char type;
    int fragment_number;
    int crc;
    int data_size;                          //ak je T_RESPONSE, posielam kolko intov mi ide v poli
    unsigned char data[1456];               //1500 - 16 - 28 odcitam hlavicku UDP a IP (28) a moju hlavicku (po zarovnani 16)
} PACKET_HEADER;

typedef struct first_message {
    int total_packets;
    char type;
    char file_name[1464];                   //1500 - 8 - 28 odcitam hlavicku UDP a IP (28) a moju hlavicku (po zarovnani 8)
} FIRST_MESSAGE;

typedef struct message_part {
    PACKET_HEADER *packet;
    struct message_part *next;
} MESSAGE_PART;

//z main.c
void server(CONFIGURATION *config_socket);

void client(CONFIGURATION *config_socket);

//zo sending.c
void *sending(void *vargp);

PACKET_HEADER *create_packet(char type, int fragment_number, int crc, int data_size, unsigned char *data);

int calculate_treshold(int total_size);

void send_broken_packets(MESSAGE_PART *backup_ok_packets, int packet_treshold, CONFIGURATION *configuration);

int contains(int packet_number);

int hash(unsigned char *data, int length);

void send_message(CONFIGURATION *configuration, PACKET_HEADER *packet);

void free_linked_list(MESSAGE_PART **linked_list);

//z receiving.c
void *receiving(void *vargp);

PACKET_HEADER *receive_message(CONFIGURATION *configuration);

//z keepp_alive.c
void *keep_alive(void *vargp);

PACKET_HEADER *create_keep_alive();

//z message_management.c
int get_message(unsigned char **final_message, int message_size, FILE *file, int *loaded_size);

//z global_variables.c
void set_global_variables();