#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <mosquitto.h>
#include <arpa/inet.h>
#include <msgpack.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

#define PUSH_TASK_EXAMPLE_TEMPERATURE "dle = require 'dle'; return dle.net(10, 300, 100, dle.temperature(3, 300, 100));"
#define PUSH_TASK_EXAMPLE_PRESSURE "dle = require 'dle'; return dle.net(10, 300, 100, dle.pressure(3, 300, 100));"
#define PULL_TASK_EXAMPLE "bmp180 = require 'bmp180'; return bmp180.temperature(2);"


const int subscribe_topics_num = 9;
int subscribe_count = 0;
struct mosquitto *mq_g;
sem_t mqtt_connected_sem;
sem_t mqtt_sent_sem;

pthread_t mqtt_thread_fd_g;

void init_mqtt();
void start_mqtt_thread();

char *string_add(char *dest, void *src, int len) {
	memcpy(dest, src, len);
	return dest+len;
}

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

void decode_NDEE_result(const char *msg, size_t msg_size) {
	char buffer[33];
	int type;
	if(msg_size != 14*8 && msg_size != 6*8 && msg_size != 11*8) {
		printf("unknown msg size: %i\n", msg_size);
	}
	memcpy(buffer, msg, 8);
	buffer[8] = 0x0;
	printf("status: %li\n", strtol(buffer, NULL, 2));
	memcpy(buffer, msg+(1*8), 8);
	printf("requestid: %li\n", strtol(buffer, NULL, 2));
	memcpy(buffer, msg+(2*8), 8*4);
	buffer[8*4] = 0x0;
	printf("timestamp: %li\n", strtol(buffer, NULL, 2));
	
	if(msg_size == 14*8) {
		memcpy(buffer, msg+(6*8), 8);
		buffer[8] = 0x0;
		printf("measurand: %li\n", strtol(buffer, NULL, 2));
		memcpy(buffer, msg+(7*8), 8);
		printf("device: %li\n", strtol(buffer, NULL, 2));
		memcpy(buffer, msg+(8*8), 8);
		printf("unit: %li\n", strtol(buffer, NULL, 2));
		memcpy(buffer, msg+(9*8), 8);
		type = (int) strtol(buffer, NULL, 2);
		printf("type: %li\n", (long int) type);
		memcpy(buffer, msg+(10*8), 8*4);
		buffer[8*4] = 0x0;
		if(type == 0) printf("value: %li\n", strtol(buffer, NULL, 2));
		else if(type == 1) {
			float val = binary_to_float(buffer);
			printf("value: %f\n", val);
		}
			else printf("value: %li (type %i)\n", strtol(buffer, NULL, 2), type);
	}
	else if(msg_size == 11*8) {
		memcpy(buffer, msg+(6*8), 8);
		buffer[8] = 0x0;
		type = (int) strtol(buffer, NULL, 2);
		printf("type: %li\n", (long int) type);
		memcpy(buffer, msg+(7*8), 8*4);
		buffer[8*4] = 0x0;
		if(type == 0) printf("value: %li\n", strtol(buffer, NULL, 2));
		else if(type == 1) {
			float val = binary_to_float(buffer);
			printf("value: %f\n", val);
		}
			else printf("value: %li (type %i)\n", strtol(buffer, NULL, 2), type);
	}
}

void mqtt_message_callback(struct mosquitto *mq, void *userdata, const struct mosquitto_message *message) {
	bool match;
	//long long unsigned t;

	//t = get_time();
	//printf("got message with topic '%s', size %i [%llu]\n", message->topic, message->payloadlen, t);
	printf("got message with topic '%s', size %i\n", message->topic, message->payloadlen);

	mosquitto_topic_matches_sub("deployment_name/+/control/result", message->topic, &match);
	if(match) {
		uint32_t request_id;
		uint32_t request_id_nbo;
		uint8_t ret_code;
		uint16_t envret_size;
		uint16_t envret_size_nbo;
		char *envret;

		memcpy(&request_id_nbo, message->payload, 4);
		memcpy(&ret_code, message->payload+4, 1);
		memcpy(&envret_size_nbo, message->payload+5, 2);
		request_id = ntohl(request_id_nbo);
		envret_size = ntohs(envret_size_nbo);

		printf("request id: %i, return code: %i\n", request_id, ret_code);

		if(7+envret_size != message->payloadlen) {
			printf("size does not match: %i != %i\n", 5+envret_size, message->payloadlen);
			return;
		}
		envret = malloc(envret_size);
		memcpy(envret, message->payload+7, envret_size);
		decode_NDEE_result(envret, envret_size);
		free(envret);
		printf("--------------------------------------\n");
		return;
	}
	mosquitto_topic_matches_sub("deployment_name/control/result", message->topic, &match);
	if(match) {
		uint32_t request_id;
		uint32_t request_id_nbo;
		uint8_t ret_code;
		
		memcpy(&request_id_nbo, message->payload, 4);
		memcpy(&ret_code, message->payload+4, 1);
		request_id = ntohl(request_id_nbo);
		
		printf("request id: %i, return code: %i\n", request_id, ret_code);
		printf("--------------------------------------\n");
		return;
	}
	mosquitto_topic_matches_sub("deployment_name/+/task_message", message->topic, &match);
	if(match) {
		uint16_t location_len;
		uint16_t location_len_nbo;
		uint16_t envret_size;
		uint16_t envret_size_nbo;
		uint64_t timestamp;
		uint64_t timestamp_nbo;
		msgpack_unpacked unp;
		msgpack_unpack_return ret;
		char *envret;

		msgpack_unpacked_init(&unp);
		memcpy(&timestamp_nbo, message->payload, 8);
		timestamp = be64toh(timestamp_nbo);
		memcpy(&location_len_nbo, message->payload+8, 2);
		location_len = ntohs(location_len_nbo);
		ret = msgpack_unpack_next(&unp, message->payload+10, location_len, NULL);

		memcpy(&envret_size_nbo, message->payload+10+location_len, 2);
		envret_size = htons(envret_size_nbo);
		envret = malloc(envret_size);
		memcpy(envret, message->payload+12+location_len, envret_size);
		printf("timestamp: %llu, location: ", timestamp);
		msgpack_object_print(stdout, unp.data);
		printf("\n");
		decode_NDEE_result(envret, envret_size);
		
		msgpack_unpacked_destroy(&unp);
		free(envret);
		printf("--------------------------------------\n");
		return;
	}
		else printf("unknown MQTT topic\n");
}

void send_metadata_complex_command(struct mosquitto *mq) {
	char topic_pub[] = "deployment_name/control/start";
	char task[] = "bmp180 = require 'bmp180'; return bmp180.temperature(2);";
	char mkey[] = "location";
	uint32_t request_id = 12345;
	uint32_t request_id_nbo = htonl(request_id);
	uint16_t task_size = strlen(task);
	uint16_t task_size_nbo = htons(task_size);
	uint16_t mkey_size = strlen(mkey);
	uint16_t mkey_size_nbo = htons(mkey_size);
	uint16_t mval_size;
	uint16_t mval_size_nbo;
	uint8_t value_type = 7;
	uint8_t compare_type = 30;
	char *msg_start, *msg;
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	//long long unsigned t;

	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	
	msgpack_pack_map(&pk, 2);
	msgpack_pack_str(&pk, sizeof("room")-1);
	msgpack_pack_str_body(&pk, "room", sizeof("room")-1);
	msgpack_pack_int(&pk, 123);
	msgpack_pack_str(&pk, sizeof("building")-1);
	msgpack_pack_str_body(&pk, "building", sizeof("building")-1);
	msgpack_pack_str(&pk, sizeof("building-name")-1);
	msgpack_pack_str_body(&pk, "building-name", sizeof("building-name")-1);
	mval_size = sbuf.size;
	mval_size_nbo = htons(mval_size);

	msg_start = malloc(12+task_size+mkey_size+mval_size);
	msg = msg_start;
	msg = string_add(msg, &request_id_nbo, 4);
	msg = string_add(msg, &task_size_nbo, 2);
	msg = string_add(msg, task, task_size);
	msg = string_add(msg, &mkey_size_nbo, 2);
	msg = string_add(msg, mkey, mkey_size);
	msg = string_add(msg, &value_type, 1);
	msg = string_add(msg, &compare_type, 1);
	msg = string_add(msg, &mval_size_nbo, 2);
	msg = string_add(msg, sbuf.data, mval_size);

	//t = get_time();
	mosquitto_publish(mq, NULL, topic_pub, 12+task_size+mkey_size+mval_size, msg_start, 2, false);
	//printf("complex metadata pull command msg sent [%llu]\n", t);
	printf("complex metadata pull command msg sent\n");
	free(msg_start);
	msgpack_sbuffer_destroy(&sbuf);
}

void print_help(const char *cmd_name) {
	printf("%s COMMAND [OPTIONS]…\n", cmd_name);
	printf("commands:\n");
	printf("	help\n");
	printf("		show this help text\n");
	printf("	start-id CLIENT_ID push|pull|custom [SCRIPT]\n");
	printf("		send a start task request to a client with the given CLIENT_ID\n");
	printf("		send an example push, an example pull or a custom task [SCRIPT]\n");
	printf("	start-md push-t|push-p|pull|custom [SCRIPT]\n");
	printf("		send a start task request to all client with matching metadata\n");
	printf("		send an example push task for temperature sensor, an example\n");
	printf("		  push task for pressure sensor, an example pull or a custom\n");
	printf("		  task [SCRIPT]\n");
	printf("	stop-id CLIENT_ID MQTT_ID\n");
	printf("		send request to stop a task with given MQTT_ID to a client with\n");
	printf("		  the given CLIENT_ID\n");
	printf("	stop MQTT_ID\n");
	printf("		send request to stop a task with given MQTT_ID to all clients\n");
	printf("		  which execute this task\n");
	printf("	receive\n");
	printf("		receive all MQTT messages sent by the control application\n");
}

int start_task_by_id(int argc, const char *argv[]) {
	char *topic_pub;
	char *task;
	uint32_t request_id = rand() % 4294967296;
	uint32_t request_id_nbo = htonl(request_id);
	uint16_t task_size;
	uint16_t task_size_nbo;
	char *msg;
	//long long unsigned t;

	if(argc < 4) {
		printf("too few arguments\n");
		print_help(argv[0]);
		return 1;
	}
	if(strcmp(argv[3], "custom") == 0 && argc < 5) {
		printf("too few arguments\n");
		print_help(argv[0]);
		return 1;
	}

	if(strcmp(argv[3], "push") == 0) {
		task = malloc(sizeof(PUSH_TASK_EXAMPLE_TEMPERATURE));
		memcpy(task, PUSH_TASK_EXAMPLE_TEMPERATURE, sizeof(PUSH_TASK_EXAMPLE_TEMPERATURE));
		task_size = sizeof(PUSH_TASK_EXAMPLE_TEMPERATURE)-1;
	}
	else if(strcmp(argv[3], "pull") == 0) {
		task = malloc(sizeof(PULL_TASK_EXAMPLE));
		memcpy(task, PULL_TASK_EXAMPLE, sizeof(PULL_TASK_EXAMPLE));
		task_size = sizeof(PULL_TASK_EXAMPLE)-1;
	}
	else if(strcmp(argv[3], "custom") == 0) {
		task = malloc(strlen(argv[4])+1);
		memcpy(task, argv[4], strlen(argv[4])+1);
		task_size = strlen(argv[4]);
	}
		else {
			fprintf(stderr, "unknown task type %s\n", argv[3]);
			print_help(argv[0]);
			return 1;
		}
	task_size_nbo = htons(task_size);
	topic_pub = malloc(30+1+strlen(argv[2]));
	snprintf(topic_pub, 31+strlen(argv[2]), "deployment_name/%s/control/start", argv[2]);

	msg = malloc(6+task_size);
	memcpy(msg, &request_id_nbo, 4);
	memcpy(&msg[4], &task_size_nbo, 2);
	memcpy(&msg[6], task, task_size);

	sem_wait(&mqtt_connected_sem);
	//t = get_time();
	mosquitto_publish(mq_g, NULL, topic_pub, 6+task_size, msg, 2, false);
	//printf("sent start task request, MQTT request id: %i [%llu]\n", request_id, t);
	printf("sent start task request, MQTT request id: %i\n", request_id);
	free(msg);
	free(task);
	free(topic_pub);
	return 0;
}

int start_task_by_metadata(int argc, const char *argv[]) {
	char topic_pub[] = "deployment_name/control/start";
	char *task;
	uint32_t request_id = rand() % 4294967296;	
	uint32_t request_id_nbo = htonl(request_id);
	uint16_t task_size;
	uint16_t task_size_nbo;
	char mkey_example[] = "battery";
	char *mkey;
	int32_t mval_int = 20;
	char *mval_str = NULL;
	uint16_t mkey_size = strlen(mkey_example);
	uint16_t mkey_size_nbo = htons(mkey_size);
	uint16_t mval_size = 4;
	uint16_t mval_size_nbo = htons(mval_size);
	uint8_t value_type = 0;
	uint8_t compare_type = 0;
	char *msg_start, *msg;
	//long long unsigned t;

	mkey = malloc(strlen(mkey_example));
	memcpy(mkey, mkey_example, strlen(mkey_example));

	if(argc < 3) {
		printf("too few arguments\n");
		print_help(argv[0]);
		return 1;
	}

	if(strcmp(argv[2], "push-t") == 0) {
		task = malloc(sizeof(PUSH_TASK_EXAMPLE_TEMPERATURE));
		memcpy(task, PUSH_TASK_EXAMPLE_TEMPERATURE, sizeof(PUSH_TASK_EXAMPLE_TEMPERATURE));
		task_size = sizeof(PUSH_TASK_EXAMPLE_TEMPERATURE)-1;

		free(mkey);
		mkey = malloc(strlen("available-measurands"));
		memcpy(mkey, "available-measurands", strlen("available-measurands"));
		mkey_size = strlen("available-measurands");
		mkey_size_nbo = htons(mkey_size);
		value_type = 5;
		compare_type = 19;
		mval_str = malloc(strlen("temperature"));
		memcpy(mval_str, "temperature", strlen("temperature"));
		mval_size = strlen("temperature");
		mval_size_nbo = htons(mval_size);
	}
	else if(strcmp(argv[2], "push-p") == 0) {
		task = malloc(sizeof(PUSH_TASK_EXAMPLE_PRESSURE));
		memcpy(task, PUSH_TASK_EXAMPLE_PRESSURE, sizeof(PUSH_TASK_EXAMPLE_PRESSURE));
		task_size = sizeof(PUSH_TASK_EXAMPLE_PRESSURE)-1;

		free(mkey);
		mkey = malloc(strlen("available-measurands"));
		memcpy(mkey, "available-measurands", strlen("available-measurands"));
		mkey_size = strlen("available-measurands");
		mkey_size_nbo = htons(mkey_size);
		value_type = 5;
		compare_type = 19;
		mval_str = malloc(strlen("pressure"));
		memcpy(mval_str, "pressure", strlen("pressure"));
		mval_size = strlen("pressure");
		mval_size_nbo = htons(mval_size);
	}
	else if(strcmp(argv[2], "pull") == 0) {
		task = malloc(sizeof(PULL_TASK_EXAMPLE));
		memcpy(task, PULL_TASK_EXAMPLE, sizeof(PULL_TASK_EXAMPLE));
		task_size = sizeof(PULL_TASK_EXAMPLE)-1;
	}
	else if(strcmp(argv[2], "custom") == 0) {
		if(argc < 4) {
			printf("too few arguments\n");
			print_help(argv[0]);
			return 1;
		}
		task = malloc(strlen(argv[3])+1);
		memcpy(task, argv[3], strlen(argv[3])+1);
		task_size = strlen(argv[3]);
	}
		else {
			fprintf(stderr, "unknown task type %s\n", argv[2]);
			print_help(argv[0]);
			return 1;
		}
	task_size_nbo = htons(task_size);

	msg_start = malloc(12+task_size+mkey_size+mval_size);
	msg = msg_start;
	msg = string_add(msg, &request_id_nbo, 4);
	msg = string_add(msg, &task_size_nbo, 2);
	msg = string_add(msg, task, task_size);
	msg = string_add(msg, &mkey_size_nbo, 2);
	msg = string_add(msg, mkey, mkey_size);
	msg = string_add(msg, &value_type, 1);
	msg = string_add(msg, &compare_type, 1);
	msg = string_add(msg, &mval_size_nbo, 2);
	if(value_type == 0) msg = string_add(msg, &mval_int, mval_size);
	else if(value_type == 5) {
		msg = string_add(msg, mval_str, mval_size);
	}
		else {
			fprintf(stderr, "start_task_by_metadata: internal error, value_type invalid\n");
			return 1;
		}

	sem_wait(&mqtt_connected_sem);
	//t = get_time();
	mosquitto_publish(mq_g, NULL, topic_pub, 12+task_size+mkey_size+mval_size, msg_start, 2, false);
	//printf("sent start task request, MQTT request id: %u [%llu]\n", request_id, t);
	printf("sent start task request, MQTT request id: %u\n", request_id);
	free(msg_start);
	free(task);
	free(mkey);
	free(mval_str);
	return 0;
}

int stop_task_by_id(int argc, const char *argv[]) {
	char *topic_pub;
	uint32_t request_id = rand() % 4294967296;
	uint32_t request_id_nbo = htonl(request_id);
	uint32_t task_id;
	uint32_t task_id_nbo;
	char *msg;
	//long long unsigned t;

	if(argc < 4) {
		printf("too few arguments\n");
		print_help(argv[0]);
		return 1;
	}
	topic_pub = malloc(29+1+strlen(argv[2]));
	snprintf(topic_pub, 30+strlen(argv[2]), "deployment_name/%s/control/stop", argv[2]);
	task_id = (uint32_t) atoi(argv[3]); // expecting valid number
	task_id_nbo = htonl(task_id);

	msg = malloc(8);
	memcpy(msg, &request_id_nbo, 4);
	memcpy(&msg[4], &task_id_nbo, 4);

	sem_wait(&mqtt_connected_sem);
	//t = get_time();
	mosquitto_publish(mq_g, NULL, topic_pub, 8, msg, 2, false);
	//printf("sent stop task request, MQTT request id: %u [%llu]\n", request_id, t);
	printf("sent stop task request, MQTT request id: %u\n", request_id);
	free(msg);
	free(topic_pub);
	return 0;
}

int stop_task(int argc, const char *argv[]) {
	char topic_pub[] = "deployment_name/control/stop";
	uint32_t request_id = rand() % 4294967296;
	uint32_t request_id_nbo = htonl(request_id);
	uint32_t task_id;
	uint32_t task_id_nbo;
	char *msg;
	//long long unsigned t;

	if(argc < 3) {
		printf("too few arguments\n");
		print_help(argv[0]);
		return 1;
	}
	task_id = (uint32_t) atoi(argv[2]); // expecting valid number
	task_id_nbo = htonl(task_id);

	msg = malloc(8);
	memcpy(msg, &request_id_nbo, 4);
	memcpy(&msg[4], &task_id_nbo, 4);

	sem_wait(&mqtt_connected_sem);
	//t = get_time();
	mosquitto_publish(mq_g, NULL, topic_pub, 8, msg, 2, false);
	//printf("sent stop task request, MQTT request id: %u [%llu]\n", request_id, t);
	printf("sent stop task request, MQTT request id: %u\n", request_id);
	free(msg);
	return 0;
}

void mqtt_publish_callback(struct mosquitto *mq, void *userdata, int mid) {
	//long long unsigned t;

	//t = get_time();
	//printf("publish callback [%llu]\n", t);
	printf("MQTT publish callback\n");
	sem_post(&mqtt_sent_sem);
}

void mqtt_subscribe_callback(struct mosquitto *mq, void *userdata, int mid, int qos_count, const int *granted_qos) {
	subscribe_count++;

	if(subscribe_count == subscribe_topics_num) {
		printf("subscribed to all topics\n");	
	}
}

void subscribe_mqtt_topics() {
	char topic_sub_0_r[] = "deployment_name/ID0/control/result";
	char topic_sub_0_m[] = "deployment_name/ID0/task_message";
	char topic_sub_1_r[] = "deployment_name/ID1/control/result";
	char topic_sub_1_m[] = "deployment_name/ID1/task_message";
	char topic_sub_2_r[] = "deployment_name/ID2/control/result";
	char topic_sub_2_m[] = "deployment_name/ID2/task_message";
	char topic_sub_3_r[] = "deployment_name/ID3/control/result";
	char topic_sub_3_m[] = "deployment_name/ID3/task_message";
	char topic_sub_r[] = "deployment_name/control/result";

	// Quality of Service 2: exactly once
	mosquitto_subscribe(mq_g, NULL, topic_sub_0_r, 2);
	mosquitto_subscribe(mq_g, NULL, topic_sub_0_m, 2);
	mosquitto_subscribe(mq_g, NULL, topic_sub_1_r, 2);
	mosquitto_subscribe(mq_g, NULL, topic_sub_1_m, 2);
	mosquitto_subscribe(mq_g, NULL, topic_sub_2_r, 2);
	mosquitto_subscribe(mq_g, NULL, topic_sub_2_m, 2);
	mosquitto_subscribe(mq_g, NULL, topic_sub_3_r, 2);
	mosquitto_subscribe(mq_g, NULL, topic_sub_3_m, 2);
	mosquitto_subscribe(mq_g, NULL, topic_sub_r, 2);
}

void mqtt_connect_callback(struct mosquitto *mq, void *userdata, int result) {
	if(result == 0) {
		printf("MQTT connected\n");
		sem_post(&mqtt_connected_sem);
	} else {
		fprintf(stderr, "MQTT connect failed\n");
		exit(1);
	}
}

void *mqtt_thread(void *ptr) {
	mosquitto_loop_forever(mq_g, -1, 1);
}
void start_mqtt_thread() {
	pthread_create(&mqtt_thread_fd_g, NULL, mqtt_thread, NULL);	
}
void init_mqtt() {
	mosquitto_lib_init();
	mq_g = mosquitto_new(NULL, true, NULL); // clean session
	if(mq_g == NULL) {
		perror("error creating new mosquitto client instance");
		exit(1);
	}
	mosquitto_connect_callback_set(mq_g, mqtt_connect_callback);
	mosquitto_message_callback_set(mq_g, mqtt_message_callback);
	mosquitto_publish_callback_set(mq_g, mqtt_publish_callback);
	mosquitto_subscribe_callback_set(mq_g, mqtt_subscribe_callback);

	if(mosquitto_connect(mq_g, "localhost", 1883, 60) != MOSQ_ERR_SUCCESS){
		perror("error connecting mqtt client");
		exit(1);
	}
}
void mqtt_loop_for_sem(sem_t *sem) {
	while(true) {
		if(sem_trywait(sem) == 0) {
			sem_post(sem);
			return;
		}
		mosquitto_loop(mq_g, (-1), 1);
	}
}
void mqtt_cleanup() {
	mosquitto_destroy(mq_g);
	mosquitto_lib_cleanup();
}

int main(int argc, char const *argv[])
{
	int ret;

	srand((unsigned int) time(NULL));
	sem_init(&mqtt_connected_sem, 0, 0);
	sem_init(&mqtt_sent_sem, 0, 0);
	init_mqtt();

	setbuf(stdout, NULL);
	if(argc < 2) {
		fprintf(stderr, "argument missing\n");
		print_help(argv[0]);
		mqtt_cleanup();
		return 1;
	}
	else if(strcmp(argv[1], "help") == 0) {
		print_help(argv[0]);
	}
	else if(strcmp(argv[1], "start-id") == 0) {
		mqtt_loop_for_sem(&mqtt_connected_sem);
		ret = start_task_by_id(argc, argv);
		if(ret != 0) return ret;
		mqtt_loop_for_sem(&mqtt_sent_sem);
		mqtt_cleanup();
		return ret;
	}
	else if(strcmp(argv[1], "start-md") == 0) {
		mqtt_loop_for_sem(&mqtt_connected_sem);
		ret = start_task_by_metadata(argc, argv);
		if(ret != 0) return ret;
		mqtt_loop_for_sem(&mqtt_sent_sem);
		mqtt_cleanup();
		return ret;
	}
	else if(strcmp(argv[1], "stop-id") == 0) {
		mqtt_loop_for_sem(&mqtt_connected_sem);
		ret = stop_task_by_id(argc, argv);
		if(ret != 0) return ret;
		mqtt_loop_for_sem(&mqtt_sent_sem);
		mqtt_cleanup();
		return ret;
	}
	else if(strcmp(argv[1], "stop") == 0) {
		mqtt_loop_for_sem(&mqtt_connected_sem);
		ret = stop_task(argc, argv);
		if(ret != 0) return ret;
		mqtt_loop_for_sem(&mqtt_sent_sem);
		mqtt_cleanup();
		return ret;
	}
	else if(strcmp(argv[1], "receive") == 0) {
		start_mqtt_thread();
		sem_wait(&mqtt_connected_sem);
		subscribe_mqtt_topics();
		while(1) sleep(10);
	}
		else {
			printf("unknown command\n");
			print_help(argv[0]);
			return 1;
		}

	mqtt_cleanup();
	return 0;
}
