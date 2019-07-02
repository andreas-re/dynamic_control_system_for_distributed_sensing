#ifndef _MQTT_HANDLING_H_
#define _MQTT_HANDLING_H_

#include <mosquitto.h>
#include <stdint.h>
#include "main.h"

#define MQTT_TASK_START 0
#define MQTT_TASK_STOP 1

typedef enum {
	MQTT_RET_SUCCESS = 0,
	MQTT_RET_CLIENT_NOT_FOUND = 1,
	MQTT_RET_WRONG_MSGLEN = 2,
	MQTT_RET_REQUEST_OVERFLOW = 3,
	MQTT_RET_SEND_ERROR = 4,
	MQTT_RET_STOP_ID_NOT_FOUND = 5,
	MQTT_RET_STOP_ID_NO_CLIENT_FOUND = 6,
	MQTT_RET_METADATA_INVALID = 7,
	MQTT_RET_NO_MATCHING_CLIENT = 8
} mqtt_ret_code;

typedef struct {
	uint32_t mqtt_id;
	list_element *clients_list;
} mqtt_id_clients;
typedef struct {
	client_data *client;
	uint32_t task_id;
} mqtt_task_client_mapping;

extern int mqtt_port_g;
extern char *mqtt_address_g;
extern pthread_mutex_t mqtt_id_map_mutex;

void *mqtt_thread(void *ptr);
void subscribe_mqtt_client(client_data *client);

void mqtt_send_response(char *client_id, uint32_t request_id, mqtt_ret_code ret_code, const char *env_response, uint16_t env_response_size);
void mqtt_id_map_add(client_data *client, uint32_t mqtt_id, uint32_t task_id);
void mqtt_id_map_remove_client(client_data *client);

void mqtt_send_task_message(client_data *client, char *msg, int msg_size);

#endif /* _MQTT_HANDLING_H_ */
