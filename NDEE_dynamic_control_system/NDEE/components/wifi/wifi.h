#ifndef COMPONENTS_WIFI_WIFI_H_
#define COMPONENTS_WIFI_WIFI_H_

#include "queue.h"
#include "answer.h"

typedef struct {
    int cs;
    char* message;
} communication_t;

int answer (int client, answer_decoded_t *value);

int send_binary(char *msg, int size);

void wifi_ntp_get_time();

int open_network();

#endif /* COMPONENTS_WIFI_WIFI_H_ */
