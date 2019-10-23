// Created by Susanka on 15/10/2019.

#include "header.h"

#define PRIME 51

void *sending(void *vargp) {
    CONFIGURATION *socket_with_config = (CONFIGURATION *) vargp;
    unsigned char *message;
    int ask_again;
    FILE *file_to_read;

    while (run_thread) {
        unsigned char *type = (unsigned char*)malloc(sizeof(unsigned char));
        FIRST_MESSAGE *first_packet = (FIRST_MESSAGE *) malloc(sizeof(FIRST_MESSAGE));
        int messageSize = 0;

        ask_again = 1;
        while (ask_again) {
            printf("Chcem poslat: text (t) alebo subor (f) alebo ukoncit peogram (e)\n");
            //scanf("%c", &type);

            get_message(&type, 2, stdin, &messageSize);
            fflush(stdin);
            switch (*type) {
                case T_TEXT:
                    printf("Posielam spravu\n");
                    ask_again = 0;
                    file_to_read = stdin;
                    //first_packet->file_name = NULL;
                    first_packet->file_name[0] = '\0';
                    break;
                case T_FILE:
                    ask_again = 0;
                    unsigned char *name_of_file;
                    printf("Zadaj nazov suboru:\n");
                    get_message(&name_of_file, 1448, stdin, &messageSize);
                    memcpy(first_packet->file_name, name_of_file, 1448 * sizeof(char));
                    for (int i = 0; i < 1448; i++) {
                        if (first_packet->file_name[i] == '\n') {
                            first_packet->file_name[i] = '\0';
                            break;
                        }
                    }

                    if ((file_to_read = fopen(first_packet->file_name, "rb")) == NULL) {
                        ask_again = 1;
                        printf("Subor v adresari neexistuje\n");
                    }
                    free(name_of_file);
                    fflush(stdin);
                    break;
                case T_EXIT:
                    printf("Ukoncujem program..");
                    exit(27);
                default:
                    printf("Prikaz nerozpoznany\n");
                    break;
            }
        }

        //TODO max size musi byt najviac 1500- sizeof header
        int total_size = 0;
        int loaded_message;
        MESSAGE_PART *linked_list = malloc(sizeof(MESSAGE_PART));
        MESSAGE_PART *header = linked_list;
        //MAGIC
        do {
            loaded_message = get_message(&message, max_size_of_packet, file_to_read, &messageSize);
            if (messageSize > 0) {
                int crc = hash(message, messageSize);
                linked_list->packet = create_packet(*type, total_size, crc, messageSize, message);
                free(message);
                linked_list->next = malloc(sizeof(MESSAGE_PART));

                if (loaded_message == 0) {
                    linked_list->next = NULL;
                } else {
                    linked_list = linked_list->next;
                }
                total_size++;
            }
        } while (loaded_message != 0);
        printf("Total packets: %d\n", total_size);
//        linked_list->next = NULL;                   //posledny packet nema nasledovnika

        //fragment c2 bude chybny
        printf("chces poslat chybny fragment? n(nie)/a(ano)\n");
        int broken;
        unsigned char *broken_response = malloc(sizeof(unsigned char));
        get_message(&broken_response, 2, stdin, &messageSize);
        if (*broken_response == 'a') {
            printf("Poskodzujem fragment\n");
            header->packet->crc = -1;
        } else {
            printf("Posielam spravu v poriadku\n");
        }


        //ahoj idem nieco posielat
        first_packet->type = *type;
        first_packet->total_packets = total_size;

        //Idem spravit realne posielanie
        PACKET_HEADER *header_first = create_packet(T_INITIAL_MESSAGE, 0, 0, sizeof(FIRST_MESSAGE), (
                unsigned char *) first_packet);

        printf("Posielam prvy packet\n");

        send_message(socket_with_config, header_first);
        wait = 1;
        while (wait == 1) {
            //cakam kym pride odpoved do druheho vlakna kde sa wait nastavi na 1 a cyklus konci
        }

        //skonci while viem, ze mozem posielat
        //wait  = counter packetov
        MESSAGE_PART *aktual = header;
        MESSAGE_PART *backup_ok_packets = header;
        int packet_treshold = calculate_treshold(
                total_size);          //kolko mi treba poslat paketov aby som cakala na poslanie dalsich
        while (aktual != NULL) {
            total_size--;                                              //kolko nam ich este ostava
            wait++;
            wait %= (packet_treshold +
                     1);                             //aby mi wait nestuplo nad 19, usetrim si nulovanie
            send_message(socket_with_config, aktual->packet);
            //printf("Poslal som packet #%d\ntreshold je: %d\n", aktual->packet->fragment_number, packet_treshold);
            while (wait == packet_treshold) {
                //zasa cakam na odpoved z druheho vlakna
            }
            if (wait == 0) {
                if (broken_packets == NULL) {
                    //vsetky packety su ok
                    backup_ok_packets = aktual;
                    if (aktual->next != NULL) {
                        if (total_size == 0) {
                            //Hack na subory, lebo sa pri citani vytvori jeden fragment naviac ktory je null
                            break;
                        }
                    }
                } else {
                    printf("Broken packets nie je null, treba poslat znova\n");
                    //poskodene packety treba poslat znova
                    send_broken_packets(backup_ok_packets, packet_treshold, socket_with_config);
                }
                packet_treshold = calculate_treshold(total_size);      //pre dalsiu iteraciu
            }

            aktual = aktual->next;
        }


        //dokoncilo sa posielanie sprav TODO uvolnit pointery
        free(first_packet);
        free_linked_list(&header);


    }

    return NULL;
}

PACKET_HEADER *create_packet(char type, int fragment_number, int crc, int data_size, unsigned char *data) {
    PACKET_HEADER *packet = (PACKET_HEADER *) malloc(sizeof(PACKET_HEADER));

    packet->type = type;
    packet->fragment_number = fragment_number;
    packet->crc = crc;
    packet->data_size = data_size;

//    packet->data = (unsigned char*)malloc(data_size * sizeof(unsigned char));
    if (data != NULL) {
        memcpy(packet->data, data, data_size * sizeof(unsigned char));
    }

    return packet;
}

int calculate_treshold(int total_size) {
    if (total_size <= 20) {
        return total_size;
    } else {
        return 20;
    }
}

void send_broken_packets(MESSAGE_PART *backup_ok_packets, int packet_treshold, CONFIGURATION *configuration) {
    MESSAGE_PART *aktual = backup_ok_packets;
    int counter = 0;

    while (counter < packet_treshold) {
        aktual->packet->crc = hash(aktual->packet->data, aktual->packet->data_size);
        if(contains(aktual->packet->fragment_number) || counter == packet_treshold - 1 ){
            send_message(configuration, aktual->packet);
        }
        counter++;
        aktual = aktual->next;
    }
}

int contains(int packet_number){
    if(broken_packets != NULL){
        for(int i = 0; i < broken_packets_size; i++){
            if(packet_number == broken_packets[i]){
                return 1;
            }
        }
    }
    return 0;
}

int hash(unsigned char *data, int length) {
    int hashed_string = 0;

    for (int i = 0; i < length; i++) {
        hashed_string += data[i] % PRIME;
    }
    return hashed_string;
}

void send_message(CONFIGURATION *configuration, PACKET_HEADER *packet) {
    char *data = (char *) packet;
    if (sendto(configuration->socket, data, sizeof(PACKET_HEADER), 0,
               (struct sockaddr *) &(configuration->socket_config), sizeof(configuration->socket_config)) ==
        SOCKET_ERROR) {
        printf("sendto() zlyhalo error code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
}

void free_linked_list(MESSAGE_PART **linked_list) {
    MESSAGE_PART *aktual = *linked_list, *next;

    while (aktual != NULL) {

        next = aktual->next;
        free(aktual);
        aktual = next;
    }

    *linked_list = NULL;
}