#ifndef _MAIN_H_
#define _MAIN_H_

typedef struct client_data client_data;

#include <pthread.h>
#include <stdint.h>
#include "ikvl.h"
#include "metadata.h"

typedef enum {
	INITIAL = 0,
	GOT_PROTOCOL_VERSION = 1,
	GOT_CLIENT_ID = 2,
	INITIALIZED = 3
} client_status_t;

struct client_data {
	int socket_fd;
	pthread_t thread_fd;
	client_status_t state;
	char *id;
	metadata_kv *metadata_config;
	metadata_kv *metadata_merged;
	pthread_mutex_t metadata_merged_mutex;
};



extern char *deployment_name_g;
extern int socket_port_g;
extern list_element *clients_list;
extern pthread_mutex_t access_client_list;
extern pthread_mutex_t mqtt_access_clients;

client_data *get_client_by_id(char *id);
ssize_t send_start_task_client(client_data *client, uint32_t mqtt_id, char *task, size_t task_size);
void send_start_task_list(list_element *clients, uint32_t mqtt_id, char *task, size_t task_size);
ssize_t send_stop_task(client_data *client, uint32_t mqtt_id, uint32_t task_id);

unsigned long long get_time();

#endif /* _MAIN_H_ */
