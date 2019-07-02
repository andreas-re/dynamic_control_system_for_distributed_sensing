#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "stack.h"
#include "lcd.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "actuatorreplystack.h"
#include "taskmanager.h"
#include "sys/time.h"

static const char *TAG = "REPLYSTACK";


mapped_stacks_t *stacks;
aconsumers_t *aconsumers;
aconsumer_clients_t *aconsumer_clients;

int stat = 0;
int currentClient;

void vTaskWriteActuatorStack(void *pvParameter)
{
    TickType_t lastStart;
    lastStart = xTaskGetTickCount();
    //UBaseType_t uxHighWaterMark;
    
    /* Inspect our own high water mark on entering the task. */
    //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    int counter;
    counter = 0;
    int top;
    actuator_reply_stack_config_t reply = *(actuator_reply_stack_config_t *) pvParameter;
    int frequency;
    int duration;
    frequency = reply.frequency;
    duration = reply.duration;
    while (duration == -1 || counter < duration){
        
        top = pop(reply.reply_stack);
        (*reply.write_value) (top);
        vTaskDelayUntil( &lastStart, frequency );
        //uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        //ESP_LOGI(TAG, "Mark: %d", uxHighWaterMark);
        counter++;
        frequency = frequency_by_aconsumer (aconsumers, reply.taskid);
        duration = duration_by_aconsumer (aconsumers, reply.taskid);
    }
    vTaskDelete(NULL);
}

int check_int (lua_State *L, char *key)
{
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    int result = -1;
    stat = 5;
    if (lua_isinteger (L, -1)) {
        result = lua_tointeger(L, -1);
        stat = 0;
    }
    lua_settop(L, 4);
    return result;
}

answer_decoded_t *err (int statusno)
{
    answer_decoded_t *answer;
    answer = answer_decoded_new();
    (*answer).status = statusno;
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

answer_decoded_t *ok (int requestid)
{
    ESP_LOGI(TAG, "Was okay");
    answer_decoded_t *answer;
    answer = answer_decoded_new();
    (*answer).status = 0;
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

answer_decoded_t *actuator_write_stack(lua_State *L, int (*write_value) (int), char *actuator)
{
    answer_decoded_t *result;
    
    if ((lua_gettop(L) != 4) || (!lua_isinteger(L, 1)) || (!lua_isinteger(L, 2)) || (!lua_isinteger(L, 3)) || (!lua_istable(L, 4))) {
        ESP_LOGI(TAG, "111");
        result = err(5);
        return result;
    }
    
    int requestid;
    int frequency;
    int duration;
    int stackid;
    int requeststatus;
    
    requestid = luaL_checkinteger(L, 1);
    frequency = luaL_checkinteger(L, 2);
    duration = luaL_checkinteger(L, 3);
    
    requeststatus = check_int(L, "status");
    if (stat == 5) {
        ESP_LOGI(TAG, "128");
        return err(stat);
    }
    
    if (requeststatus == 2) {
        stackid = check_int(L, "value");
        if (stat == 5) {
            ESP_LOGI(TAG, "135");
            return err(stat);
        }
        
        if ((frequency <= 0) || (stackid < 0)) {
            return err(6);
        }
        
        bool doesStackExist;
        doesStackExist = exists_stack (stacks, stackid);
        if (!doesStackExist) {
            return err(8);
        }
        
        stat = 0;
        
        int taskid;
        taskid = aconsumer_by_actuator (aconsumers, actuator);
        
        if (taskid == -1) {
            taskid = insert_aconsumer_client (aconsumers, aconsumer_clients, frequency, duration, stackid, actuator, currentClient);
            
            actuator_reply_stack_config_t currentReply;
            currentReply.write_value = write_value;
            currentReply.reply_stack = stack_by_stackid (stacks, stackid);
            currentReply.frequency = frequency;
            currentReply.duration = duration;
            currentReply.taskid = taskid;
            
            xTaskCreate( vTaskWriteActuatorStack, "WriteStack Task", 8000, (void*) &currentReply, 10, NULL);
            lua_settop(L, 0);
        }
        else {
            int currentFrequency;
            int currentDuration;
            currentFrequency = frequency_by_aconsumer (aconsumers, taskid);
            currentDuration = duration_by_aconsumer (aconsumers, taskid);
            if (currentFrequency > frequency) {
                change_aconsumer_frequency (aconsumers, taskid, frequency);
            }
            if ((currentDuration < duration) && (currentDuration >= 0)) {
                change_aconsumer_duration (aconsumers, taskid, duration);
            }
            
            insert_client_for_aconsumer (aconsumer_clients, taskid, currentClient);
        }
        
        return ok(requestid);
    }
    
    return err(6);
}

int lcd_stack(lua_State *L)
{
    answer_decoded_t *answer;
    answer = actuator_write_stack(L, &lcd_write, "LCD");
    
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
