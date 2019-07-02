#include "string.h"
#include "stack.h"
#include "wifi.h"
#include "esp_log.h"
#include "unistd.h"
#include "controlsystem.h"
#include "msgpack.h"
#include "dle.h"
#include "configuration.h"



typedef enum {
	INITIAL = 0,
	SENT_PROTOCOL_VERSION = 1,
	SENT_CLIENT_ID = 2,
	INITIALIZED = 3
} communication_status_t;

typedef struct {
	int state;
	struct msgpack_object *array_objs;
	int array_size;
	int offset;
	bool is_array;
} msg_ptr_len_t;

static const char *TAG = "CONTROLSYSTEM";
int currentClient;
communication_status_t com_status;

const int SOCKET_CONNECTED = BIT0;
const int CLIENTID_SENT_BIT = BIT0;
EventGroupHandle_t *initiated_event_group;

// request types
#define REQUEST_TYPE_PROTOCOL_VERSION 0	// client -> server
#define REQUEST_TYPE_CLIENT_ID 1		// client -> server
#define REQUEST_TYPE_METADATA 2			// client -> server
#define REQUEST_TYPE_START_TASK 3		// server -> client
#define REQUEST_TYPE_STOP_TASK 4		// server -> client
#define REQUEST_TYPE_CONTROL_RESULT 5	// client -> server
#define REQUEST_TYPE_MESSAGE 6			// client -> server


void send_protocol_version() {
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	int ret;

	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	
	msgpack_pack_array(&pk, 2);
	msgpack_pack_int(&pk, REQUEST_TYPE_PROTOCOL_VERSION);
	msgpack_pack_str(&pk, sizeof(CONFIG_PROCOL_VERSION)-1);
	msgpack_pack_str_body(&pk, CONFIG_PROCOL_VERSION, sizeof(CONFIG_PROCOL_VERSION)-1);

	ret = send_binary(sbuf.data, sbuf.size);
	if(ret != sbuf.size) {
		ESP_LOGI(TAG, "failed sending protocol version: %i", ret);
	}
		else ESP_LOGI(TAG, "sent protocol version");

	msgpack_sbuffer_destroy(&sbuf);
	com_status = SENT_PROTOCOL_VERSION;
}

void send_client_id() {
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	int ret;

	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	
	msgpack_pack_array(&pk, 2);
	msgpack_pack_int(&pk, REQUEST_TYPE_CLIENT_ID);
	msgpack_pack_str(&pk, sizeof(CONFIG_CLIENT_ID)-1);
	msgpack_pack_str_body(&pk, CONFIG_CLIENT_ID, sizeof(CONFIG_CLIENT_ID)-1);

	ret = send_binary(sbuf.data, sbuf.size);
	if(ret != sbuf.size) {
		ESP_LOGI(TAG, "failed sending client id: %i", ret);
	}
		else ESP_LOGI(TAG, "sent client id");

	msgpack_sbuffer_destroy(&sbuf);
	com_status = SENT_CLIENT_ID;
	xEventGroupSetBits(initiated_event_group, CLIENTID_SENT_BIT);
}

void send_metadata() {
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	int ret;

	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	
	msgpack_pack_array(&pk, 2);
	msgpack_pack_int(&pk, REQUEST_TYPE_METADATA);
	msgpack_pack_map(&pk, 3);
	msgpack_pack_str(&pk, sizeof("battery")-1);
	msgpack_pack_str_body(&pk, "battery", sizeof("battery")-1);
	msgpack_pack_int(&pk, 20);
	msgpack_pack_str(&pk, sizeof("location")-1);
	msgpack_pack_str_body(&pk, "location", sizeof("location")-1);
		msgpack_pack_map(&pk, 2);
		msgpack_pack_str(&pk, sizeof("field1")-1);
		msgpack_pack_str_body(&pk, "field1", sizeof("field1")-1);
		msgpack_pack_str(&pk, sizeof("value1")-1);
		msgpack_pack_str_body(&pk, "value1", sizeof("value1")-1);
		msgpack_pack_str(&pk, sizeof("field2")-1);
		msgpack_pack_str_body(&pk, "field2", sizeof("field2")-1);
		msgpack_pack_int(&pk, 100);
	msgpack_pack_str(&pk, sizeof("sensors")-1);
	msgpack_pack_str_body(&pk, "sensors", sizeof("sensors")-1);
	msgpack_pack_array(&pk, 2);
	msgpack_pack_str(&pk, sizeof("bmp180")-1);
	msgpack_pack_str_body(&pk, "bmp180", sizeof("bmp180")-1);
	msgpack_pack_str(&pk, sizeof("bmp280")-1);
	msgpack_pack_str_body(&pk, "bmp280", sizeof("bmp280")-1);

	ret = send_binary(sbuf.data, sbuf.size);
	if(ret != sbuf.size) {
		ESP_LOGI(TAG, "failed sending metadata: %i\n", ret);
	}

	msgpack_sbuffer_destroy(&sbuf);
	com_status = INITIALIZED;
	ESP_LOGI(TAG, "sent metadata");
}


// extract and evaluate the protocol version
void evaluate_protocol_version(msgpack_object obj) {
	printf("Received protocol version: ");
	msgpack_object_print(stdout, obj);
	printf("\n");
	send_metadata();
}

bool check_task_type_push(char *result) {
	if(strlen(result) == 88) return true;
		else return false;
}

uint32_t extract_task_id(char *result) {
	uint32_t task_id = 0;
	int task_id_pos;

	task_id_pos = strlen(result)-32;
	if(task_id_pos >= 0) task_id = strtol(result+task_id_pos, NULL, 2);
		else ESP_LOGE(TAG, "extract_task_id: result length too small: %i", strlen(result));
	return task_id;
}

// extract task and start it
void evaluate_start_task(msgpack_object obj_req_id, msgpack_object obj_task) {
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	int ret;

	printf("Received task to start: ");
	msgpack_object_print(stdout, obj_task);
	printf("\n");

	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	
	char *task = malloc(obj_task.via.str.size+1);
	memcpy(task, obj_task.via.str.ptr, obj_task.via.str.size);
	task[obj_task.via.str.size] = 0x0;
	//ESP_LOGI(TAG, "Heap Caps, before start task: %d", heap_caps_check_integrity_all(true));
	char *result = startLuaTask(task);
	//ESP_LOGI(TAG, "Heap Caps, after start task: %d", heap_caps_check_integrity_all(true));

	msgpack_pack_array(&pk, 5);
	msgpack_pack_int(&pk, REQUEST_TYPE_CONTROL_RESULT);
	msgpack_pack_uint32(&pk, obj_req_id.via.u64);
	//ESP_LOGI(TAG, "request ID: %lli", obj_req_id.via.u64);

	if(check_task_type_push(result)) {
		// task ID exists
		msgpack_pack_true(&pk);
		msgpack_pack_uint32(&pk, extract_task_id(result));
	}
		else {
			msgpack_pack_false(&pk);
			msgpack_pack_nil(&pk);
		}

	msgpack_pack_str(&pk, strlen(result));
	msgpack_pack_str_body(&pk, result, strlen(result));

	ret = send_binary(sbuf.data, sbuf.size);
	if(ret != sbuf.size) {
		ESP_LOGI(TAG, "failed sending start task result: %i", ret);
	}
		else ESP_LOGI(TAG, "sent start task result");

	msgpack_sbuffer_destroy(&sbuf);
	free(task);
	free(result);
}

// extract task ID and stop it
void evaluate_stop_task(msgpack_object obj_req_id, msgpack_object obj_task_id) {
	msgpack_packer pk;
	msgpack_sbuffer sbuf;
	int ret;

	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	
	ESP_LOGI(TAG, "Received task ID to stop: %llu\n", obj_task_id.via.u64);

	// max uint32: 4294967295 -> 10 characters
	char *task = malloc(38+10+1);
	snprintf(task, 49, "dle=require 'dle';return dle.stop(1,%llu);", obj_task_id.via.u64);
	char *result = startLuaTask(task);

	msgpack_pack_array(&pk, 5);
	msgpack_pack_int(&pk, REQUEST_TYPE_CONTROL_RESULT);
	msgpack_pack_uint8(&pk, obj_req_id.via.u64);
	msgpack_pack_nil(&pk);
	msgpack_pack_nil(&pk);
	msgpack_pack_str(&pk, strlen(result));
	msgpack_pack_str_body(&pk, result, strlen(result));

	ret = send_binary(sbuf.data, sbuf.size);
	if(ret != sbuf.size) {
		ESP_LOGI(TAG, "failed sending start task result: %i", ret);
	}
		else printf("sent stop task result\n");

	msgpack_sbuffer_destroy(&sbuf);
	free(task);
	free(result);
}

msg_ptr_len_t evaluate_msgpack(char *msg, int length) {
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

void evaluate_array(msgpack_object *objs, int length) {
	if(objs[0].type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
		// successfully unpacked the type
		if(com_status == SENT_CLIENT_ID) {
			if(length != 2) {
				ESP_LOGE(TAG, "received msgpack array has unexpected length: %i (expected 2)", length);
				return;
			}

			// expect to get the protocol version
			if(objs[0].via.u64 == REQUEST_TYPE_PROTOCOL_VERSION) {
				evaluate_protocol_version(objs[1]);
			}
				else {
					ESP_LOGI(TAG, "expected protocol version, but request has type: %llu", objs[0].via.u64);
				}
		}
		else if(com_status == INITIALIZED)
		{
			if(length != 3) {
				ESP_LOGE(TAG, "received msgpack array has unexpected length: %i (expected 3)", length);
				return;
			}
			switch(objs[0].via.u64) {
				case REQUEST_TYPE_PROTOCOL_VERSION:
					ESP_LOGI(TAG, "protocol version was sent yet");
				break;
				case REQUEST_TYPE_START_TASK:
					evaluate_start_task(objs[1], objs[2]);
				break;
				case REQUEST_TYPE_STOP_TASK:
					evaluate_stop_task(objs[1], objs[2]);
				break;
				default:
					ESP_LOGI(TAG, "unknown type of message: %llu", objs[0].via.u64);
			}
		}
			else {
				ESP_LOGI(TAG, "unexpected status or message, status: %i, request type: %llu", com_status, objs[0].via.u64);
			}
	}
		else {
			ESP_LOGI(TAG, "type field is not a positive integer: %i", objs[0].type);
		}
}

int evaluate_request(char *msg_buffer, int msg_length) {
	//ESP_LOGI(TAG, "free space: %i", heap_caps_get_free_size(MALLOC_CAP_8BIT));
	//ESP_LOGI(TAG, "minimum free space: %i", xPortGetMinimumEverFreeHeapSize());

	msg_ptr_len_t ev_arr_ret;

    do {
    	ev_arr_ret = evaluate_msgpack(msg_buffer, msg_length);
        if(ev_arr_ret.state == MSGPACK_UNPACK_SUCCESS) {
        	if(ev_arr_ret.is_array) {
        		// received complete array
            	xEventGroupWaitBits(initiated_event_group, CLIENTID_SENT_BIT, false, true, portMAX_DELAY);
                evaluate_array(ev_arr_ret.array_objs, ev_arr_ret.array_size);
                free(ev_arr_ret.array_objs);
            }
                else {
                	ESP_LOGE(TAG, "received message is not an array");
                	msg_length = 0;
                }

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
			    }
        }
        else if(ev_arr_ret.state != MSGPACK_UNPACK_CONTINUE) {
        	// error occured
            ESP_LOGE(TAG, "error unpacking message: %i", ev_arr_ret.state);
            msg_length = 0;
        }
        // else: yet bytes missing for complete array
    } while(msg_length > 0 && ev_arr_ret.state == MSGPACK_UNPACK_SUCCESS);

    return msg_length;
}

void encode_msgpack_answer(answer_decoded_t *answer, char **msg, int *size) {
	char *answer_str;
	msgpack_packer pk;
	msgpack_sbuffer sbuf;

	answer_str = encode(answer);

	msgpack_sbuffer_init(&sbuf);
	msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);
	msgpack_pack_array(&pk, 2);
	msgpack_pack_int(&pk, REQUEST_TYPE_MESSAGE);
	msgpack_pack_str(&pk, strlen(answer_str));
	msgpack_pack_str_body(&pk, answer_str, strlen(answer_str));

	*msg = malloc(sbuf.size);
	memcpy(*msg, sbuf.data, sbuf.size);
	*size = sbuf.size;

	msgpack_sbuffer_destroy(&sbuf);
	free(answer_str);
}

void start_control_system() {
	int ret;

	printf("\n\nID: '%s'\n\n\n", CONFIG_CLIENT_ID);
	initiated_event_group = xEventGroupCreate();
	ret = open_network();
	if(ret >= 0) {
		com_status = INITIAL;
		
		// send protocol version and client id
		send_protocol_version();
		send_client_id();
	}
}
