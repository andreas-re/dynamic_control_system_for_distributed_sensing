#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "stop.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "taskmanager.h"
#include "sys/time.h"

static const char *TAG = "STOP";

producers_t *producers;
cconsumers_t *cconsumers;

EventGroupHandle_t *request_event_group;
int result = 0;

answer_decoded_t *stop_task(lua_State *L)
{
    answer_decoded_t *answer;
    answer = answer_decoded_new();
    
    if((lua_gettop(L) != 2) || !lua_isinteger(L, 1) || !lua_isinteger(L, 2)) {
        lua_settop(L, 0);
        result = 5;
        (*answer).status = result;
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
    int taskid;
    requestid = luaL_checkinteger(L, 1);
    taskid = luaL_checkinteger(L, 2);
    lua_settop(L, 0);
    
    if(taskid < 0) {
        result = 6;
        (*answer).status = result;
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
    result = 0;
    
    change_producer_duration(producers, taskid, 0);
    change_cconsumer_duration(cconsumers, taskid, 0);
    
    (*answer).status = result;
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

int stop(lua_State *L)
{
    
    request_event_group = xEventGroupCreate();

    answer_decoded_t *answer;
    
    answer = stop_task(L);
    
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
