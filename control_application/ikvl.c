#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ikvl.h"


ikvl_element *ikvl_add(ikvl_element *start, int key, void *value) {
	ikvl_element *new_el;

	new_el = malloc(sizeof(ikvl_element));
	new_el->key = key;
	new_el->value = value;

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
			start->last = new_el;

			return start;
		}
}

ikvl_element *ikvl_remove_by_key(ikvl_element *start, int key) {
	ikvl_element *el = ikvl_element_by_key(start, key);
	ikvl_element *new_start = NULL;

	if(el != NULL) {
		if(start == el) {
			new_start = start->next;
		}
			else {
				ikvl_element *prev = el->prev; // can not be NULL
				ikvl_element *next = el->next; // may be NULL

				prev->next = next;
				if(next != NULL) next->prev = prev;
				new_start = start;
			}

		free(el);

		return new_start;
	}
		else return start; // element not found
}

ikvl_element *ikvl_element_by_key(ikvl_element *start, int key) {
	ikvl_element *el = start;

	while(el != NULL) {
		if(el->key == key) return el;
		el = el->next;
	}
	return NULL;
}

void *ikvl_value_by_key(ikvl_element *start, int key) {
	ikvl_element *el = start;

	while(el != NULL) {
		if(el->key == key) return el->value;
		el = el->next;
	}
	return 0;
}

bool ikvl_key_exists(ikvl_element *start, int key) {
	ikvl_element *el = start;

	while(el != NULL) {
		if(el->key == key) return true;
		el = el->next;
	}
	return false;
}