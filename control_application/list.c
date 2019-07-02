#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "main.h"

/* add an element to a list */
list_element *list_add(list_element *start, void *data) {
	list_element *new_el;

	new_el = malloc(sizeof(list_element));
	new_el->data = data;
	new_el->prev = NULL;
	new_el->next = NULL;
	new_el->last = NULL;

	if(start == NULL) {
		// create new list
		new_el->prev = NULL;
		new_el->next = NULL;
		new_el->last = new_el;

		return new_el;
	}
		else {
			start->last->next = new_el;
			new_el->prev = start->last;
			new_el->next = NULL;
			start->last = new_el;

			return start;
		}
}

list_element *list_remove_element(list_element *start, list_element *remove_el) {
	list_element *new_start = NULL;

	if(remove_el != NULL) {
		if(start == remove_el) {
			new_start = start->next; // may be NULL
			if(new_start != NULL) {
				if(remove_el->last == remove_el) new_start->last = new_start;
					else new_start->last = remove_el->last;
			}
		}
			else {
				list_element *prev = remove_el->prev; // can not be NULL
				list_element *next = remove_el->next; // may be NULL

				prev->next = next;
				if(next != NULL) {
					next->prev = prev;
					start->last = next;
				}
					else start->last = prev;
				new_start = start;
			}

		free(remove_el);

		return new_start;
	}
		else return start; // element not found
}

list_element *list_remove(list_element *start, void *data) {
	return list_remove_element(start, list_get_element_by_ptr(start, data));
}

list_element *list_get_element_by_ptr(list_element *start, void *data) {
	list_element *el = start;

	while(el != NULL) {
		// comparing if pointer address is the same
		if(el->data == data) return el;
		el = el->next;
	}
	return NULL;
}

list_element *list_free(list_element *start) {
	list_element *el = start;
	list_element *next;

	while(el != NULL) {
		next = el->next;
		free(el);
		el = next;
	}
	return NULL;
}

int list_count(list_element *start) {
	list_element *el = start;
	int count = 0;

	while(el != NULL) {
		count++;
		el = el->next;
	}
	return count;
}

client_data *list_get_data_by_clientid(list_element *start, char *client_id) {
	list_element *el = start;
	client_data *client;

	while(el != NULL) {
		client = el->data;
		if(client->id != NULL && client_id != NULL && strcmp(client->id, client_id) == 0) return client;
		el = el->next;
	}
	return NULL;
}

list_element *list_mqtt_id_add(list_element *start, uint32_t mqtt_id) {
	list_element *el = start;
	mqtt_id_clients *new_el;
	bool el_found = false;

	while(el != NULL) {
		if(((mqtt_id_clients *)el->data)->mqtt_id == mqtt_id) return start;
		el = el->next;
	}
	new_el = malloc(sizeof(mqtt_id_clients));
	new_el->mqtt_id = mqtt_id;
	new_el->clients_list = NULL;
	return list_add(start, new_el);
}

mqtt_id_clients *list_mqtt_id_get(list_element *start, uint32_t mqtt_id) {
	list_element *el = start;

	while(el != NULL) {
		if(((mqtt_id_clients *)el->data)->mqtt_id == mqtt_id) return (mqtt_id_clients *) el->data;
		el = el->next;
	}
	return NULL;
}

list_element *list_mqtt_get_clients(list_element *start, uint32_t mqtt_id) {
	mqtt_id_clients *idclmap = list_mqtt_id_get(start, mqtt_id);

	if(idclmap == NULL) return NULL;
		else return idclmap->clients_list;
}

list_element *list_mqtt_id_remove(list_element *start, uint32_t mqtt_id) {
	list_element *el = start;

	while(el != NULL) {
		if(((mqtt_id_clients *)el->data)->mqtt_id == mqtt_id) {
			((mqtt_id_clients *)el->data)->clients_list = list_free(((mqtt_id_clients *)el->data)->clients_list);
			free(el->data);
			return list_remove_element(start, el);
		}
		el = el->next;
	}
	return start;
}
