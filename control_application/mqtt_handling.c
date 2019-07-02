#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mosquitto.h>
#include <arpa/inet.h>
#include <limits.h>
#include "metadata.h"
#include "mqtt_handling.h"
#include "main.h"

// Mosquitto instance
struct mosquitto *mq_g;
list_element *mqtt_id_map = NULL;
pthread_mutex_t mqtt_id_map_mutex;

void mqtt_id_map_add(client_data *client, uint32_t mqtt_id, uint32_t task_id) {
	mqtt_id_clients *mqtt_id_clients;
	mqtt_task_client_mapping *mapping;

	mqtt_id_map = list_mqtt_id_add(mqtt_id_map, mqtt_id); // add mqtt id to list if it doesn't exist yet
	mqtt_id_clients = list_mqtt_id_get(mqtt_id_map, mqtt_id);
	mapping = malloc(sizeof(mqtt_task_client_mapping));
	mapping->client = client;
	mapping->task_id = task_id;

	mqtt_id_clients->clients_list = list_add(mqtt_id_clients->clients_list, mapping);
}

/* returns true, if element was found, else false */
bool mqtt_id_map_remove_single(client_data *client, uint32_t mqtt_id, uint32_t *task_id) {
	list_element *el = mqtt_id_map;
	list_element **cl_start;

	while(el != NULL) {
		if(((mqtt_id_clients *) el->data)->mqtt_id == mqtt_id) break;
		el = el->next;
	}
	if(el != NULL) {
		cl_start = &((mqtt_id_clients *) el->data)->clients_list;
		el = *cl_start;
		while(el != NULL) {
			if(((mqtt_task_client_mapping *) el->data)->client == client) {
				*task_id = ((mqtt_task_client_mapping *) el->data)->task_id;
				*cl_start = list_remove_element(*cl_start, el);
				if(*cl_start == NULL) {
					mqtt_id_map = list_mqtt_id_remove(mqtt_id_map, mqtt_id);
				}
				return true;
			}
			el = el->next;
		}
	}
	return false;
}

void mqtt_id_map_remove_client(client_data *client) {
	list_element *el = mqtt_id_map;
	list_element *cl_el;
	list_element **cl_start;

	while(el != NULL) {
		cl_start = &((mqtt_id_clients *) el->data)->clients_list;
		cl_el = *cl_start;
		while(cl_el != NULL) {
			if(((mqtt_task_client_mapping *) cl_el->data)->client == client) {
				*cl_start = list_remove_element(*cl_start, cl_el);
				if(*cl_start == NULL) {
					mqtt_id_map = list_mqtt_id_remove(mqtt_id_map, ((mqtt_id_clients *) el->data)->mqtt_id);
				}
			}
			cl_el = cl_el->next;
		}
		el = el->next;
	}
}

void subscribe_mqtt_client(client_data *client) {
	int mqtt_topic_length;
	char *mqtt_topic;

	// subscribing to MQTT topic for starting tasks
	// 15: 2 slashes, 13 chars for 'control/start'
	mqtt_topic_length = strlen(deployment_name_g) + strlen(client->id) + 15;
	mqtt_topic = malloc(mqtt_topic_length + 1);
	snprintf(mqtt_topic, (mqtt_topic_length+1), "%s/%s/control/start", deployment_name_g, client->id);
	mosquitto_subscribe(mq_g, NULL, mqtt_topic, 2);
	printf("(client %s) subscribed to topic: '%s'\n", client->id, mqtt_topic);

	// subscribing to MQTT topic for stopping tasks
	// 14: 2 slashes, 12 chars for 'control/stop'
	mqtt_topic_length = strlen(deployment_name_g) + strlen(client->id) + 14;
	mqtt_topic = realloc(mqtt_topic, mqtt_topic_length + 1);
	snprintf(mqtt_topic, (mqtt_topic_length+1), "%s/%s/control/stop", deployment_name_g, client->id);
	mosquitto_subscribe(mq_g, NULL, mqtt_topic, 2);
	printf("(client %s) subscribed to topic: '%s'\n", client->id, mqtt_topic);

	free(mqtt_topic);
}

void mqtt_subscribe_task_control_metadata() {
	int mqtt_topic_length;
	char *mqtt_topic;

	// subscribing to MQTT topic for starting tasks
	mqtt_topic_length = strlen(deployment_name_g) + 14;
	mqtt_topic = malloc(mqtt_topic_length + 1);
	snprintf(mqtt_topic, (mqtt_topic_length+1), "%s/control/start", deployment_name_g);
	mosquitto_subscribe(mq_g, NULL, mqtt_topic, 2);
	printf("subscribed to topic: '%s'\n", mqtt_topic);

	// subscribing to MQTT topic for stopping tasks
	mqtt_topic_length = strlen(deployment_name_g) + 13;
	mqtt_topic = realloc(mqtt_topic, mqtt_topic_length + 1);
	snprintf(mqtt_topic, (mqtt_topic_length+1), "%s/control/stop", deployment_name_g);
	mosquitto_subscribe(mq_g, NULL, mqtt_topic, 2);
	printf("subscribed to topic: '%s'\n", mqtt_topic);

	free(mqtt_topic);
}

void mqtt_send_response(char *client_id, uint32_t request_id, mqtt_ret_code ret_code, const char *env_response, uint16_t env_response_size) {
	int msg_size;
	int topic_size;
	char *msg;
	char *topic;
	uint16_t ers_nbo;
	uint32_t req_id_nbo;
	//unsigned long long t;

	// 7 = 4 for request ID + 1 for return code + 2 for length of execution environment response
	msg_size = 7 + env_response_size;

	if(client_id != NULL) {
		// 16 = 1 for slash and 15 for '/control/result'
		topic_size = strlen(deployment_name_g) + strlen(client_id) + 16;
		topic = malloc(topic_size+1); // +1 for null byte
		snprintf(topic, (topic_size+1), "%s/%s/control/result", deployment_name_g, client_id);
	}
		else {
			// 15 for '/control/result'
			topic_size = strlen(deployment_name_g) + 15;
			topic = malloc(topic_size+1); // +1 for null byte
			snprintf(topic, (topic_size+1), "%s/control/result", deployment_name_g);
		}

	// assuming little-endian for copying integers to the char array
	msg = malloc(msg_size);
	req_id_nbo = htonl(request_id);
	memcpy(msg, &req_id_nbo, 4);
	memcpy(&msg[4], &ret_code, 1);
	ers_nbo = htons(env_response_size);
	memcpy(&msg[5], &ers_nbo, 2);
	memcpy(&msg[7], env_response, env_response_size);

	
	//t = get_time();
	mosquitto_publish(mq_g, NULL, topic, msg_size, msg, 2, false);
	if(client_id != NULL) {
		//printf("(client %s) started publishing MQTT task response [%llu]\n", client_id, t);
		printf("(client %s) started publishing MQTT task response\n", client_id);
	}
		else {
			//printf("started publishing MQTT task response [%llu]\n", t);
			printf("started publishing MQTT task response\n");
		}

	free(topic);
	free(msg);
}

/* expecting to get a message with a topic of the format <deployment name>/+/+/start or
 * <deployment name>/+/+/stop
 * type is 0 for start and 1 for stop
 */
void mqtt_control_task_single(const struct mosquitto_message *message, int type) {
	char *start_pos;
	char *end_pos;
	char *client_id;
	int client_id_length;
	client_data *client;
	int i;
	uint32_t mqtt_id_nbo;
	uint32_t mqtt_id;
	uint16_t msg_len_nbo;
	uint16_t msg_len;
	uint32_t stop_id;
	uint32_t stop_id_nbo;
	uint32_t task_id;

	if(message->payloadlen >= 4) {
		// extract MQTT request id
		memcpy(&mqtt_id_nbo, message->payload, 4);
		mqtt_id = ntohl(mqtt_id_nbo);

		// check client id
		start_pos = strchr(message->topic, '/');
		if(start_pos == NULL) {
			fprintf(stderr, "error while parsing MQTT topic (p0)\n");
			return;
		}
		start_pos += 1; // first character after slash
		end_pos = strchr(start_pos+1, '/');
		if(end_pos == NULL) {
			fprintf(stderr, "error while parsing MQTT topic (p1)\n");
			return;
		}
		end_pos -= 1; // last character before slash
		client_id_length = end_pos - start_pos + 1;

		client_id = malloc(client_id_length+1);
		memcpy(client_id, start_pos, client_id_length);
		client_id[client_id_length] = 0x0;

		pthread_mutex_lock(&mqtt_access_clients);
		client = get_client_by_id(client_id);

		if(client == NULL) {
			pthread_mutex_unlock(&mqtt_access_clients);
			fprintf(stderr, "client with ID '%s' not found\n", client_id);
			mqtt_send_response(client_id, mqtt_id, MQTT_RET_CLIENT_NOT_FOUND, NULL, 0);
		}
			else {
				if(type == MQTT_TASK_START) {
					// extract msg len
					memcpy(&msg_len_nbo, message->payload+4, 2);
					msg_len = ntohs(msg_len_nbo);
					if(message->payloadlen == 6+msg_len) {
						send_start_task_client(client, mqtt_id, message->payload+6, msg_len);
					}
						else {
							fprintf(stderr, "(client %s) unexpected MQTT message payloadlen: got %i, expected %i (p0)\n", client_id, message->payloadlen, 6+msg_len);
							mqtt_send_response(client_id, mqtt_id, MQTT_RET_WRONG_MSGLEN, NULL, 0);
						}
				}
				else if(type == MQTT_TASK_STOP) {
					if(message->payloadlen == 8) {
						memcpy(&stop_id_nbo, message->payload+4, 4);
						stop_id = ntohl(stop_id_nbo);
						pthread_mutex_lock(&mqtt_id_map_mutex);
						if(mqtt_id_map_remove_single(client, stop_id, &task_id)) {
							send_stop_task(client, mqtt_id, task_id);
						}
							else {
								fprintf(stderr, "(client %s) MQTT request id for stopping task not found\n", client_id);
								mqtt_send_response(client_id, mqtt_id, MQTT_RET_STOP_ID_NOT_FOUND, NULL, 0);
							}
						pthread_mutex_unlock(&mqtt_id_map_mutex);
					}
						else {
							fprintf(stderr, "(client %s) unexpected MQTT message payloadlen: got %i, expected %i (p1)\n", client_id, message->payloadlen, 4);
							mqtt_send_response(client_id, mqtt_id, MQTT_RET_WRONG_MSGLEN, NULL, 0);
						}
				}
					else fprintf(stderr, "unknown type for mqtt_control_task_single: %i\n", type);
				pthread_mutex_unlock(&mqtt_access_clients);
			}

		free(client_id);
	}
		else {
			fprintf(stderr, "received MQTT message (topic: '%s') with too small payloadlen: %i (p0)\n", message->topic, message->payloadlen);
		}
}


/* evaluate a request for starting a task 
 * metadata has to be compared first and then the task is sent to all
 * clients with matching metadata
 */
void mqtt_start_task_metadata(const struct mosquitto_message *message) {
	uint32_t mqtt_id_nbo;
	uint32_t mqtt_id;
	uint16_t task_len_nbo;
	uint16_t task_len;
	uint16_t key_len_nbo;
	uint16_t key_len;
	uint16_t value_len_nbo;
	uint16_t value_len;
	uint32_t task_id;
	uint32_t task_id_nbo;
	const int task_pos = 6;
	char *key_buffer;
	int key_pos;
	int value_pos;
	metadata_value mval;
	int cmp_type;
	list_element *clients_matched = NULL;
	
	if(message->payloadlen >= 4) {
		// extract mqtt request id
		memcpy(&mqtt_id_nbo, message->payload, 4);
		mqtt_id = ntohl(mqtt_id_nbo);

		// length of message: 12 + length of task + length of metadata key to compare +
		// length of metadata value to compare

		if(message->payloadlen < 12) {
			fprintf(stderr, "MQTT message payloadlen too small (p0)\n");
			mqtt_send_response(NULL, mqtt_id, MQTT_RET_WRONG_MSGLEN, NULL, 0);
			return;
		}
		// extract msg len
		memcpy(&task_len_nbo, message->payload+4, 2);
		task_len = ntohs(task_len_nbo);
		if(message->payloadlen < 12+task_len) {
			fprintf(stderr, "MQTT message payloadlen too small (p1)\n");
			mqtt_send_response(NULL, mqtt_id, MQTT_RET_WRONG_MSGLEN, NULL, 0);
			return;
		}
		key_pos = 6+task_len+2;

		memcpy(&key_len_nbo, message->payload+6+task_len, 2);
		key_len = ntohs(key_len_nbo);
		if(message->payloadlen < 12+task_len+key_len) {
			fprintf(stderr, "MQTT message payload has wrong size\n");
			mqtt_send_response(NULL, mqtt_id, MQTT_RET_WRONG_MSGLEN, NULL, 0);
			return;
		}
		value_pos = 12+task_len+key_len;
		mval.type = *((unsigned char*) message->payload+8+task_len+key_len);
		cmp_type = *((unsigned char*) message->payload+9+task_len+key_len);
		memcpy(&value_len_nbo, message->payload+10+task_len+key_len, 2);
		value_len = ntohs(value_len_nbo);

		if(!metadata_check_insert_svalue(&mval, message->payload+value_pos, value_len)) {
			fprintf(stderr, "MQTT message metadata payload invalid\n");
			mqtt_send_response(NULL, mqtt_id, MQTT_RET_METADATA_INVALID, NULL, 0);
			return;
		}

		key_buffer = malloc(key_len+1);
		memcpy(key_buffer, message->payload+key_pos, key_len);
		key_buffer[key_len] = 0x0;
		pthread_mutex_lock(&mqtt_access_clients);
		clients_matched = metadata_get_matching_clients(key_buffer, mval, cmp_type);
		send_start_task_list(clients_matched, mqtt_id, message->payload+task_pos, task_len);
		pthread_mutex_unlock(&mqtt_access_clients);

		free(key_buffer);
		clients_matched = list_free(clients_matched);
		metadata_free_value(&mval);
	}
		else {
			fprintf(stderr, "received MQTT message (topic: '%s') with too small payloadlen: %i (p1)\n", message->topic, message->payloadlen);
		}
}

void mqtt_stop_task_mqttid(const struct mosquitto_message *message) {
	uint32_t req_id_nbo;
	uint32_t req_id;
	uint32_t mqtt_id;
	uint32_t mqtt_id_nbo;
	list_element *cl = NULL;
	mqtt_task_client_mapping *mapping;

	if(message->payloadlen >= 4) {
		// extract MQTT request id
		memcpy(&req_id_nbo, message->payload, 4);
		req_id = ntohl(req_id_nbo);

		if(message->payloadlen == 8) {
			memcpy(&mqtt_id_nbo, message->payload+4, 4);
			mqtt_id = ntohl(mqtt_id_nbo);
			pthread_mutex_lock(&mqtt_access_clients);
			pthread_mutex_lock(&mqtt_id_map_mutex);
			cl = list_mqtt_get_clients(mqtt_id_map, mqtt_id);
			if(cl != NULL) {
				while(cl != NULL) {
					mapping = (mqtt_task_client_mapping *) cl->data;
					send_stop_task(mapping->client, mqtt_id, mapping->task_id);
					cl = cl->next;
				}
				mqtt_id_map = list_mqtt_id_remove(mqtt_id_map, mqtt_id);
				mqtt_send_response(NULL, mqtt_id, MQTT_RET_SUCCESS, NULL, 0);
			}
				else {
					fprintf(stderr, "No clients with MQTT request ID %u found\n", mqtt_id);
					mqtt_send_response(NULL, mqtt_id, MQTT_RET_STOP_ID_NO_CLIENT_FOUND, NULL, 0);
				}
			pthread_mutex_unlock(&mqtt_id_map_mutex);
			pthread_mutex_unlock(&mqtt_access_clients);
		}
			else {
				fprintf(stderr, "unexpected MQTT message payloadlen (p2)\n");
				mqtt_send_response(NULL, mqtt_id, MQTT_RET_WRONG_MSGLEN, NULL, 0);
			}
	}
		else {
			fprintf(stderr, "received MQTT message (topic: '%s') with too small payloadlen: %i (p2)\n", message->topic, message->payloadlen);
		}
}

void mqtt_send_task_message(client_data *client, char *msg, int msg_size) {
	char *topic;
	int topic_length;
	//long long unsigned t;

	topic_length = strlen(deployment_name_g)+strlen(client->id)+14;
	topic = malloc(topic_length+1);
	snprintf(topic, topic_length+1, "%s/%s/task_message", deployment_name_g, client->id);
	//t = get_time();
	mosquitto_publish(mq_g, NULL, topic, msg_size, msg, 2, false);
	//printf("(client %s) started publishing MQTT task message [%llu]\n", client->id, t);
	printf("(client %s) started publishing MQTT task message\n", client->id);
}

void mqtt_message_callback(struct mosquitto *mq, void *userdata, const struct mosquitto_message *message) {
	bool match;
	char *check_topic;
	int check_topic_length;
	//long long unsigned t;

	//t = get_time();
	//printf("MQTT message callback [%llu]\n", t);
	printf("MQTT message callback\n");
	if(message->payloadlen) {
		// check if message is for this deployment
		check_topic_length = strlen(deployment_name_g) + 2;
		check_topic = malloc(check_topic_length + 1);
		snprintf(check_topic, (check_topic_length+1), "%s/#", deployment_name_g);
		mosquitto_topic_matches_sub(check_topic, message->topic, &match);
		free(check_topic);
		if(match) {
			// check if message is for starting a task on a specified client
			check_topic_length = strlen(deployment_name_g) + 16;
			check_topic = malloc(check_topic_length + 1);
			snprintf(check_topic, (check_topic_length+1), "%s/+/control/start", deployment_name_g);
			mosquitto_topic_matches_sub(check_topic, message->topic, &match);
			free(check_topic);
			if(match) {
				mqtt_control_task_single(message, MQTT_TASK_START);
				return;
			}
			// check if message is for stopping a task on a specified client
			check_topic_length = strlen(deployment_name_g) + 15;
			check_topic = malloc(check_topic_length + 1);
			snprintf(check_topic, (check_topic_length+1), "%s/+/control/stop", deployment_name_g);
			mosquitto_topic_matches_sub(check_topic, message->topic, &match);
			free(check_topic);
			if(match) {
				mqtt_control_task_single(message, MQTT_TASK_STOP);
				return;
			}

			// check if message is for starting a task on all clients with matching metadata
			check_topic_length = strlen(deployment_name_g) + 14;
			check_topic = malloc(check_topic_length + 1);
			snprintf(check_topic, (check_topic_length+1), "%s/control/start", deployment_name_g);
			mosquitto_topic_matches_sub(check_topic, message->topic, &match);
			free(check_topic);
			if(match) {
				mqtt_start_task_metadata(message);
				return;
			}
			// check if message is for stopping a task by a data ID
			check_topic_length = strlen(deployment_name_g) + 13;
			check_topic = malloc(check_topic_length + 1);
			snprintf(check_topic, (check_topic_length+1), "%s/control/stop", deployment_name_g);
			mosquitto_topic_matches_sub(check_topic, message->topic, &match);
			free(check_topic);
			if(match) {
				mqtt_stop_task_mqttid(message);
				return;
			}
		}
	}
}

void mqtt_publish_callback(struct mosquitto *mq, void *userdata, int mid) {
	//unsigned long long t = get_time();
	//printf("MQTT publish callback: (%i) [%llu]\n", mid, t);
	printf("MQTT publish callback (mid: %i)\n", mid);
}

void mqtt_connect_callback(struct mosquitto *mq, void *userdata, int result) {
	int i;
	if(result == 0) {
		printf("MQTT connected\n");
		mqtt_subscribe_task_control_metadata();
	}
		else{
			fprintf(stderr, "MQTT connect failed\n");
		}
}

void *mqtt_thread(void *ptr) {
	pthread_mutex_init(&mqtt_id_map_mutex, NULL);

	mosquitto_lib_init();
	mq_g = mosquitto_new(NULL, true, NULL); // clean session
	if(mq_g == NULL) {
		perror("error creating new mosquitto client instance");
		return (void *) 1;
	}
	// enable threaded operation
	mosquitto_threaded_set(mq_g, true);
	mosquitto_connect_callback_set(mq_g, mqtt_connect_callback);
	mosquitto_publish_callback_set(mq_g, mqtt_publish_callback);
	mosquitto_message_callback_set(mq_g, mqtt_message_callback);

	if(mosquitto_connect(mq_g, mqtt_address_g, mqtt_port_g, 60) != MOSQ_ERR_SUCCESS){
		perror("error connecting mqtt client");
		return (void *) 1;
	}

	mosquitto_loop_forever(mq_g, -1, 1);

	mosquitto_destroy(mq_g);
	mosquitto_lib_cleanup();
}
