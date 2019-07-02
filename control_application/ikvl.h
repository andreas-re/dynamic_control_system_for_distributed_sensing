// int key value list
#ifndef _IKVL_H_
#define _IKVL_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct ikvl_element ikvl_element;

struct ikvl_element {
	ikvl_element *prev;
	ikvl_element *next;
	ikvl_element *last; // only set on first element in list
	int key;
	void *value;
};

/* returns start element */
ikvl_element *ikvl_add(ikvl_element *start, int key, void *value);
/* returns new start element */
ikvl_element *ikvl_remove_by_key(ikvl_element *start, int key);
/* returns element if found, else NULL */
ikvl_element *ikvl_element_by_key(ikvl_element *start, int key);
/* returns value if found, else NULL */
void *ikvl_value_by_key(ikvl_element *start, int key);
/* check if key exists */
bool ikvl_key_exists(ikvl_element *start, int key);

#endif /* _IKVL_H_ */
