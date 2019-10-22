// Created by Susanka on 10/10/2019.

#include "header.h"

void *receiving(void *vargp) {
    CONFIGURATION *socket_with_config = (CONFIGURATION *) vargp;
    unsigned char **messages = (unsigned char **) malloc(sizeof(unsigned char *));
    int total_packets = 0;
    int *messages_sizes;
    FILE *file;
    char type;

    while (run_thread) {
        PACKET_HEADER *recv_message = receive_message(socket_with_config);
        FIRST_MESSAGE *first_message;
        PACKET_HEADER *response;

        switch (recv_message->type) {
            case T_INITIAL_MESSAGE :
                first_message = (FIRST_MESSAGE *) (recv_message->data);
                type = first_message->type;
                total_packets = first_message->total_packets;
                printf("Budem prijimat %d fragmentov\n", total_packets);
                messages = malloc(total_packets * sizeof(unsigned char *));
                messages_sizes = (int *) calloc((first_message->total_packets), sizeof(int));
                if (type == T_FILE) {
                    file = fopen(first_message->file_name, "ab");
                    char full_path[_MAX_PATH];
                    _fullpath(full_path, first_message->file_name, _MAX_PATH);
                    printf("V adresári %s bol vytvoreny subor s menom: %s\n", full_path ,first_message->file_name);
                } else {
                    file = stdout;
                }

                response = create_packet(T_RESPONSE, 1, 0, 0, NULL);
                send_message(socket_with_config, response);

                printf("Prisiel initial message\n");
                break;

            case T_TEXT :
            case T_FILE:
                messages[recv_message->fragment_number] = (unsigned char *) malloc(
                        max_size_of_packet * sizeof(unsigned char));
                memcpy(messages[recv_message->fragment_number], recv_message->data,
                       max_size_of_packet);
                messages_sizes[recv_message->fragment_number] = recv_message->data_size;

                printf("Prijimam packet #%d/%d, velkost = %d\n", (recv_message->fragment_number) + 1, total_packets,
                       recv_message->data_size + 16);

                int expected_crc = hash(recv_message->data, recv_message->data_size);
                if(expected_crc != recv_message->crc){
                    printf("Prisiel chybny packet, hash sa nezhoduje\n");
                    if(broken_packets == NULL){
                        broken_packets = malloc(20 * sizeof(int));
                    }
                    printf("Pridavam packet do pola, ktore packety chcem poslat znova\n");
                    broken_packets[broken_packets_size] = recv_message->fragment_number;
                    broken_packets_size++;
                }

                //ak je to posledna sprava a pole chybnych je prazdne, vypis/ uloz.
                if ((recv_message->fragment_number == total_packets - 1) && (broken_packets == NULL)) {
                    for (int i = 0; i < total_packets; i++) {
                        fwrite(messages[i], sizeof(unsigned char), messages_sizes[i], file);
                    }
                    if (file != stdout) {
                        fclose(file);
                    }
                }

                //ked prisiel posledny fragment alebo som prisla na kontrolny bod (20 packetov), poslem odpoved
                //ci prisli vsetky v poriadku a mozem posielat dalej alebo vyziadam chybne
                if (recv_message->fragment_number % 20 == 19 || (recv_message->fragment_number == total_packets - 1)) {
                    response = create_packet(T_RESPONSE, 1, 0, broken_packets_size, (unsigned char*)broken_packets);
                    send_message(socket_with_config, response);
                    free(broken_packets);
                    broken_packets = NULL;
                    broken_packets_size = 0;
                }

                break;
            case T_KEEP_ALIVE:
                //som server, dostal som keep alive a posielam odpoved "ano som este tu"
                response = create_packet(T_KEEP_RESPONSE, 1, 0, -1, NULL);
                //-1 aby mi keepalive nevynuloval pole chybnych
                send_message(socket_with_config, response);
                break;
            case T_KEEP_RESPONSE:
                //som klint, server odpoveda na moj keep alive "ano som este tu"
                counter = 0;
                break;
            case T_RESPONSE:
                printf("Prisla odpoved\n");
                broken_packets = (int *) (recv_message->data);
                broken_packets_size = recv_message->data_size;      //obsahuje informaciu kolko prvkov v poli mi prislo
                if (broken_packets_size == 0) {
                    broken_packets = NULL;
                }
                wait = 0;
                break;
            default:
                break;
        }
    }
    return NULL;
}

PACKET_HEADER *receive_message(CONFIGURATION *configuration) {
    char *message = (char *) malloc(sizeof(PACKET_HEADER));
    int size = sizeof(configuration->socket_config);

    if ((recvfrom(configuration->socket, message, sizeof(PACKET_HEADER), 0,
                  (struct sockaddr *) &(configuration->socket_config), &size)) ==
        SOCKET_ERROR) {
        //printf("recvfrom() zlyhalo error code : %d", WSAGetLastError());
        //printf("server odpojeny\n");
        //exit(EXIT_FAILURE);
    } else {

    }

    return (PACKET_HEADER *) message;
}