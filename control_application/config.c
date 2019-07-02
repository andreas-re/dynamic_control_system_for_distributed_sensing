#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "jsmn.h"
#include "config.h"
#include "mqtt_handling.h"
#include "metadata.h"

int evaluate_jc_keyvalue(jsmntok_t *tokens, char *json, int cur_pos, int token_count, metadata_kv *keyvalue);
int evaluate_jc_array(jsmntok_t *tokens, char *json, int cur_pos, int token_count, metadata_array *array);
int evaluate_jc_value(jsmntok_t *tokens, char *json, int cur_pos, int token_count, metadata_value *value);

metadata_kv *config_metadata_g = NULL;

bool config_json_check_key(jsmntok_t token, char *json, char *key) {
	if(token.type != JSMN_STRING) return false;
	if(strncmp(key, json+token.start, token.end-token.start) == 0) return true;
		else return false;
}

char *config_json_copy_str(jsmntok_t token, char *json) {
	char *value = NULL;

	if(token.type != JSMN_STRING && token.type != JSMN_PRIMITIVE) return NULL;
	
	value = malloc(token.end-token.start+1);
	memcpy(value, json+token.start, token.end-token.start);
	value[token.end-token.start] = 0x0;

	return value;
}

int evaluate_jc_value(jsmntok_t *tokens, char *json, int cur_pos, int token_count, metadata_value *value) {
	int i = cur_pos;
	char first_char;
	char *buffer;

	if(i >= token_count) {
		fprintf(stderr, "Error parsing JSON configuration: not enough tokens (p0)\n");
		return (-1);
	}
	if(tokens[i].type == JSMN_PRIMITIVE) {
		first_char = *(json+tokens[i].start);
		if(first_char == 't' || first_char == 'f') {
			value->type = METADATA_TYPE_BOOLEAN;
			if(first_char == 't') value->data.b = true;
				else value->data.b = false;
		}
		else if(first_char == '-' || (first_char >= 48 && first_char <= 57)) {
			buffer = malloc(tokens[i].end-tokens[i].start+1);
			memcpy(buffer, json+tokens[i].start, tokens[i].end-tokens[i].start);
			if(strchr(buffer, '.') == NULL) {
				// int or long
				long num = strtol(buffer, NULL, 10);
				if(num >= INT_MIN && num <= INT_MAX) {
					value->type = METADATA_TYPE_LONG;
					value->data.l = num;
				}
					else {
						value->type = METADATA_TYPE_INTEGER;
						value->data.i = (int) num;
					}
			}
				else {
					// double
					value->type = METADATA_TYPE_DOUBLE;
					value->data.d = strtod(buffer, NULL);
				}
			free(buffer);
		}
			else return (-1);
		i++;
	}
	else if(tokens[i].type == JSMN_STRING) {
		value->type = METADATA_TYPE_STRING;
		value->data.s = malloc(tokens[i].end-tokens[i].start+1);
		memcpy(value->data.s, json+tokens[i].start, tokens[i].end-tokens[i].start);
		value->data.s[tokens[i].end-tokens[i].start] = 0x0;

		i++;
	}
	else if(tokens[i].type == JSMN_ARRAY) {
		value->type = METADATA_TYPE_ARRAY;
		if(tokens[i].size == 0) {
			value->data.a = NULL;
			i++;
		}
			else {
				value->data.a = malloc(sizeof(metadata_array));
				i = evaluate_jc_array(tokens, json, i, token_count, value->data.a);
				if(i < 0) return (-1);
			}
	}
	else if(tokens[i].type == JSMN_OBJECT) {
		value->type = METADATA_TYPE_KEYVALUE;
		if(tokens[i].size == 0) {
			value->data.kv = NULL;
			i++;
		}
			else {
				value->data.kv = malloc(sizeof(metadata_kv));
				i = evaluate_jc_keyvalue(tokens, json, i, token_count, value->data.kv);
				if(i < 0) return (-1);
			}
	}
	return i;
}

int evaluate_jc_array(jsmntok_t *tokens, char *json, int cur_pos, int token_count, metadata_array *array) {
	int an;
	int i = cur_pos;
	int a_size;
	metadata_array *a = array;

	if(i >= token_count) {
		fprintf(stderr, "Error parsing JSON configuration: not enough tokens (p1)\n");
		return (-1);
	}
	a_size = tokens[i].size;

	i++;
	for(an = 0; an < a_size; an++) {
		i = evaluate_jc_value(tokens, json, i, token_count, &a->value);
		if(i < 0) return (-1);

		if(an < a_size-1) {
			a->next = malloc(sizeof(metadata_array));
			a = a->next;
		}
	}
	a->next = NULL;
	return i;
}

int evaluate_jc_keyvalue(jsmntok_t *tokens, char *json, int cur_pos, int token_count, metadata_kv *keyvalue) {
	int i = cur_pos;
	int en;
	int kv_size;
	metadata_kv *kv = keyvalue;

	if(i >= token_count) {
		fprintf(stderr, "Error parsing JSON configuration: not enough tokens (p2)\n");
		return (-1);
	}
	kv_size = tokens[i].size;
	i++;
	for(en = 0; en < kv_size; en++) {
		// key
		if(tokens[i].type != JSMN_STRING) return (-1);
		kv->key = malloc(tokens[i].end-tokens[i].start+1);
		memcpy(kv->key, json+tokens[i].start, tokens[i].end-tokens[i].start);
		kv->key[tokens[i].end-tokens[i].start] = 0x0;

		i++;
		// value
		i = evaluate_jc_value(tokens, json, i, token_count, &kv->value);
		if(i < 0) return (-1);
		if(en < kv_size-1) {
			kv->next = malloc(sizeof(metadata_kv));
			kv = kv->next;
		}
	}
	kv->next = NULL;
	return i;
}

int evaluate_json_config_metadata(jsmntok_t *tokens, char *json, int cur_pos, int token_count) {
	int i = cur_pos;
	int cn, en;
	int client_num;
	int kv_size;
	char *key;
	metadata_value *metadata;
	metadata_kv *mkv;
	metadata_kv *cmg = NULL;

	if(i >= token_count) {
		fprintf(stderr, "Error parsing JSON configuration: not enough tokens (p3)\n");
		return (-1);
	}
	if(tokens[i].type != JSMN_OBJECT) return (-1);
	client_num = tokens[i].size;

	i++;
	if(i >= token_count) {
		fprintf(stderr, "Error parsing JSON configuration: not enough tokens (p4)\n");
		return (-1);
	}
	if(client_num > 0) {
		config_metadata_g = malloc(sizeof(metadata_kv));
		cmg = config_metadata_g;
	}
	for(cn = 0; cn < client_num; cn++) {
		metadata = malloc(sizeof(metadata_value));
		metadata->type = METADATA_TYPE_KEYVALUE;
		metadata->data.kv = malloc(sizeof(metadata_kv));
		mkv = metadata->data.kv;
		mkv->next = NULL;

		if(i >= token_count) {
			fprintf(stderr, "Error parsing JSON configuration: not enough tokens (p5)\n");
			return (-1);
		}
		if(tokens[i].type != JSMN_STRING) return (-1);
		key = malloc(tokens[i].end-tokens[i].start+1);
		memcpy(key, json+tokens[i].start, tokens[i].end-tokens[i].start);
		key[tokens[i].end-tokens[i].start] = 0x0;
		printf("client name: %s\n", key);
		
		// now extract actual metadata
		i++;
		if(i >= token_count) {
			fprintf(stderr, "Error parsing JSON configuration: not enough tokens (p6)\n");
			return (-1);
		}
		if(tokens[i].type != JSMN_OBJECT) return (-1);
		i = evaluate_jc_keyvalue(tokens, json, i, token_count, mkv);
		if(i < 0) return (-1);
		
		cmg->key = key;
		cmg->value = *metadata;

		if(cn < client_num-1) {
			cmg->next = malloc(sizeof(metadata_kv));
			cmg = cmg->next;
		}
			else cmg->next = NULL;
	}
	metadata_print_kv(config_metadata_g);
	return i+1;
}

// evaluate the configuration json string
int evaluate_json_configuration(char *json) {
	jsmn_parser parser;
	int token_count;
	int i;
	jsmntok_t *tokens;
	char *deployment_name = NULL;
	char *mqtt_server = NULL;
	char *mqtt_port_buffer = NULL;
	int mqtt_port = -1;
	char *client_socket_port_buffer = NULL;
	int client_socket_port = -1;
	char *endptr;

	jsmn_init(&parser);
	token_count = jsmn_parse(&parser, json, strlen(json), NULL, 0);
	tokens = malloc(token_count * sizeof(jsmntok_t));
	jsmn_init(&parser);
	token_count = jsmn_parse(&parser, json, strlen(json), tokens, token_count);

	if(token_count < 0) {
		if(token_count == JSMN_ERROR_INVAL || token_count == JSMN_ERROR_PART) fprintf(stderr, "Error parsing JSON configuration: invalid JSON data\n");
		else if(token_count == JSMN_ERROR_NOMEM) fprintf(stderr, "Error parsing JSON configuration: internal error\n");
			else fprintf(stderr, "Error parsing JSON configuration: undefined error\n");
		free(tokens);
		return 1;
	}

	if(token_count < 1 || tokens[0].type != JSMN_OBJECT) {
		fprintf(stderr, "Error parsing JSON configuration: expected object\n");
		return 1;
	}
	for(i = 1; i < token_count; i++) {
		if(config_json_check_key(tokens[i], json, "deployment_name")) {
			deployment_name = config_json_copy_str(tokens[i+1], json);
			if(deployment_name == NULL || tokens[i+1].type != JSMN_STRING) {
				fprintf(stderr, "Error parsing JSON configuration: expected string (p0)\n");
				return 1;
			}
			deployment_name_g = deployment_name;
			printf("CONFIG: deployment_name=%s\n", deployment_name_g);
			i++;
		}
		else if(config_json_check_key(tokens[i], json, "mqtt_server")) {
			mqtt_server = config_json_copy_str(tokens[i+1], json);
			if(mqtt_server == NULL || tokens[i+1].type != JSMN_STRING) {
				fprintf(stderr, "Error parsing JSON configuration: expected string (p1)\n");
				return 1;
			}
			mqtt_address_g = mqtt_server;
			printf("CONFIG: mqtt_server=%s\n", mqtt_address_g);
			i++;
		}
		else if(config_json_check_key(tokens[i], json, "mqtt_port")) {
			mqtt_port_buffer = config_json_copy_str(tokens[i+1], json);
			if(mqtt_port_buffer == NULL || tokens[i+1].type != JSMN_PRIMITIVE) {
				fprintf(stderr, "Error parsing JSON configuration: expected jsmn primitive (p0)\n");
				return 1;
			}
			mqtt_port = (int) strtol(mqtt_port_buffer, &endptr, 10);
			if(endptr == mqtt_port_buffer || *endptr != 0x0) {
				fprintf(stderr, "Error parsing JSON configuration: expected integer (p0)\n");
				return 1;
			}
			free(mqtt_port_buffer);
			mqtt_port_g = mqtt_port;
			printf("CONFIG: mqtt_port=%i\n", mqtt_port);
			i++;
		}
		else if(config_json_check_key(tokens[i], json, "client_socket_port")) {
			client_socket_port_buffer = config_json_copy_str(tokens[i+1], json);
			if(client_socket_port_buffer == NULL || tokens[i+1].type != JSMN_PRIMITIVE) {
				fprintf(stderr, "Error parsing JSON configuration: expected jsmn primitive (p1)\n");
				return 1;
			}
			client_socket_port = (int) strtol(client_socket_port_buffer, &endptr, 10);
			if(endptr == client_socket_port_buffer || *endptr != 0x0) {
				fprintf(stderr, "Error parsing JSON configuration: expected integer (p1)\n");
				return 1;
			}
			free(client_socket_port_buffer);
			socket_port_g = client_socket_port;
			printf("CONFIG: client_socket_port=%i\n", socket_port_g);
			i++;
		}
		else if(config_json_check_key(tokens[i], json, "clients")) {
			i++;
			i = evaluate_json_config_metadata(tokens, json, i, token_count);
			if(i < 0) {
				fprintf(stderr, "Error parsing JSON configuration: metadata configuration corrupt\n");
				return 1;
			}
		}
			else {
				fprintf(stderr, "Error parsing JSON configuration: unknown key\n");
				return 1;
			}
	}
	free(tokens);

	if(deployment_name == NULL) {
		fprintf(stderr, "Error parsing JSON configuration: deployment_name missing\n");
		return 1;
	}
	if(mqtt_server == NULL) {
		fprintf(stderr, "Error parsing JSON configuration: mqtt_server missing\n");
		return 1;
	}
	if(mqtt_port < 0) {
		fprintf(stderr, "Error parsing JSON configuration: mqtt_port missing or invalid\n");
		return 1;
	}
	if(client_socket_port < 0) {
		fprintf(stderr, "Error parsing JSON configuration: client_socket_port missing or invalid\n");
		return 1;
	}
	return 0;
}

// read configuration file
int read_configuration() {
	FILE *fp;
	long file_size;
	char *json_string;
	int ev_ret;

	fp = fopen("configuration.json", "r");
	if(fp == NULL) {
		perror("Error opening configuration file");
		return 1;
	}
	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	if(file_size == -1L) {
		perror("Error determining configuration file size");
		return 1;
	}
	rewind(fp);
	json_string = malloc(file_size+1);
	if(fread(json_string, 1, file_size, fp) != file_size) {
		fprintf(stderr, "Error reading configuration file\n");
		return 1;
	}
	fclose(fp);
	json_string[file_size] = 0x0;

	if(strlen(json_string) != file_size) {
		fprintf(stderr, "Error reading configuration file: too few characters\n");
		return 1;
	}

	ev_ret = evaluate_json_configuration(json_string);
	free(json_string);

	if(ev_ret == 0) return 0;
		else return 1;
}
