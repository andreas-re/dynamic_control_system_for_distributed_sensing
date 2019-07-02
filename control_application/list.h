#ifndef _LIST_H_
#define _LIST_H_

typedef struct list_element list_element;

#include <stdbool.h>
#include "mqtt_handling.h"
#include "main.h"

struct list_element {
	list_element *prev;
	list_element *next;
	list_element *last; // only set on first element in list
	void *data;
};

/* returns start element */
list_element *list_add(list_element *start, void *data);
/* returns new start element */
list_element *list_remove(list_element *start, void *data);
/* returns new start element */
list_element *list_remove_element(list_element *start, list_element *remove_el);
/* returns element if found, else NULL */
list_element *list_get_element_by_ptr(list_element *start, void *data);
/* frees the list and returns NULL */
list_element *list_free(list_element *start);
/* returns the elements inside the list with given first element */
int list_count(list_element *start);

/* returns start element */
list_element *list_mqtt_id_add(list_element *start, uint32_t mqtt_id);
/* returns the element with the given mqtt_id if it exists, else NULL */
mqtt_id_clients *list_mqtt_id_get(list_element *start, uint32_t mqtt_id);
/* returns the client if found, else NULL */
client_data *list_get_data_by_clientid(list_element *start, char *client_id);
/* returns the clients list for the given mqtt id if it exists, else NULL */
list_element *list_mqtt_get_clients(list_element *start, uint32_t mqtt_id);
/* returns the new start element */
list_element *list_mqtt_id_remove(list_element *start, uint32_t mqtt_id);

#endif /* _LIST_H_ */
