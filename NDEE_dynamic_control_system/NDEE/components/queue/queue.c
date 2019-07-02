#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "queue.h"

//static EventGroupHandle_t *event_group;

//Create new queue
queue_t *queue_new() //EventGroupHandle_t *queue_event_group)
{
    queue_t *queue = malloc(sizeof(*queue));
    if (queue == NULL) {
        return NULL;
    }
    queue->top = NULL;
    queue->bottom = NULL;
    //event_group = queue_event_group;
    return queue;
}

void set_event_group(EventGroupHandle_t *queue_event_group)
{
    event_group = queue_event_group;
}

//Push item to top of queue
void add_last(queue_t *queue, data_t *data)
{
    item_t *item = malloc(sizeof(*item));
    if (queue != NULL && item != NULL) {
        item->data = data;
        item->next = queue->bottom;
        item->prev = NULL;
        if (queue->bottom != NULL) {
            queue->bottom->prev = item;
        }
        queue->bottom = item;
        if (queue->top == NULL) {
            queue->top = item;
        }
    //xEventGroupSetBits(event_group, BIT0);
    }
}

//Get element from top of queue (with removing it)
data_t *remove_first(queue_t *queue)
{
    if (queue != NULL && queue->top != NULL) {
        item_t *item = queue->top;
        char *data = item->data;
        queue->top = item->prev;
        if(queue->top == NULL){
            queue->bottom = NULL;
            //xEventGroupClearBits(event_group, BIT0);
        }
        else {
            item->prev->next = NULL;
        }
        free(item);
        return data;
    }
    return NULL;
}
