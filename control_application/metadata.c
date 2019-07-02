#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <msgpack.h>
#include "metadata.h"
#include "main.h"
#include "ikvl.h"

ikvl_element *metadata_compare_functions = NULL;
pthread_mutex_t metadata_compare_functions_mutex;

void metadata_print_obj(metadata_value *obj, int indentation);
void metadata_print_array(metadata_array *array, int indentation);
void metadata_print_keyvalue(metadata_kv *array, int indentation);

void metadata_merge_kv(metadata_kv *kv1, metadata_kv *kv2_prio);
void metadata_merge_single_kv(metadata_kv *dest, metadata_kv *src);

void metadata_copy_kv(metadata_value *dest_val, metadata_kv *src);
void metadata_copy_array(metadata_value *dest_val, metadata_array *src);

void metadata_free_value(metadata_value *value);
void metadata_free_array(metadata_array *array);
void metadata_free_kv(metadata_kv *keyvalue);

void metadata_convert_msgpack_value(metadata_value value, msgpack_packer *pk);
void metadata_convert_msgpack_keyvalue(metadata_kv *keyvalue, msgpack_packer *pk);
void metadata_convert_msgpack_array(metadata_array *array, msgpack_packer *pk);

bool metadata_compare_array(metadata_array *a1, metadata_array *a2);
bool metadata_compare_kv(metadata_kv *keyvalue1, metadata_kv *keyvalue2);

metadata_kv *metadata_get_kv_ptr_by_key(metadata_kv *keyvalue, char *key);


void metadata_print_indentation(int indentation) {
	if(indentation > 0) {
		for(int i = 0; i < indentation; i++) printf("	");
	}
}

void metadata_print_keyvalue(metadata_kv *keyvalue, int indentation) {
	metadata_kv *kv = keyvalue;

	if(kv == NULL) {
		metadata_print_indentation(indentation);
		printf("<keyvalue_null>");
	}
		else {
			while(kv != NULL) {
				metadata_print_indentation(indentation);
				printf("%s: ", kv->key);
				metadata_print_obj(&kv->value, indentation+1);
				kv = kv->next;
				if(kv != NULL) printf("\n");
			}
		}
}

void metadata_print_array(metadata_array *array, int indentation) {
	bool first_element = true;
	metadata_array *a = array;

	if(array == NULL) printf("<array_null>");
		else {
			while(a != NULL) {
				if(first_element) first_element = false;
					else printf(" | ");
				metadata_print_obj(&a->value, indentation);
				a = a->next;
			}
		}
}

void metadata_print_obj(metadata_value *obj, int indentation) {
	if(obj->type == METADATA_TYPE_INTEGER) {
		printf("%i", obj->data.i);
	}
	else if(obj->type == METADATA_TYPE_LONG) {
		printf("%li", obj->data.l);
	}
	else if(obj->type == METADATA_TYPE_FLOAT) {
		printf("%f", obj->data.f);
	}
	else if(obj->type == METADATA_TYPE_DOUBLE) {
		printf("%f", obj->data.d);
	}
	else if(obj->type == METADATA_TYPE_BOOLEAN) {
		if(obj->data.b) printf("true");
			else printf("false");
	}
	else if(obj->type == METADATA_TYPE_STRING) {
		printf("\"%s\"", obj->data.s);
	}
	else if(obj->type == METADATA_TYPE_ARRAY) {
		metadata_print_array(obj->data.a, indentation);
	}
	else if(obj->type == METADATA_TYPE_KEYVALUE) {
		printf("\n");
		metadata_print_keyvalue(obj->data.kv, indentation);
	}
}


void metadata_print_kv(metadata_kv *kv) {
	metadata_print_keyvalue(kv, 0);
	printf("\n");
}
void metadata_print(metadata_value *obj) {
	metadata_print_obj(obj, 0);
}


metadata_value *metadata_kv_get_value_ptr(metadata_kv *keyvalue, char *key) {
	metadata_kv *kv = keyvalue;

	while(kv != NULL) {
		if(strcmp(kv->key, key) == 0) return &kv->value;
		kv = kv->next;
	}
	return NULL;
}

void metadata_copy_value(metadata_value *dest, metadata_value src) {
	if(src.type == METADATA_TYPE_INTEGER) {
		dest->type = METADATA_TYPE_INTEGER;
		dest->data.i = src.data.i;
	}
	else if(src.type == METADATA_TYPE_LONG) {
		dest->type = METADATA_TYPE_LONG;
		dest->data.l = src.data.l;
	}
	else if(src.type == METADATA_TYPE_FLOAT) {
		dest->type = METADATA_TYPE_FLOAT;
		dest->data.f = src.data.f;
	}
	else if(src.type == METADATA_TYPE_DOUBLE) {
		dest->type = METADATA_TYPE_DOUBLE;
		dest->data.d = src.data.d;
	}
	else if(src.type == METADATA_TYPE_BOOLEAN) {
		dest->type = METADATA_TYPE_BOOLEAN;
		dest->data.b = src.data.b;
	}
	else if(src.type == METADATA_TYPE_STRING) {
		dest->type = METADATA_TYPE_STRING;
		dest->data.s = malloc(strlen(src.data.s)+1);
		memcpy(dest->data.s, src.data.s, strlen(src.data.s)+1);
	}
	else if(src.type == METADATA_TYPE_ARRAY) {
		metadata_copy_array(dest, src.data.a);
	}
	else if(src.type == METADATA_TYPE_KEYVALUE) {
		metadata_copy_kv(dest, src.data.kv);
	}
}

void metadata_copy_array(metadata_value *dest_val, metadata_array *src) {
	metadata_array *a = src;
	metadata_array *dest;

	dest_val->type = METADATA_TYPE_ARRAY;
	if(src != NULL) {
		dest = malloc(sizeof(metadata_array));
		dest_val->data.a = dest;

		while(a != NULL) {
			metadata_copy_value(&dest->value, a->value);
			a = a->next;
			if(a != NULL) {
				dest->next = malloc(sizeof(metadata_array));
				dest = dest->next;
			}
				else dest->next = NULL;
		}
	}
		else {
			dest_val->data.a = NULL;
		}
}

void metadata_copy_kv(metadata_value *dest_val, metadata_kv *src) {
	metadata_kv *kv = src;
	metadata_kv *dest;

	dest_val->type = METADATA_TYPE_KEYVALUE;
	if(src != NULL) {
		dest = malloc(sizeof(metadata_kv));
		dest_val->data.kv = dest;

		while(kv != NULL) {
			dest->key = malloc(strlen(kv->key)+1);
			memcpy(dest->key, kv->key, strlen(kv->key)+1);
			metadata_copy_value(&dest->value, kv->value);
			kv = kv->next;
			if(kv != NULL) {
				dest->next = malloc(sizeof(metadata_kv));
				dest = dest->next;
			}
				else dest->next = NULL;
		}
	}
		else {
			dest_val->data.kv = NULL;
		}
}

bool metadata_compare_value(metadata_value v1, metadata_value v2) {
	if(v1.type == v2.type) {
		if(v1.type == METADATA_TYPE_INTEGER) {
			if(v1.data.i == v2.data.i) return true;
				else return false;
		}
		else if(v1.type == METADATA_TYPE_LONG) {
			if(v1.data.l == v2.data.l) return true;
				else return false;
		}
		else if(v1.type == METADATA_TYPE_FLOAT) {
			if(v1.data.f == v2.data.f) return true;
				else return false;
		}
		else if(v1.type == METADATA_TYPE_DOUBLE) {
			if(v1.data.d == v2.data.d) return true;
				else return false;
		}
		else if(v1.type == METADATA_TYPE_BOOLEAN) {
			if(v1.data.b == v2.data.b) return true;
				else return false;
		}
		else if(v1.type == METADATA_TYPE_STRING) {
			if(strcmp(v1.data.s, v2.data.s) == 0) return true;
				else return false;
		}
		else if(v1.type == METADATA_TYPE_ARRAY) {
			return metadata_compare_array(v1.data.a, v2.data.a);
		}
		else if(v1.type == METADATA_TYPE_KEYVALUE) {
			return metadata_compare_kv(v1.data.kv, v2.data.kv);
		}
	}
		else return false;
}

bool metadata_compare_array(metadata_array *array1, metadata_array *array2) {
	metadata_array *a1 = array1;
	metadata_array *a2 = array2;

	while(true) {
		if(a1 == NULL && a2 == NULL) return true;
		else if(a1 == NULL || a2 == NULL) return false;
			else {
				if(!metadata_compare_value(a1->value, a2->value)) return false;
			}
		a1 = a1->next;
		a2 = a2->next;
	}
}

bool metadata_compare_kv(metadata_kv *keyvalue1, metadata_kv *keyvalue2) {
	metadata_kv *kv1 = keyvalue1;
	metadata_kv *kv2 = keyvalue2;

	while(true) {
		if(kv1 == NULL && kv2 == NULL) return true;
		else if(kv1 == NULL || kv2 == NULL) return false;
			else {
				if(strcmp(kv1->key, kv2->key) != 0) return false;
				if(!metadata_compare_value(kv1->value, kv2->value)) return false;
			}
		kv1 = kv1->next;
		kv2 = kv2->next;
	}
}

void metadata_free_value(metadata_value *value) {
	if(value->type == METADATA_TYPE_STRING) {
		if(value->data.s != NULL) {
			free(value->data.s);
			value->data.s = NULL;
		}
	}
	else if(value->type == METADATA_TYPE_ARRAY) {
		metadata_free_array(value->data.a);
	}
	else if(value->type == METADATA_TYPE_KEYVALUE) {
		metadata_free_kv(value->data.kv);
	}
}

void metadata_free_array(metadata_array *array) {
	metadata_array *a = array;
	metadata_array *next;

	while(a != NULL) {
		next = a->next;
		metadata_free_value(&a->value);
		free(a);
		a = next;
	}
}

void metadata_free_kv(metadata_kv *keyvalue) {
	metadata_kv *kv = keyvalue;
	metadata_kv *next;

	while(kv != NULL) {
		next = kv->next;
		free(kv->key);
		metadata_free_value(&kv->value);
		free(kv);
		kv = next;
	}
}

void metadata_array_add_value(metadata_array *dest, metadata_value val) {
	metadata_array *a = dest;
	metadata_array *last_el;

	while(a != NULL) {
		if(metadata_compare_value(a->value, val)) return;
		if(a->next == NULL) last_el = a;
		a = a->next;
	}
	last_el->next = malloc(sizeof(metadata_array));
	metadata_copy_value(&last_el->next->value, val);
}

void metadata_merge_array(metadata_array *dest, metadata_array *src) {
	metadata_array *last_el = NULL;
	metadata_array *a = src;

	if(src == NULL) {
		metadata_free_array(dest);
		return;
	}
	while(a != NULL) {
		metadata_array_add_value(dest, a->value);
		a = a->next;
	}
}

void metadata_merge_value(metadata_value *dest, metadata_value src) {
	if(src.type == METADATA_TYPE_INTEGER) {
		metadata_free_value(dest);
		dest->type = METADATA_TYPE_INTEGER;
		dest->data.i = src.data.i;
	}
	else if(src.type == METADATA_TYPE_LONG) {
		metadata_free_value(dest);
		dest->type = METADATA_TYPE_LONG;
		dest->data.l = src.data.l;
	}
	else if(src.type == METADATA_TYPE_FLOAT) {
		metadata_free_value(dest);
		dest->type = METADATA_TYPE_FLOAT;
		dest->data.f = src.data.f;
	}
	else if(src.type == METADATA_TYPE_DOUBLE) {
		metadata_free_value(dest);
		dest->type = METADATA_TYPE_DOUBLE;
		dest->data.d = src.data.d;
	}
	else if(src.type == METADATA_TYPE_BOOLEAN) {
		metadata_free_value(dest);
		dest->type = METADATA_TYPE_BOOLEAN;
		dest->data.b = src.data.b;
	}
	else if(src.type == METADATA_TYPE_STRING) {
		metadata_free_value(dest);
		dest->type = METADATA_TYPE_STRING;
		dest->data.s = malloc(strlen(src.data.s)+1);
		memcpy(dest->data.s, src.data.s, strlen(src.data.s)+1);
	}
	else if(src.type == METADATA_TYPE_ARRAY) {
		if(dest->type == METADATA_TYPE_ARRAY) {
			metadata_merge_array(dest->data.a, src.data.a);
		}
			else {
				metadata_free_value(dest);
				metadata_copy_array(dest, src.data.a);
			}
	}
	else if(src.type == METADATA_TYPE_KEYVALUE) {
		if(dest->type == METADATA_TYPE_KEYVALUE) {
			metadata_merge_kv(dest->data.kv, src.data.kv);
		}
			else {
				metadata_free_value(dest);
				metadata_copy_kv(dest, src.data.kv);
			}
	}
}

// merge single keyvalue element; dest must not be null
void metadata_merge_single_kv(metadata_kv *dest, metadata_kv *src) {
	metadata_kv *kv = dest;
	metadata_kv *last_kv;

	while(kv != NULL) {
		if(strcmp(kv->key, src->key) == 0) {
			metadata_merge_value(&kv->value, src->value);
			return;
		}
		if(kv->next == NULL) last_kv = kv;
		kv = kv->next;
	}
	// key doesn't exist in dest yet
	last_kv->next = malloc(sizeof(metadata_kv));
	last_kv->next->key = malloc(strlen(src->key)+1);
	memcpy(last_kv->next->key, src->key, strlen(src->key)+1); // +1 because of null byte
	metadata_copy_value(&last_kv->next->value, src->value);
	last_kv->next->next = NULL;
}

void metadata_merge_kv(metadata_kv *kv1, metadata_kv *kv2_prio) {
	metadata_kv *kv = kv2_prio;
	while(kv != NULL) {
		metadata_merge_single_kv(kv1, kv);
		kv = kv->next;
	}
}

/* values of kv2_prio are more important than values of kv1 */
metadata_kv *metadata_merge(metadata_kv *kv1, metadata_kv *kv2_prio) {
	metadata_value val_ret;

	if(kv1 == NULL && kv2_prio == NULL) {
		return NULL;
	}
	else if(kv1 == NULL) {
		metadata_copy_kv(&val_ret, kv2_prio);	
	}
	else if(kv2_prio == NULL) {
		metadata_copy_kv(&val_ret, kv1);	
	}
		else {
			metadata_copy_kv(&val_ret, kv1);
			metadata_merge_kv(val_ret.data.kv, kv2_prio);
		}
	
	return val_ret.data.kv;
}

bool metadata_check_insert_svalue(metadata_value *mval, void *v_pos, uint16_t v_len) {
	if(mval->type == METADATA_TYPE_INTEGER) {
		if(v_len != 4) return false;
		memcpy(&mval->data.i, v_pos, 4);
	}
	else if(mval->type == METADATA_TYPE_LONG) {
		if(v_len != 8) return false;
		memcpy(&mval->data.l, v_pos, 8);
	}
	else if(mval->type == METADATA_TYPE_FLOAT) {
		if(v_len != 4) return false;
		memcpy(&mval->data.f, v_pos, 4);
	}
	else if(mval->type == METADATA_TYPE_DOUBLE) {
		if(v_len != 8) return false;
		memcpy(&mval->data.d, v_pos, 8);
	}
	else if(mval->type == METADATA_TYPE_BOOLEAN) {
		uint8_t b = *((uint8_t *) v_pos);
		if(b == 0) mval->data.b = false;
		else if(b == 0) mval->data.b = true;
			else return false;
	}
	else if(mval->type == METADATA_TYPE_STRING) {
		mval->data.s = malloc(v_len+1);
		memcpy(mval->data.s, v_pos, v_len);
		mval->data.s[v_len] = 0x0;
	}
	else if(mval->type == METADATA_TYPE_ARRAY) {
		// payload is array in msgpack format
		msgpack_unpacked unp;
		msgpack_unpack_return ret;

		msgpack_unpacked_init(&unp);
		ret = msgpack_unpack_next(&unp, v_pos, v_len, NULL);
		if(ret == MSGPACK_UNPACK_SUCCESS && unp.data.type == MSGPACK_OBJECT_ARRAY) {
			if(unp.data.via.array.size > 0) {
				mval->data.a = malloc(sizeof(metadata_array));
				if(metadata_evaluate_msgpack_array(unp.data.via.array, mval->data.a) < 0) {
					msgpack_unpacked_destroy(&unp);
					return false;	
				}
			}
				else mval->data.a = NULL;
			msgpack_unpacked_destroy(&unp);
			return true;
		}
			else {
				msgpack_unpacked_destroy(&unp);
				return false;
			}
	}
	else if(mval->type == METADATA_TYPE_KEYVALUE) {
		// payload is key-value object in msgpack format
		// payload is array in msgpack format
		msgpack_unpacked unp;
		msgpack_unpack_return ret;

		msgpack_unpacked_init(&unp);
		ret = msgpack_unpack_next(&unp, v_pos, v_len, NULL);
		if(ret == MSGPACK_UNPACK_SUCCESS && unp.data.type == MSGPACK_OBJECT_MAP) {
			if(unp.data.via.map.size > 0) {
				mval->data.kv = malloc(sizeof(metadata_kv));
				if(metadata_evaluate_msgpack_keyvalue(unp.data.via.map, mval->data.kv) < 0) {
					msgpack_unpacked_destroy(&unp);
					return false;	
				}
			}
				else mval->data.kv = NULL;
			msgpack_unpacked_destroy(&unp);
			return true;
		}
			else {
				msgpack_unpacked_destroy(&unp);
				return false;
			}
	}
		else return false;

	return true;
}

bool metadata_array_contains(metadata_array *array, metadata_value value) {
	metadata_array *a = array;
	
	while(a != NULL) {
		if(metadata_compare_value(a->value, value)) return true;
		a = a->next;
	}
	return false;
}

int metadata_array_get_length(metadata_array *array) {
	int count = 0;
	metadata_array *a = array;
	while(a != NULL) {
		count++;
		a = a->next;
	}
	return count;
}
int metadata_keyvalue_get_length(metadata_kv *keyvalue) {
	int count = 0;
	metadata_kv *kv = keyvalue;
	while(kv != NULL) {
		count++;
		kv = kv->next;
	}
	return count;
}

/* functions for comparing numbers */
bool metadata_cmp_number_eq(metadata_value v1, metadata_value v2) {
	if(v1.type != v2.type) return false;
	if(v1.type == METADATA_TYPE_INTEGER) return (v1.data.i == v2.data.i) ? true : false;
	else if(v1.type == METADATA_TYPE_LONG) return (v1.data.l == v2.data.l) ? true : false;
	else if(v1.type == METADATA_TYPE_FLOAT) return (v1.data.f == v2.data.f) ? true : false;
	else if(v1.type == METADATA_TYPE_DOUBLE) return (v1.data.d == v2.data.d) ? true : false;
		else return false;
}
bool metadata_cmp_number_le(metadata_value v1, metadata_value v2) {
	if(v1.type != v2.type) return false;
	if(v1.type == METADATA_TYPE_INTEGER) return (v1.data.i < v2.data.i) ? true : false;
	else if(v1.type == METADATA_TYPE_LONG) return (v1.data.l < v2.data.l) ? true : false;
	else if(v1.type == METADATA_TYPE_FLOAT) return (v1.data.f < v2.data.f) ? true : false;
	else if(v1.type == METADATA_TYPE_DOUBLE) return (v1.data.d < v2.data.d) ? true : false;
		else return false;
}
bool metadata_cmp_number_leeq(metadata_value v1, metadata_value v2) {
	if(v1.type != v2.type) return false;
	if(v1.type == METADATA_TYPE_INTEGER) return (v1.data.i <= v2.data.i) ? true : false;
	else if(v1.type == METADATA_TYPE_LONG) return (v1.data.l <= v2.data.l) ? true : false;
	else if(v1.type == METADATA_TYPE_FLOAT) return (v1.data.f <= v2.data.f) ? true : false;
	else if(v1.type == METADATA_TYPE_DOUBLE) return (v1.data.d <= v2.data.d) ? true : false;
		else return false;
}
bool metadata_cmp_number_gr(metadata_value v1, metadata_value v2) {
	if(v1.type != v2.type) return false;
	if(v1.type == METADATA_TYPE_INTEGER) return (v1.data.i > v2.data.i) ? true : false;
	else if(v1.type == METADATA_TYPE_LONG) return (v1.data.l > v2.data.l) ? true : false;
	else if(v1.type == METADATA_TYPE_FLOAT) return (v1.data.f > v2.data.f) ? true : false;
	else if(v1.type == METADATA_TYPE_DOUBLE) return (v1.data.d > v2.data.d) ? true : false;
		else return false;
}
bool metadata_cmp_number_greq(metadata_value v1, metadata_value v2) {
	if(v1.type != v2.type) return false;
	if(v1.type == METADATA_TYPE_INTEGER) return (v1.data.i >= v2.data.i) ? true : false;
	else if(v1.type == METADATA_TYPE_LONG) return (v1.data.l >= v2.data.l) ? true : false;
	else if(v1.type == METADATA_TYPE_FLOAT) return (v1.data.f >= v2.data.f) ? true : false;
	else if(v1.type == METADATA_TYPE_DOUBLE) return (v1.data.d >= v2.data.d) ? true : false;
		else return false;
}

/* compare the metadata values indexed by the given key of the client metadata and the given
	compare metadata
*/
bool metadata_check_client_matching(client_data *client, char *key, metadata_value cmpval, int cmp_type) {
	pthread_mutex_lock(&client->metadata_merged_mutex);
	metadata_kv *md = client->metadata_merged;
	while(md != NULL) {
		if(strcmp(md->key, key) == 0) {
			if(cmpval.type == METADATA_TYPE_KEYVALUE) {
				// complex metadata value
				bool (*cmp_function)(int cmp_type, char *key, metadata_kv *client_metadata, metadata_kv *cmp_metadata);
				pthread_mutex_lock(&metadata_compare_functions_mutex);
				cmp_function = ikvl_value_by_key(metadata_compare_functions, cmp_type);
				pthread_mutex_unlock(&metadata_compare_functions_mutex);
				if(cmp_function != NULL) {
					bool ret_val = false;
					metadata_kv *client_kv = metadata_get_kv_ptr_by_key(client->metadata_merged, key);
					ret_val = (*cmp_function)(cmp_type, key, client_kv, cmpval.data.kv);
					if(ret_val) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else {
						fprintf(stderr, "(client %s) no metadata compare function found for type number %i\n", client->id, cmp_type);
						goto RET_FALSE;
					}
			}
			else if(cmp_type == METADATA_CMPTYPE_NNE) {
				if(metadata_cmp_number_eq(md->value, cmpval)) goto RET_TRUE;
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_NNL) {
				if(metadata_cmp_number_le(md->value, cmpval)) goto RET_TRUE;
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_NNLE) {
				if(metadata_cmp_number_leeq(md->value, cmpval)) goto RET_TRUE;
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_NNG) {
				if(metadata_cmp_number_gr(md->value, cmpval)) goto RET_TRUE;
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_NNGE) {
				if(metadata_cmp_number_greq(md->value, cmpval)) goto RET_TRUE;
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_BB) {
				if(md->value.type == cmpval.type) {
					if(md->value.data.b == cmpval.data.b) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_SSE) {
				if(md->value.type == METADATA_TYPE_STRING && cmpval.type == METADATA_TYPE_STRING) {
					if(strcmp(md->value.data.s, cmpval.data.s) == 0) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_SSMCC) {
				if(md->value.type == METADATA_TYPE_STRING && cmpval.type == METADATA_TYPE_STRING) {
					if(strstr(md->value.data.s, cmpval.data.s) != NULL) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_SSCCM) {
				if(md->value.type == METADATA_TYPE_STRING && cmpval.type == METADATA_TYPE_STRING) {
					if(strstr(cmpval.data.s, md->value.data.s) != NULL) goto RET_FALSE;
						else goto RET_TRUE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_SIE) {
				if(md->value.type == METADATA_TYPE_STRING && cmpval.type == METADATA_TYPE_INTEGER) {
					if(strlen(md->value.data.s) == cmpval.data.i) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_SIL) {
				if(md->value.type == METADATA_TYPE_STRING && cmpval.type == METADATA_TYPE_INTEGER) {
					if(strlen(md->value.data.s) < cmpval.data.i) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_SILE) {
				if(md->value.type == METADATA_TYPE_STRING && cmpval.type == METADATA_TYPE_INTEGER) {
					if(strlen(md->value.data.s) <= cmpval.data.i) goto RET_TRUE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_SIG) {
				if(md->value.type == METADATA_TYPE_STRING && cmpval.type == METADATA_TYPE_INTEGER) {
					if(strlen(md->value.data.s) > cmpval.data.i) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_SIGE) {
				if(md->value.type == METADATA_TYPE_STRING && cmpval.type == METADATA_TYPE_INTEGER) {
					if(strlen(md->value.data.s) >= cmpval.data.i) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_AIE) {
				if(md->value.type == METADATA_TYPE_ARRAY && cmpval.type == METADATA_TYPE_INTEGER) {
					int l = metadata_array_get_length(md->value.data.a);
					if(l == cmpval.data.i) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_AIL) {
				if(md->value.type == METADATA_TYPE_ARRAY && cmpval.type == METADATA_TYPE_INTEGER) {
					int l = metadata_array_get_length(md->value.data.a);
					if(l < cmpval.data.i) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_AILE) {
				if(md->value.type == METADATA_TYPE_ARRAY && cmpval.type == METADATA_TYPE_INTEGER) {
					int l = metadata_array_get_length(md->value.data.a);
					if(l <= cmpval.data.i) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_AIG) {
				if(md->value.type == METADATA_TYPE_ARRAY && cmpval.type == METADATA_TYPE_INTEGER) {
					int l = metadata_array_get_length(md->value.data.a);
					if(l > cmpval.data.i) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_AIGE) {
				if(md->value.type == METADATA_TYPE_ARRAY && cmpval.type == METADATA_TYPE_INTEGER) {
					int l = metadata_array_get_length(md->value.data.a);
					if(l >= cmpval.data.i) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
			else if(cmp_type == METADATA_CMPTYPE_AANYC) {
				if(md->value.type == METADATA_TYPE_ARRAY) {
					if(metadata_array_contains(md->value.data.a, cmpval)) goto RET_TRUE;
						else goto RET_FALSE;
				}
					else goto RET_FALSE;
			}
		}	
		md = md->next;
	}
RET_FALSE:
	pthread_mutex_unlock(&client->metadata_merged_mutex);
	return false;
RET_TRUE:
	pthread_mutex_unlock(&client->metadata_merged_mutex);
	return true;
}

list_element *metadata_get_matching_clients(char *key, metadata_value cmpval, int cmp_type) {
	pthread_mutex_lock(&access_client_list);
	list_element *cl = clients_list;
	list_element *ret = NULL;

	while(cl != NULL) {
		if(metadata_check_client_matching((client_data *) cl->data, key, cmpval, cmp_type)) {
			ret = list_add(ret, cl->data);
		}
		cl = cl->next;
	}
	pthread_mutex_unlock(&access_client_list);
	return ret;
}

int metadata_evaluate_msgpack_value(msgpack_object msgpack_value, metadata_value *store_value) {
	int ret;

	if(msgpack_value.type == MSGPACK_OBJECT_BOOLEAN) {
		store_value->type = METADATA_TYPE_BOOLEAN;
		store_value->data.b = msgpack_value.via.boolean;
	}
	else if(msgpack_value.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
		if(msgpack_value.via.u64 <= INT_MAX) {
			store_value->type = METADATA_TYPE_INTEGER;
			store_value->data.i = msgpack_value.via.u64;
		}
			else {
				// expect that number fits into long
				store_value->type = METADATA_TYPE_LONG;
				store_value->data.l = msgpack_value.via.u64;
			}
	}
	else if(msgpack_value.type == MSGPACK_OBJECT_NEGATIVE_INTEGER) {
		if(msgpack_value.via.i64 >= INT_MIN) {
			store_value->type = METADATA_TYPE_INTEGER;
			store_value->data.i = msgpack_value.via.i64;
		}
			else {
				// expect that number fits into long
				store_value->type = METADATA_TYPE_LONG;
				store_value->data.l = msgpack_value.via.i64;
			}
	}	
	else if(msgpack_value.type == MSGPACK_OBJECT_FLOAT) {
		// as not knowing if float or double precision, store it into double
		store_value->type = METADATA_TYPE_DOUBLE;
		store_value->data.d = msgpack_value.via.f64;
	}
	else if(msgpack_value.type == MSGPACK_OBJECT_STR) {
		store_value->type = METADATA_TYPE_STRING;
		store_value->data.s = malloc(msgpack_value.via.str.size+1);
		memcpy(store_value->data.s, msgpack_value.via.str.ptr, msgpack_value.via.str.size);
		store_value->data.s[msgpack_value.via.str.size] = 0x0;
	}
	else if(msgpack_value.type == MSGPACK_OBJECT_ARRAY) {
		store_value->type = METADATA_TYPE_ARRAY;
		if(msgpack_value.via.array.size == 0) {
			store_value->data.a = NULL;
		}
			else {
				store_value->data.a = malloc(sizeof(metadata_array));
				ret = metadata_evaluate_msgpack_array(msgpack_value.via.array, store_value->data.a);
				if(ret < 0) return (-1);
			}
	}
	else if(msgpack_value.type == MSGPACK_OBJECT_MAP) {
		store_value->type = METADATA_TYPE_KEYVALUE;
		if(msgpack_value.via.map.size == 0) {
			store_value->data.kv = NULL;
		}
			else {
				store_value->data.kv = malloc(sizeof(metadata_kv));
				ret = metadata_evaluate_msgpack_keyvalue(msgpack_value.via.map, store_value->data.kv);
				if(ret < 0) return (-1);
			}
	}
	return 0;
}

int metadata_evaluate_msgpack_array(msgpack_object_array msgpack_array, metadata_array *store_array) {
	int i, ret;
	int a_size = msgpack_array.size;
	metadata_array *a = store_array;

	for(i = 0; i < a_size; i++) {
		ret = metadata_evaluate_msgpack_value(msgpack_array.ptr[i], &a->value);
		if(ret < 0) return (-1);

		if(i < a_size-1) {
			a->next = malloc(sizeof(metadata_array));
			a = a->next;
		}
	}
	a->next = NULL;
	return 0;
}

int metadata_evaluate_msgpack_keyvalue(msgpack_object_map map, metadata_kv *keyvalue) {
	int i, ret;
	int kv_size = map.size;
	metadata_kv *kv = keyvalue;
	msgpack_object_kv msgp_kv;

	for(i = 0; i < kv_size; i++) {
		msgp_kv = map.ptr[i];

		// key
		if(msgp_kv.key.type != MSGPACK_OBJECT_STR) return (-1);
		kv->key = malloc(msgp_kv.key.via.str.size+1);
		memcpy(kv->key, msgp_kv.key.via.str.ptr, msgp_kv.key.via.str.size);
		kv->key[msgp_kv.key.via.str.size] = 0x0;

		// value
		ret = metadata_evaluate_msgpack_value(msgp_kv.val, &kv->value);
		if(ret < 0) return (-1);
		if(i < kv_size-1) {
			kv->next = malloc(sizeof(metadata_kv));
			kv = kv->next;
		}
	}
	kv->next = NULL;
	return 0;
}


void metadata_convert_msgpack_value(metadata_value value, msgpack_packer *pk) {
	if(value.type == METADATA_TYPE_INTEGER) {
		msgpack_pack_int(pk, value.data.i);
	}
	else if(value.type == METADATA_TYPE_LONG) {
		msgpack_pack_long(pk, value.data.l);
	}
	else if(value.type == METADATA_TYPE_FLOAT) {
		msgpack_pack_float(pk, value.data.f);
	}
	else if(value.type == METADATA_TYPE_DOUBLE) {
		msgpack_pack_double(pk, value.data.d);
	}
	else if(value.type == METADATA_TYPE_BOOLEAN) {
		if(value.data.b) msgpack_pack_true(pk);
			else msgpack_pack_false(pk);
	}
	else if(value.type == METADATA_TYPE_STRING) {
		msgpack_pack_str(pk, strlen(value.data.s));
		msgpack_pack_str_body(pk, value.data.s, strlen(value.data.s));
	}
	else if(value.type == METADATA_TYPE_ARRAY) {
		metadata_convert_msgpack_array(value.data.a, pk);
	}
	else if(value.type == METADATA_TYPE_KEYVALUE) {
		metadata_convert_msgpack_keyvalue(value.data.kv, pk);
	}
}
void metadata_convert_msgpack_keyvalue(metadata_kv *keyvalue, msgpack_packer *pk) {
	metadata_kv *kv = keyvalue;
	int kv_lenth = metadata_keyvalue_get_length(keyvalue);

	msgpack_pack_map(pk, kv_lenth);
	while(kv != NULL) {
		msgpack_pack_str(pk, strlen(kv->key));
		msgpack_pack_str_body(pk, kv->key, strlen(kv->key));
		metadata_convert_msgpack_value(kv->value, pk);
		kv = kv->next;
	}
}
void metadata_convert_msgpack_array(metadata_array *array, msgpack_packer *pk) {
	metadata_array *a = array;
	int array_lenth = metadata_array_get_length(array);

	msgpack_pack_array(pk, array_lenth);
	while(a != NULL) {
		metadata_convert_msgpack_value(a->value, pk);
		a = a->next;
	}
}
void metadata_convert_msgpack_location(client_data *client, msgpack_packer *pk) {
	metadata_kv *el = client->metadata_merged;

	while(el != NULL) {
		if(strcmp(el->key, "location") == 0) {
			metadata_convert_msgpack_value(el->value, pk);
			return;
		}
		el = el->next;
	}
	msgpack_pack_nil(pk);
}

void metadata_add_compare_function(bool (*cmp_function)(int, char *, metadata_kv *, metadata_kv *), int cmp_type) {
	pthread_mutex_lock(&metadata_compare_functions_mutex);
	metadata_compare_functions = ikvl_add(metadata_compare_functions, cmp_type, cmp_function);
	pthread_mutex_unlock(&metadata_compare_functions_mutex);
}

metadata_kv *metadata_get_kv_ptr_by_key(metadata_kv *keyvalue, char *key) {
	metadata_kv *kv = keyvalue;
	while(kv != NULL) {
		if(strcmp(kv->key, key) == 0) {
			if(kv->value.type == METADATA_TYPE_KEYVALUE) return kv->value.data.kv;
				else return NULL; // assuming no duplicate keys
		}
		kv = kv->next;
	}
	return NULL;
}
