#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <msgpack.h>
#include <stdbool.h>
#include "main.h"
#include "mqtt_handling.h"
#include "ikvl.h"
#include "list.h"
#include "config.h"
#include "metadata.h"
#include "compare_complex_metadata.h"


// request types
#define REQUEST_TYPE_PROTOCOL_VERSION 0	// client -> server
#define REQUEST_TYPE_CLIENT_ID 1		// client -> server
#define REQUEST_TYPE_METADATA 2			// client -> server
#define REQUEST_TYPE_START_TASK 3		// server -> client
#define REQUEST_TYPE_STOP_TASK 4		// server -> client
#define REQUEST_TYPE_CONTROL_RESULT 5	// client -> server
#define REQUEST_TYPE_MESSAGE 6			// client -> server

#define PROCOL_VERSION "1.0"

typedef struct {
	int state;
	struct msgpack_object *array_objs;
	int array_size;
	int offset;
	bool is_array;
} msg_ptr_len_t;


char *deployment_name_g;
char *mqtt_address_g;
int socket_port_g;
int mqtt_port_g;

client_data clients[100]; // will be with dynamic memory allocation in the future

list_element *clients_list = NULL;
pthread_mutex_t access_client_list;
pthread_mutex_t mqtt_access_clients;


float binary_to_float(char *bin) {
	char fl[4] = {0x0,0x0,0x0,0x0};
	float ret;
	int mask = 128;
	int ccount = 3;
	
	for(int i = 0; i < 32; i++) {
		if(bin[i] == '1') {
			fl[ccount] += mask;
		}
		mask >>= 1;
		if(i == 7 || i == 15 || i == 23) {
			ccount--;
			mask = 128;
		}
	}

	memcpy(&ret, fl, 4);
	return ret;
}

unsigned long long get_time() {
	struct timespec t;
	unsigned long long ret_time;
	clock_gettime(CLOCK_REALTIME, &t);

	ret_time = ((long long unsigned) t.tv_sec) * 1000000 + ((long long unsigned) t.tv_nsec) / 1000;
	return ret_time;
}

// print a result received from the NDEE platform
void decode_NDEE_result(client_data *client, const char *msg, size_t msg_size) {
	char buffer[33];
	if(msg_size != 14*8 && msg_size != 6*8) {
		printf("(client %s) unknown msg size: %i\n", client->id, msg_size);
	}
	memcpy(buffer, msg, 8);
	buffer[8] = 0x0;
	printf("(client %s) status: %li\n", client->id, strtol(buffer, NULL, 2));
	memcpy(buffer, msg+(1*8), 8);
	printf("(client %s) requestid: %li\n", client->id, strtol(buffer, NULL, 2));
	memcpy(buffer, msg+(2*8), 8*4);
	buffer[8*4] = 0x0;
	printf("(client %s) timestamp: %li\n", client->id, strtol(buffer, NULL, 2));
	
	if(msg_size == 14*8) {
		memcpy(buffer, msg+(6*8), 8);
		buffer[8] = 0x0;
		printf("(client %s) measurand: %li\n", client->id, strtol(buffer, NULL, 2));
		memcpy(buffer, msg+(7*8), 8);
		printf("(client %s) device: %li\n", client->id, strtol(buffer, NULL, 2));
		memcpy(buffer, msg+(8*8), 8);
		printf("(client %s) unit: %li\n", client->id, strtol(buffer, NULL, 2));
		memcpy(buffer, msg+(9*8), 8);
		printf("(client %s) type: %li\n", client->id, strtol(buffer, NULL, 2));
		memcpy(buffer, msg+(10*8), 8*4);
		buffer[8*4] = 0x0;
		printf("(client %s) value as int: %li\n", client->id, strtol(buffer, NULL, 2));
		float val = binary_to_float(buffer);
		printf("(client %s) value as float: %f\n", client->id, val);
	}
}

// find the client with the given ID in the global variable clients_list
client_data *get_client_by_id(char *id) {
	int i;
	client_data *client;

	pthread_mutex_lock(&access_client_list);
	client = list_get_data_by_clientid(clients_list, id);
	pthread_mutex_unlock(&access_client_list);

	return client;
}

// evaluate new metadata in msgpack format and combine this
// the metadata with the metadata from the configuration file
int evaluate_msgpack_metadata(client_data *client, msgpack_object obj) {
	metadata_kv *metadata_client;
	if(obj.type != MSGPACK_OBJECT_MAP) {
		printf("(client %s) received metadata has wrong format: %i\n", client->id, obj.type);
		return (-1);
	}
	if(obj.via.map.size > 0) {
		metadata_client = malloc(sizeof(metadata_kv));
		// convert metadata in msgpack format to internal structure
		metadata_evaluate_msgpack_keyvalue(obj.via.map, metadata_client);
		pthread_mutex_lock(&client->metadata_merged_mutex);
		metadata_free_kv(client->metadata_merged);
		client->metadata_merged = metadata_merge(client->metadata_config, metadata_client);
		printf("(client %s) merged metadata:\n", client->id);
		metadata_print_kv(client->metadata_merged);
		pthread_mutex_unlock(&client->metadata_merged_mutex);
	}

}

/*
 * send a start task request to a client
 * the function returns the result of the socket write function
 */
ssize_t send_start_task_client(client_data *client, uint32_t mqtt_id, char *task, size_t task_size) {
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	ssize_t ret;
	int req_id;
	//long long unsigned t;

	// build message in msgpack format
	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	
	msgpack_pack_array(&pk, 3);
	msgpack_pack_int(&pk, REQUEST_TYPE_START_TASK);
	msgpack_pack_uint32(&pk, mqtt_id);
	msgpack_pack_str(&pk, task_size);
	msgpack_pack_str_body(&pk, task, task_size);

	// send message
	ret = write(client->socket_fd, sbuf.data, sbuf.size);
	//t = get_time();
	//printf("(client %s) sent start task request [%llu]\n", client->id, t);
	printf("(client %s) sent start task request\n", client->id);
	if(ret != sbuf.size) {
		printf("(client %s) failed sending start task request: %i\n", client->id, ret);
		mqtt_send_response(client->id, mqtt_id, MQTT_RET_SEND_ERROR, NULL, 0);
	}
	msgpack_sbuffer_destroy(&sbuf);

	return ret;
}

// send a start task request to all clients in the given clients list
void send_start_task_list(list_element *clients, uint32_t mqtt_id, char *task, size_t task_size) {
	list_element *cl = clients;

	if(cl == NULL) {
		// empty list
		mqtt_send_response(NULL, mqtt_id, MQTT_RET_NO_MATCHING_CLIENT, NULL, 0);
		return;
	}
	while(cl != NULL) {
		send_start_task_client((client_data *)cl->data, mqtt_id, task, task_size);
		cl = cl->next;
	}
	mqtt_send_response(NULL, mqtt_id, MQTT_RET_SUCCESS, NULL, 0);
}

/*
 * send the task ID of the task to be stopped to the client
 * the function returns the result of the socket write function
 */
ssize_t send_stop_task(client_data *client, uint32_t mqtt_id, uint32_t task_id) {
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	ssize_t ret;
	int req_id;
	//long long unsigned t;

	// build message in msgpack format
	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	msgpack_pack_array(&pk, 3);
	msgpack_pack_int(&pk, REQUEST_TYPE_STOP_TASK);
	msgpack_pack_uint32(&pk, mqtt_id);
	msgpack_pack_uint32(&pk, task_id);

	// send message
	ret = write(client->socket_fd, sbuf.data, sbuf.size);
	//t = get_time();
	//printf("(client %s) sent stop task request [%llu]\n", client->id, t);
	printf("(client %s) sent stop task request\n", client->id);
	if(ret != sbuf.size) {
		printf("(client %s) failed sending stop task request: %i\n", client->id, ret);
		mqtt_send_response(client->id, mqtt_id, MQTT_RET_SEND_ERROR, NULL, 0);
	}
		else printf("(client %s) sent stop task request\n", client->id);

	msgpack_sbuffer_destroy(&sbuf);

	return ret;
}

// send the protocol version to the given client
void send_protocol_version(client_data *client) {
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	int ret;

	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	
	msgpack_pack_array(&pk, 2);
	msgpack_pack_int(&pk, REQUEST_TYPE_PROTOCOL_VERSION);
	msgpack_pack_str(&pk, sizeof(PROCOL_VERSION)-1);
	msgpack_pack_str_body(&pk, PROCOL_VERSION, sizeof(PROCOL_VERSION)-1);

	ret = write(client->socket_fd, sbuf.data, sbuf.size);
	if(ret != sbuf.size) {
		printf("(client %s) failed sending protocol version: %i\n", client->id, ret);
	}
		else printf("(client %s) sent protocol version\n", client->id);

	msgpack_sbuffer_destroy(&sbuf);
}

// extract and evaluate the protocol version
void evaluate_protocol_version(client_data *client, msgpack_object obj) {
	printf("(client -%i-) Received protocol version: ", client->socket_fd);
	msgpack_object_print(stdout, obj);
	printf("\n");
	client->state = GOT_PROTOCOL_VERSION;
}

// extract and evaluate the client id
void evaluate_client_id(client_data *client, msgpack_object obj) {
	metadata_value *metadata_val;
	char *id;
	client_data *old_client;
	pthread_t old_thread_fd;

	printf("(client -%i-) Received client id: ", client->socket_fd);
	msgpack_object_print(stdout, obj);
	printf("\n");
	id = malloc(obj.via.str.size+1);
	memcpy(id , obj.via.str.ptr, obj.via.str.size);
	id[obj.via.str.size] = 0x0;

	client->id = id;
	client->state = GOT_CLIENT_ID;

	metadata_val = metadata_kv_get_value_ptr(config_metadata_g, client->id);
	if(metadata_val == NULL) client->metadata_config = NULL;
		else client->metadata_config = metadata_val->data.kv;

	send_protocol_version(client);
}


// extract and evaluate the metadata
void evaluate_metadata(client_data *client, msgpack_object obj) {
	printf("(client %s) Received metadata: ", client->id);
	msgpack_object_print(stdout, obj);
	printf("\n");
	evaluate_msgpack_metadata(client, obj);
	client->state = INITIALIZED;

	subscribe_mqtt_client(client);
}

// extract and evaluate a control request result
void evaluate_control_result(client_data *client, msgpack_object obj_req_id, msgpack_object obj_taskid_exists, msgpack_object obj_taskid, msgpack_object obj_result) {
	printf("(client %s) Received result of task start/stop\n", client->id);
	//printf("(client %s) Received result of task start/stop: ", client->id);
	//msgpack_object_print(stdout, obj_result);
	//printf("\n");

	if(obj_taskid_exists.type == MSGPACK_OBJECT_BOOLEAN) {
		// result of start task request
		// check if it is a pull or push task
		if(obj_taskid_exists.via.boolean) {
			pthread_mutex_lock(&mqtt_id_map_mutex);
			mqtt_id_map_add(client, (uint32_t) obj_req_id.via.u64, (uint32_t) obj_taskid.via.u64);
			pthread_mutex_unlock(&mqtt_id_map_mutex);
		}
	}
	// else: result of stop task request

	mqtt_send_response(client->id, (uint32_t) obj_req_id.via.u64, MQTT_RET_SUCCESS, obj_result.via.str.ptr, obj_result.via.str.size);
}

// extract and evaluate a message generated by a task from the client
void evaluate_task_message(client_data *client, msgpack_object obj) {
	char *msg;
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	uint16_t location_len;
	uint16_t location_len_nbo;
	uint16_t task_msg_len = obj.via.str.size;
	uint16_t task_msg_len_nbo = htons(task_msg_len);
	uint64_t timestamp = time(NULL);
	uint64_t timestamp_nbo = htobe64(timestamp);

	printf("(client %s) Received task message: ", client->id);
	msgpack_object_print(stdout, obj);
	printf("\n");
	//decode_NDEE_result(client, obj.via.str.ptr, obj.via.str.size);

	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	metadata_convert_msgpack_location(client, &pk);
	location_len = sbuf.size;
	location_len_nbo = htons(location_len);
	
	msg = malloc(12+location_len+task_msg_len);
	memcpy(msg, &timestamp_nbo, 8);
	memcpy(&msg[8], &location_len_nbo, 2);
	memcpy(&msg[10], sbuf.data, location_len);
	memcpy(&msg[10+location_len], &task_msg_len_nbo, 2);
	memcpy(&msg[12+location_len], obj.via.str.ptr, task_msg_len);

	mqtt_send_task_message(client, msg, 12+location_len+task_msg_len);
	free(msg);
	msgpack_sbuffer_destroy(&sbuf);
}

// evaluate an unpacked message received from a client
void evaluate_client_array(client_data *client, msgpack_object *objs, int length) {
	if(length < 1) {
		fprintf(stderr, "(client %s) received message with invalid length (p0)\n", client->id);
		return;
	}
	// the first object in the array is the type and this is a positive integer
	if(objs[0].type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
		// successfully unpacked the type
		if(client->state == INITIAL) {
			// expect to get the protocol version
			if(objs[0].via.u64 == REQUEST_TYPE_PROTOCOL_VERSION) {
				if(length != 2) {
					fprintf(stderr, "(client -%i-) received message with invalid length (p1)\n", client->socket_fd);
					return;
				}
				evaluate_protocol_version(client, objs[1]);
			}
				else {
					printf("(client -%i-) expected protocol version, but request has type: %llu\n", client->socket_fd, objs[0].via.u64);
				}
		}
		else if(client->state == GOT_PROTOCOL_VERSION) {
			// expect to get the client ID
			if(objs[0].via.u64 == REQUEST_TYPE_CLIENT_ID) {
				if(length != 2) {
					fprintf(stderr, "(client -%i-) received message with invalid length (p2)\n", client->socket_fd);
					return;
				}
				evaluate_client_id(client, objs[1]);
			}
				else {
					printf("(client -%i-) expected client id, but request has type: %llu\n", client->socket_fd, objs[0].via.u64);
				}
		}
		else if(client->state == GOT_CLIENT_ID) {
			// expect to get the metadata
			if(objs[0].via.u64 == REQUEST_TYPE_METADATA) {
				if(length != 2) {
					fprintf(stderr, "(client %s) received message with invalid length (p3)\n", client->id);
					return;
				}
				evaluate_metadata(client, objs[1]);
			}
				else {
					printf("(client %s) expected initial metadata, but request has type: %llu\n", client->id, objs[0].via.u64);
				}
		}
		else if(client->state == INITIALIZED) {
			if(objs[0].via.u64 == REQUEST_TYPE_METADATA) {
				if(length != 2) {
					fprintf(stderr, "(client %s) received message with invalid length (p4)\n", client->id);
					return;
				}
				evaluate_metadata(client, objs[1]);
			}
			else if(objs[0].via.u64 == REQUEST_TYPE_CONTROL_RESULT) {
				if(length != 5) {
					fprintf(stderr, "(client %s) received message with invalid length (p5)\n", client->id);
					return;
				}
				evaluate_control_result(client, objs[1], objs[2], objs[3], objs[4]);
			}
			else if(objs[0].via.u64 == REQUEST_TYPE_MESSAGE) {
				if(length != 2) {
					fprintf(stderr, "(client %s) received message with invalid length (p6)\n", client->id);
					return;
				}
				evaluate_task_message(client, objs[1]);
			}
				else {
					printf("(client %s) unexpected type of message: %llu\n", client->id, objs[0].via.u64);
				}
		}
	}
		else {
			printf("(client %s) type field is not a positive integer, but of type: %i\n", client->id, objs[0].type);
		}
}

// evaluate message received via socket connection from client
// the message is unpacked with the msgpack library functions
msg_ptr_len_t evaluate_client_msg(client_data *client, char *msg, int length) {
	msgpack_unpacked unp;
	msgpack_unpack_return ret;
	msg_ptr_len_t return_data;
	msgpack_object *objs;
	size_t offset = 0;

	msgpack_unpacked_init(&unp);

	ret = msgpack_unpack_next(&unp, msg, length, &offset);
	return_data.state = ret;

	if(ret == MSGPACK_UNPACK_SUCCESS && unp.data.type == MSGPACK_OBJECT_ARRAY) {
		objs = malloc(sizeof(msgpack_object) * unp.data.via.array.size);
		memcpy(objs, unp.data.via.array.ptr, (sizeof(msgpack_object) * unp.data.via.array.size));
		return_data.array_objs = objs;
		return_data.array_size = unp.data.via.array.size;
		return_data.offset = offset;
		return_data.is_array = true;
	}
		else return_data.is_array = false;

	msgpack_unpacked_destroy(&unp);

	return return_data;
}

// free all data related to the given client
void client_cleanup(client_data *client) {
	pthread_mutex_lock(&mqtt_access_clients);
	
	pthread_mutex_lock(&access_client_list);
	pthread_mutex_lock(&mqtt_id_map_mutex);

	clients_list = list_remove(clients_list, client);
	mqtt_id_map_remove_client(client);

	pthread_mutex_unlock(&mqtt_id_map_mutex);
	pthread_mutex_unlock(&access_client_list);

	free(client->id);
	metadata_free_kv(client->metadata_merged);
	pthread_mutex_destroy(&client->metadata_merged_mutex);

	free(client);
	pthread_mutex_unlock(&mqtt_access_clients);
}

// each client is running in a thread
void *client_thread(void *ptr) {
	client_data *client = (client_data*) ptr;
	int socket_fd = client->socket_fd;
	char recv_data[256];
	char *msg_buffer = NULL;
	int read_count;
	int msg_length = 0;
	msg_ptr_len_t ev_arr_ret;
	//long long unsigned t;

	printf("new client (%i) connected\n", socket_fd);

	while(1) {
		// read from client
		read_count = read(socket_fd, recv_data, sizeof(recv_data));
		//t = get_time();

		// error checking
		if(read_count == 0)
		{
			// client has disconnected
			if(client->state == GOT_CLIENT_ID || client->state == INITIALIZED) {
				printf("(client %s) disconnected\n\n", client->id);
			}
				else printf("(client -%i-) disconnected\n\n", socket_fd);
			client_cleanup(client);
			if(msg_buffer != NULL) free(msg_buffer);
			return (void *) 0;
		}
		else if(read_count < 0)
		{
			if(client->state == GOT_CLIENT_ID || client->state == INITIALIZED) {
				printf("(client %s) reading failed\n", client->id);
			}
				else {
					printf("(client -%i-) reading failed\n", socket_fd);
				}
			read_count = 0;
		}

		// handle received data
		// in case msg_buffer is NULL, realloc behaves like malloc
		msg_buffer = realloc(msg_buffer, msg_length+read_count);
		memcpy(msg_buffer+msg_length, recv_data, read_count);
		msg_length += read_count;

		do {
			// evaluate msgpack
			ev_arr_ret = evaluate_client_msg(client, msg_buffer, msg_length);
			if(ev_arr_ret.state == MSGPACK_UNPACK_SUCCESS) {
				if(ev_arr_ret.is_array) {
					// received complete array
					if(client->state == GOT_CLIENT_ID || client->state == INITIALIZED) {
						//printf("(client %s) received socket message at [%llu]\n", client->id, t);
						printf("(client %s) received socket message\n", client->id);
					}
						else {
							//printf("(client -%i-) received socket message at [%llu]\n", socket_fd, t);
							printf("(client -%i-) received socket message\n", socket_fd);
						}
					evaluate_client_array(client, ev_arr_ret.array_objs, ev_arr_ret.array_size);
					if(ev_arr_ret.offset < msg_length) {
						// another array (part of or complete) was received
						memmove(msg_buffer, msg_buffer+ev_arr_ret.offset, (msg_length - ev_arr_ret.offset));
						msg_length -= ev_arr_ret.offset;
						// don't realloc to smaller size, as it will be reallocated in the next
						// while-loop iteration anyway
					}
						else
						{
							// no additional message in msg_buffer
							msg_length = 0;
							free(msg_buffer);
							msg_buffer = NULL;
						}
					free(ev_arr_ret.array_objs);
				}
					else {
						if(client->state == GOT_CLIENT_ID || client->state == INITIALIZED) {
							printf("(client %s) received message is not an array\n", client->id);
						}
							else {
								printf("(client -%i-) received message is not an array\n", socket_fd);
							}
						msg_length = 0;
						free(msg_buffer);
						msg_buffer = NULL;
					}
			}
			else if(ev_arr_ret.state != MSGPACK_UNPACK_CONTINUE) {
				// error occurred
				if(client->state == GOT_CLIENT_ID || client->state == INITIALIZED) {
					printf("(client %s) error unpacking message: %i\n", client->id, ev_arr_ret.state);
				}
					else {
						printf("(client -%i-) error unpacking message: %i\n", socket_fd, ev_arr_ret.state);
					}
				msg_length = 0;
				free(msg_buffer);
				msg_buffer = NULL;
			}
			// else: yet bytes missing for complete array
		} while(msg_length > 0 && ev_arr_ret.state == MSGPACK_UNPACK_SUCCESS);
	}
}

int main(int argc, char const *argv[]) {
	int server_fd, client_fd;
	struct sockaddr_in server_settings;
	pthread_t mqtt_thread_fd;
	client_data *new_client;
	int i;

	setbuf(stdout, NULL);
	if(read_configuration() != 0) {
		return 1;
	}

	// create server socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd < 0) {
		perror("creating socket failed");
		return 1;
	}
	server_settings.sin_family = AF_INET;
	server_settings.sin_addr.s_addr = INADDR_ANY;
	server_settings.sin_port = htons(socket_port_g);

	// https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr/25193462
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

	// bind socket
	if(bind(server_fd, (struct sockaddr *)&server_settings, sizeof(server_settings)) < 0) {
		perror("binding socket failed");
		return 1;
	}
	// listen for new clients
	listen(server_fd, SOMAXCONN);

	pthread_mutex_init(&access_client_list, NULL);
	pthread_mutex_init(&mqtt_access_clients, NULL);
	pthread_mutex_init(&metadata_compare_functions_mutex, NULL);

	// create thread for MQTT
	pthread_create(&mqtt_thread_fd, NULL, mqtt_thread, NULL);

	// register functions for comparing complex typed metadata
	register_compare_functions();


	while(1) {
		// accept new client
		new_client = malloc(sizeof(client_data));
		new_client->socket_fd = accept(server_fd, NULL, NULL);
		if(new_client->socket_fd < 0) {
			perror("accepting client failed");
			free(new_client);
		}
			else
			{
				// set state to initial
				new_client->state = INITIAL;
				new_client->id = NULL;
				new_client->metadata_merged = NULL;
				pthread_mutex_init(&new_client->metadata_merged_mutex, NULL);
				pthread_mutex_lock(&access_client_list);
				clients_list = list_add(clients_list, new_client);
				pthread_mutex_unlock(&access_client_list);

				// create thread for client
				pthread_create(&new_client->thread_fd, NULL, client_thread, new_client);
			}
	}

	mosquitto_lib_cleanup();

	return 0;
}
