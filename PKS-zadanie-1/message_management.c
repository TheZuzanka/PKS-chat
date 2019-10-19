// Created by Susanka on 27/09/2019.

#include "header.h"

int get_message(unsigned char **final_message, int message_size, FILE *file, int *loaded_size) {
    *final_message = (unsigned char *) malloc((message_size + 1) * sizeof(unsigned char));
    unsigned char *character = malloc(sizeof(unsigned char));
    int value;
    int counter = 0;
    while ((value = fread(character, sizeof(unsigned char), 1, file)) != 0) {
        memcpy(&(*final_message)[counter], character, sizeof(unsigned char));
        counter++;
        if (file == stdin) {
            if (*character == '\n') {
                *loaded_size = counter;
                (*final_message)[counter] = '\0';
                return 0;
            } else if (counter == message_size) {
                *loaded_size = counter + 1;
                (*final_message)[counter] = '\0';
                break;
            }
        } else {
            if (counter == message_size) {
                break;
            }
        }
    }
    (*final_message)[counter] = '\0';
    *loaded_size = counter;
    if (value == 0) {
        return 0;
    }
    return counter;
}