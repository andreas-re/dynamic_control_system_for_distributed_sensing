#ifndef COMPONENTS_QUEUE_QUEUE_H_
#define COMPONENTS_QUEUE_QUEUE_H_

#include "freertos/event_groups.h"

static EventGroupHandle_t *event_group;

typedef struct data_t {
    char *request;
    int client_socket;
} data_t;

typedef struct item_t {
    struct item_t *next;
    struct item_t *prev;
    struct data_t *data;
} item_t;

typedef struct{
    struct item_t *top;
    struct item_t *bottom;
} queue_t;

queue_t *queue_new (); //EventGroupHandle_t *queue_event_group);

void set_event_group(EventGroupHandle_t *queue_event_group);

void add_last (queue_t *stack, data_t *data);

data_t *remove_first(queue_t *queue);

#endif /* COMPONENTS_QUEUE_QUEUE_H_ */
