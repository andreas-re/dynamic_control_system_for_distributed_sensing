#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "bmp180_lua.h"
#include "gas.h"
#include "stack.h"
#include "request.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "taskmanager.h"
#include "sys/time.h"

static const char *TAG = "REQUEST";

int currentClient;

mapped_stacks_t *stacks;
producers_t *producers;
producer_clients_t *producer_clients;

EventGroupHandle_t *request_event_group;
int status = 2;

/*
 Execute time based task
 */
void vTaskRead(void *pvParameter)
{
    TickType_t lastStart;
    lastStart = xTaskGetTickCount();
    //UBaseType_t uxHighWaterMark;
    
    /* Inspect our own high water mark on entering the task. */
    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    int counter;
    counter = 0;
    request_config_t request = *(request_config_t *) pvParameter;
    int frequency = request.frequency;
    int duration = request.duration;
    int requestid = request.requestid;
    while (duration < 0 || counter < duration){
        answer_decoded_t *answer;
        answer= (*request.handle_sensor) ();
        (*answer).requestid = requestid;
        push(request.handle_stack, answer);
    
        xEventGroupSetBits(request_event_group, BIT0);
        vTaskDelayUntil( &lastStart, frequency );
        //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        //ESP_LOGI(TAG, "Mark: %d", uxHighWaterMark);
        counter++;
        
        frequency = frequency_by_producer (producers, request.taskid);
        duration = duration_by_producer(producers, request.taskid);
        ESP_LOGI(TAG, "This task has a duration( %d ) and a frequency( %d ).", duration, frequency);
    }
    vTaskDelete(NULL);
}

answer_decoded_t *read_request(lua_State *L, answer_decoded_t *(*handle_sensor) (lua_State *, stack_t *), char *measurand)
{
    answer_decoded_t *answer;
    answer = answer_decoded_new();
    
    if((lua_gettop(L) != 3) || !lua_isinteger(L, 1) || !lua_isinteger(L, 2) || !lua_isinteger(L, 3)) {
        lua_settop(L, 0);
        status = 5;
        (*answer).status = status;
        (*answer).requestid = 0;
        
        struct timeval time;
        gettimeofday(&time, NULL);
        unsigned long long now = (unsigned long long)(time.tv_sec) * 1000 + (unsigned long long)(time.tv_usec) / 1000;
        (*answer).timestamp = now;
        
        (*answer).measurand = "";
        (*answer).device = "";
        
        (*answer).unit = "";
        (*answer).type = "";
        (*answer).value = 0;
        return answer;
    }
    
    int requestid;
    int frequency;
    int duration;
    requestid = luaL_checkinteger(L, 1);
    frequency = luaL_checkinteger(L, 2);
    duration = luaL_checkinteger(L, 3);
    lua_settop(L, 0);
    
    if(frequency <= 0) {
        status = 6;
        (*answer).status = status;
        (*answer).requestid = requestid;
        struct timeval time;
        gettimeofday(&time, NULL);
        unsigned long long now = (unsigned long long)(time.tv_sec) * 1000 + (unsigned long long)(time.tv_usec) / 1000;
        (*answer).timestamp = now;
        (*answer).measurand = "";
        (*answer).device = "";
        (*answer).unit = "";
        (*answer).type = "";
        (*answer).value = 0;
        return answer;
    }
    status = 2;
    
    int stackid;
    int clientid;
    int taskid;
    
    //Start a new task, do nothing, or change an existing task
    stackid = stackid_by_measurand (stacks, measurand);

    if (stackid < 0) {
        stack_t *stack = stack_new ();
        stackid = insert_stack(stacks, measurand, stack);
    }

    taskid = producer_by_stack(producers, stackid);
    if(taskid >= 0) ESP_LOGI(TAG, "taskID of sensor reading: %i", taskid);
    
    if (taskid < 0){
        ESP_LOGI(TAG, "Heap Caps, before inserting: %d", heap_caps_check_integrity_all(true));
        taskid = insert_producer_client (producers, producer_clients, frequency, duration, stackid, measurand, currentClient);
        ESP_LOGI(TAG, "taskID of sensor reading: %i", taskid);

        request_config_t currentRequest;
        currentRequest.taskid = taskid;
        currentRequest.handle_stack = stack_by_stackid (stacks, stackid);
        currentRequest.frequency = frequency;
        currentRequest.duration = duration;
        //currentRequest.handle_sensor = &bmp180_get_answer_temperature;
        currentRequest.handle_sensor = handle_sensor;
        currentRequest.requestid = requestid;
        
        xTaskCreate( vTaskRead, "Reading Task", 5000, (void*) &currentRequest, 10, NULL);
        xEventGroupWaitBits(request_event_group,BIT0,false,true,portMAX_DELAY);
        
    }
    else {
        int givenFrequency;
        int givenDuration;
        givenFrequency = frequency_by_producer(producers, taskid);
        givenDuration = duration_by_producer(producers, taskid);
        if(givenFrequency > frequency){
            change_producer_frequency(producers, taskid, frequency);
        }
        if (((givenDuration >= 0) && (givenDuration < duration)) || (duration < 0)) {
            change_producer_duration(producers, taskid, duration);
        }
        insert_client_for_producer (producer_clients, taskid, currentClient);
    }
    
    (*answer).status = status;
    (*answer).requestid = requestid;
    struct timeval time;
    gettimeofday(&time, NULL);
    unsigned long long now = (unsigned long long)(time.tv_sec) * 1000 + (unsigned long long)(time.tv_usec) / 1000;
    (*answer).timestamp = now;
    (*answer).unit = "";
    (*answer).type = "INTEGER";
    (*answer).value = taskid;
    
    
    return answer;
}

int bmp180_temperature(lua_State *L)
{
    
    request_event_group = xEventGroupCreate();

    answer_decoded_t *answer;
    answer = read_request(L, &bmp180_get_answer_temperature, "TEMPERATURE");
    (*answer).measurand = "TEMPERATURE";
    (*answer).device = "BMP180";
    
    lua_newtable(L);
    
    lua_pushstring(L, "status");
    lua_pushinteger(L, (*answer).status);
    lua_settable(L, 1);
    
    lua_pushstring(L, "requestid");
    lua_pushinteger(L, (*answer).requestid);
    lua_settable(L, 1);
    
    lua_pushstring(L, "timestamp");
    lua_pushinteger(L, (*answer).timestamp);
    lua_settable(L, 1);
    
    lua_pushstring(L, "measurand");
    lua_pushstring(L, (*answer).measurand);
    lua_settable(L, 1);
    
    lua_pushstring(L, "device");
    lua_pushstring(L, (*answer).device);
    lua_settable(L, 1);
    
    lua_pushstring(L, "unit");
    lua_pushstring(L, (*answer).unit);
    lua_settable(L, 1);
    
    lua_pushstring(L, "type");
    lua_pushstring(L, (*answer).type);
    lua_settable(L, 1);
    
    lua_pushstring(L, "value");
    lua_pushnumber(L, (*answer).value);
    lua_settable(L, 1);

    return 1;
}

int bmp180_pressure(lua_State *L)
{
    request_event_group = xEventGroupCreate();
    answer_decoded_t *answer;
    answer = read_request(L, &bmp180_get_answer_pressure, "AIRPRESSURE");
    (*answer).measurand = "AIRPRESSURE";
    (*answer).device = "BMP180";
    
    lua_newtable(L);
    
    lua_pushstring(L, "status");
    lua_pushinteger(L, (*answer).status);
    lua_settable(L, 1);
    
    lua_pushstring(L, "requestid");
    lua_pushinteger(L, (*answer).requestid);
    lua_settable(L, 1);
    
    lua_pushstring(L, "timestamp");
    lua_pushinteger(L, (*answer).timestamp);
    lua_settable(L, 1);
    
    lua_pushstring(L, "measurand");
    lua_pushstring(L, (*answer).measurand);
    lua_settable(L, 1);
    
    lua_pushstring(L, "device");
    lua_pushstring(L, (*answer).device);
    lua_settable(L, 1);
    
    lua_pushstring(L, "unit");
    lua_pushstring(L, (*answer).unit);
    lua_settable(L, 1);
    
    lua_pushstring(L, "type");
    lua_pushstring(L, (*answer).type);
    lua_settable(L, 1);
    
    lua_pushstring(L, "value");
    lua_pushnumber(L, (*answer).value);
    lua_settable(L, 1);
    
    //dump(stackidmap, stack_task_map, task_durationfrequency_map, task_client_map);
    
    return 1;
}

int gassensor_co2(lua_State *L)
{
    request_event_group = xEventGroupCreate();
    answer_decoded_t *answer;
    answer = read_request(L, &gassensor_get_answer_gas, "GAS");
    (*answer).measurand = "CO2";
    (*answer).device = "GASSENSOR";
    
    lua_newtable(L);
    
    lua_pushstring(L, "status");
    lua_pushinteger(L, (*answer).status);
    lua_settable(L, 1);
    
    lua_pushstring(L, "requestid");
    lua_pushinteger(L, (*answer).requestid);
    lua_settable(L, 1);
    
    lua_pushstring(L, "timestamp");
    lua_pushinteger(L, (*answer).timestamp);
    lua_settable(L, 1);
    
    lua_pushstring(L, "measurand");
    lua_pushstring(L, (*answer).measurand);
    lua_settable(L, 1);
    
    lua_pushstring(L, "device");
    lua_pushstring(L, (*answer).device);
    lua_settable(L, 1);
    
    lua_pushstring(L, "unit");
    lua_pushstring(L, (*answer).unit);
    lua_settable(L, 1);
    
    lua_pushstring(L, "type");
    lua_pushstring(L, (*answer).type);
    lua_settable(L, 1);
    
    lua_pushstring(L, "value");
    lua_pushnumber(L, (*answer).value);
    lua_settable(L, 1);
    
    return 1;
}
