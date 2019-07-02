#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "stack.h"
#include "wifi.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "netreplystack.h"
#include "taskmanager.h"
#include "sys/time.h"

#include "netreplystack_ultrasonic.h"

static const char *TAG = "NETREPLYSTACK";

int currentClient;
int status;

mapped_stacks_t *stacks;
cconsumers_t *cconsumers;
producers_t *producers;
EventGroupHandle_t *net_event_group;

void vTaskWriteNetStack(void *pvParameter)
{
    TickType_t lastStart;
    lastStart = xTaskGetTickCount();
    //UBaseType_t uxHighWaterMark;
    
    /* Inspect our own high water mark on entering the task. */
    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    int counter;
    counter = 0;
    answer_decoded_t *top;
    net_reply_stack_config_t reply = *(net_reply_stack_config_t *) pvParameter;
    int duration;
    int frequency;
    int taskid;
    int ultrasonic_distance;
    int ultrasonic_threshold = reply.dynamic_threshold;
    duration = reply.duration;
    frequency = reply.frequency;
    taskid = reply.taskid;
    while (duration < 0 || counter < duration){
        top = pop(reply.reply_stack);
        //(*reply.write_value) (reply.receiver, top);
        if(reply.mode == NETREPLYSTACK_MODE_NORMAL) {
            answer(currentClient, top);
        }
        else if(reply.mode == NETREPLYSTACK_MODE_ULTRASONIC) {
            ultrasonic_distance = ultrasonic_read();
            ESP_LOGI(TAG, "ultrasonic distance in netreplystack: %i, threshold: %i", ultrasonic_distance, ultrasonic_threshold);
            if(ultrasonic_distance < ultrasonic_threshold) answer(currentClient, top);
        }
        xEventGroupSetBits(net_event_group, BIT0);
        vTaskDelayUntil( &lastStart, frequency );
        frequency = frequency_by_cconsumer (cconsumers, taskid);
        duration = duration_by_cconsumer (cconsumers, taskid);
        //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        //ESP_LOGI(TAG, "Mark: %d", uxHighWaterMark);
        counter++;
    }
    vTaskDelete(NULL);
}

int check_i (lua_State *L, char *key, net_reply_stack_mode mode)
{
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    int result = -1;
    status = 5;
    if (lua_isnumber (L, -1)) {
        result = lua_tointeger(L, -1);
        status = 0;
    }
    if(mode == NETREPLYSTACK_MODE_NORMAL) lua_settop(L, 4);
    else if(mode == NETREPLYSTACK_MODE_ULTRASONIC) lua_settop(L, 5);
    return result;
}

answer_decoded_t *e (int status)
{
    answer_decoded_t *answer;
    answer = answer_decoded_new();
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

answer_decoded_t *o (int requestid, int taskid)
{
    answer_decoded_t *answer;
    answer = answer_decoded_new();
    (*answer).status = 2;
    (*answer).requestid = requestid;
    struct timeval time;
    gettimeofday(&time, NULL);
    unsigned long long now = (unsigned long long)(time.tv_sec) * 1000 + (unsigned long long)(time.tv_usec) / 1000;
    (*answer).timestamp = now;
    (*answer).measurand = "";
    (*answer).device = "";
    (*answer).unit = "";
    (*answer).type = "INTEGER";
    (*answer).value = taskid;
    return answer;
}

static void stackDump (lua_State *L) {
    int i;
    int top = lua_gettop(L);
    for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {

            case LUA_TSTRING:  /* strings */
                printf("`%s'", lua_tostring(L, i));
                break;

            case LUA_TBOOLEAN:  /* booleans */
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;

            case LUA_TNUMBER:  /* numbers */
                printf("%g", lua_tonumber(L, i));
                break;

            default:  /* other values */
                printf("%s", lua_typename(L, t));
                break;

        }
        printf("  ");  /* put a separator */
    }
    printf("\n");  /* end the listing */
}


answer_decoded_t *net_write_stack(lua_State *L, int (*write_value) (int, answer_decoded_t *), net_reply_stack_mode mode) {
    ESP_LOGI(TAG, "Starting net_stack_write method.");
    answer_decoded_t *result;
    ESP_LOGI(TAG, "StackDump: ");
    stackDump(L);
    
    if(mode == NETREPLYSTACK_MODE_NORMAL) {
        if ((lua_gettop(L) != 4) || (!lua_isinteger(L, 1)) || (!lua_isinteger(L, 2)) || (!lua_isinteger(L, 3)) || (!lua_istable(L, 4))) {
            result = e(5);
            return result;
        }
    }
    else if(mode == NETREPLYSTACK_MODE_ULTRASONIC) {
        if ((lua_gettop(L) != 5) || (!lua_isinteger(L, 1)) || (!lua_isinteger(L, 2)) || (!lua_isinteger(L, 3)) || (!lua_isinteger(L, 4)) || (!lua_istable(L, 5))) {
            result = e(5);
            return result;
        }
    }

    int requestid;
    int frequency;
    int duration;
    int taskid;
    int stackid;
    int requeststatus;
    int dynamic_threshold = (-1);
    
    requestid = luaL_checkinteger(L, 1);
    frequency = luaL_checkinteger(L, 2);
    duration = luaL_checkinteger(L, 3);
    if(mode == NETREPLYSTACK_MODE_ULTRASONIC) dynamic_threshold = luaL_checkinteger(L, 4);
    
    requeststatus = check_i(L, "status", mode);
    if (status == 5) {
        return e(status);
    }
    
    if (requeststatus == 2) {
        taskid = check_i(L, "value", mode);
        if (status == 5) {
            return e(status);
        }
        
        if ((frequency <= 0) || (taskid < 0)) {
            return e(6);
        }
        
        stackid = stack_by_producer(producers, taskid);
        if (stackid < 0) {
            return e(8);
        }
        
        status = 0;
        
        taskid = insert_cconsumer (cconsumers, frequency, duration, stackid, currentClient);
        ESP_LOGI(TAG, "taskID of netreplystack: %i\n", taskid);
        net_reply_stack_config_t currentReply;
        currentReply.write_value = write_value;
        currentReply.receiver = currentClient;
        currentReply.reply_stack = stack_by_stackid (stacks, stackid);
        currentReply.frequency = frequency;
        currentReply.duration = duration;
        currentReply.taskid = taskid;
        currentReply.mode = mode;
        currentReply.dynamic_threshold = dynamic_threshold;
            
        xTaskCreate( vTaskWriteNetStack, "WriteStack Task", 5000, (void*) &currentReply, 10, NULL);
        xEventGroupWaitBits(net_event_group,BIT0,false,true,portMAX_DELAY);
        lua_settop(L, 0);
        return o(requestid, taskid);
    }
    
    return e(6);
}


int net_stack(lua_State *L)
{
    ESP_LOGI(TAG, "Starting net_stack method.");
    net_event_group = xEventGroupCreate();
    answer_decoded_t *answer;
    answer = net_write_stack(L, &answer, NETREPLYSTACK_MODE_NORMAL);
    
    lua_settop(L, 0);
    lua_newtable(L);
    
    lua_pushstring(L, "status");
    lua_pushinteger(L, (*answer).status);
    lua_settable(L, 1);
    
    lua_pushstring(L, "requestid");
    lua_pushinteger(L, (*answer).requestid);
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
    
    lua_pushstring(L, "timestamp");
    lua_pushinteger(L, (*answer).timestamp);
    lua_settable(L, 1);
    
    lua_pushstring(L, "value");
    lua_pushnumber(L, (*answer).value);
    lua_settable(L, 1);
    
    return 1;
}

int net_stack_us(lua_State *L)
{
    ESP_LOGI(TAG, "Starting net_stack method.");
    net_event_group = xEventGroupCreate();
    answer_decoded_t *answer;
    answer = net_write_stack(L, &answer, NETREPLYSTACK_MODE_ULTRASONIC);
    
    lua_settop(L, 0);
    lua_newtable(L);
    
    lua_pushstring(L, "status");
    lua_pushinteger(L, (*answer).status);
    lua_settable(L, 1);
    
    lua_pushstring(L, "requestid");
    lua_pushinteger(L, (*answer).requestid);
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
    
    lua_pushstring(L, "timestamp");
    lua_pushinteger(L, (*answer).timestamp);
    lua_settable(L, 1);
    
    lua_pushstring(L, "value");
    lua_pushnumber(L, (*answer).value);
    lua_settable(L, 1);
    
    return 1;
}
