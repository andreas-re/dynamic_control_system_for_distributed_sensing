#ifndef _METADATA_H_
#define _METADATA_H_

typedef struct metadata_value metadata_value;
typedef union metadata_union metadata_union;
typedef struct metadata_array metadata_array;
typedef struct metadata_kv metadata_kv;

#include <stdbool.h>
#include <msgpack.h>
#include "list.h"

typedef enum {
	METADATA_TYPE_INTEGER = 0,
	METADATA_TYPE_LONG = 1,
	METADATA_TYPE_FLOAT = 2,
	METADATA_TYPE_DOUBLE = 3,
	METADATA_TYPE_BOOLEAN = 4,
	METADATA_TYPE_STRING = 5,
	METADATA_TYPE_ARRAY = 6,
	METADATA_TYPE_KEYVALUE = 7
} metadata_type;

typedef enum {
	METADATA_CMPTYPE_NNE = 0,
	METADATA_CMPTYPE_NNL = 1,
	METADATA_CMPTYPE_NNLE = 2,
	METADATA_CMPTYPE_NNG = 3,
	METADATA_CMPTYPE_NNGE = 4,
	METADATA_CMPTYPE_BB = 5,
	METADATA_CMPTYPE_SSE = 6,
	METADATA_CMPTYPE_SSMCC = 7,
	METADATA_CMPTYPE_SSCCM = 8,
	METADATA_CMPTYPE_SIE = 9,
	METADATA_CMPTYPE_SIL = 10,
	METADATA_CMPTYPE_SILE = 11,
	METADATA_CMPTYPE_SIG = 12,
	METADATA_CMPTYPE_SIGE = 13,
	METADATA_CMPTYPE_AIE = 14,
	METADATA_CMPTYPE_AIL = 15,
	METADATA_CMPTYPE_AILE = 16,
	METADATA_CMPTYPE_AIG = 17,
	METADATA_CMPTYPE_AIGE = 18,
	METADATA_CMPTYPE_AANYC = 19
} metadata_cmp_type;

union metadata_union {
	int i;
	long l;
	float f;
	double d;
	bool b;
	char *s;
	metadata_array *a;
	metadata_kv *kv;
};

struct metadata_value {
	metadata_type type;
	metadata_union data;
};

struct metadata_array {
	metadata_value value;
	metadata_array *next; // is NULL if no next element
};

struct metadata_kv {
	char *key; // null terminated string
	metadata_value value;
	metadata_kv *next; // is NULL if no next element
};

void metadata_print(metadata_value *obj);
void metadata_print_kv(metadata_kv *kv);
void metadata_free_value(metadata_value *value);
metadata_kv *metadata_merge(metadata_kv *kv1, metadata_kv *kv2_prio);
metadata_value *metadata_kv_get_value_ptr(metadata_kv *keyvalue, char *key);
void metadata_free_kv(metadata_kv *keyvalue);
bool metadata_check_insert_svalue(metadata_value *mval, void *v_pos, uint16_t v_len);
list_element *metadata_get_matching_clients(char *key, metadata_value mval, int cmp_type);

int metadata_evaluate_msgpack_value(msgpack_object msgpack_value, metadata_value *store_value);
int metadata_evaluate_msgpack_array(msgpack_object_array msgpack_array, metadata_array *store_array);
int metadata_evaluate_msgpack_keyvalue(msgpack_object_map map, metadata_kv *keyvalue);

void metadata_convert_msgpack_location(client_data *client, msgpack_packer *packer);

void metadata_add_compare_function(bool (*cmp_function)(int, char *, metadata_kv *, metadata_kv *), int cmp_type);

extern pthread_mutex_t metadata_compare_functions_mutex;

#endif /* _METADATA_H_ */
