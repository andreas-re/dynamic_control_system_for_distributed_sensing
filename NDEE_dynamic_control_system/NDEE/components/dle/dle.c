#include "string.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "i2c.h"
#include "stack.h"
#include "wifi.h"
#include "dle.h"
#include "queue.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "modules.h"
#include "sb.h"
#include "request.h"
#include "actuatorreplystack.h"
#include "netreplystack.h"
#include "taskmanager.h"
#include "bmp180_lua.h"
#include "controlsystem.h" // changed for control system
#include "stop.h"
#include "bme280_lua.h"

#include "gas.h"
#include "ultrasonic.h"
#include "unistd.h"

static const char *TAG = "DLE";

//EventGroupHandle_t *queue_event_group;
//int currentClient; // changed for controls system


//Managing tasks, stacks, clients
mapped_stacks_t *stacks;
producers_t *producers;
producer_clients_t *producer_clients;
aconsumers_t *aconsumers;
aconsumer_clients_t *aconsumer_clients;
cconsumers_t *cconsumers;
int stackId;
int taskId;

lua_State *L;

/*
 Auxiliary functions:
    initLuaLibraries: Open libraries for initialized Lua state
    demandLuaInterpreter: Execute Lua code
 */
void initLuaLibraries (lua_State *L)
{
    luaL_openlibs(L);
    luaopen_base(L);
    luaopen_io(L);
    luaopen_string(L);
    luaopen_math(L);
    openlibs(L);
}

answer_decoded_t *demandLuaInterpreter (lua_State *L, char *line)
{
    //int ret = heap_caps_check_integrity_all(true);
    //ESP_LOGI(TAG, "Heap Caps, start demandLuaInterpreter: %d", ret);
    //Initialize String builder
    StringBuilder *sb = sb_create();
    sb_append(sb, line, 0);
    sb_append(sb, " ", 0);
    char *string_result = sb_concat(sb);
    
    //Execute Lua line by utilizing the Lua-C-API
    int result;
    answer_decoded_t *answer_decoded;
    answer_decoded = answer_decoded_new();
    if(!luaL_loadstring(L, string_result)) {
        //ret = heap_caps_check_integrity_all(true);
        //ESP_LOGI(TAG, "Heap Caps, before luaL_dostring: %d", ret);
        result = luaL_dostring(L, string_result);
        //ret = heap_caps_check_integrity_all(true);
        //ESP_LOGI(TAG, "Heap Caps, after luaL_dostring: %d", ret);
        if (lua_isfunction(L, -1)){
            (*answer_decoded).status = 7;
            (*answer_decoded).requestid = 0;
            time_t now;
            time(&now);
            (*answer_decoded).timestamp = now;
            (*answer_decoded).measurand = "";
            (*answer_decoded).device = "";
            (*answer_decoded).unit = "";
            (*answer_decoded).type = "";
            (*answer_decoded).value = 0;
            sb_reset(sb);
            //free(line); // test for heap error
            lua_settop(L, 4);
            return answer_decoded;
        }
        if (result == 0) {
            lua_pushstring(L, "status");
            lua_gettable(L, -2);
            (*answer_decoded).status = lua_tointeger(L, -1);
            lua_settop(L, 6);
        
            lua_pushstring(L, "requestid");
            lua_gettable(L, -2);
            (*answer_decoded).requestid = lua_tointeger(L, -1);
            lua_settop(L, 6);
        
            lua_pushstring(L, "measurand");
            lua_gettable(L, -2);
            (*answer_decoded).measurand = (char *)lua_tostring(L, -1);
            lua_settop(L, 6);
        
            lua_pushstring(L, "device");
            lua_gettable(L, -2);
            (*answer_decoded).device = (char *)lua_tostring(L, -1);
            lua_settop(L, 6);
        
            lua_pushstring(L, "unit");
            lua_gettable(L, -2);
            (*answer_decoded).unit = (char *)lua_tostring(L, -1);
            lua_settop(L, 6);
        
            lua_pushstring(L, "type");
            lua_gettable(L, -2);
            (*answer_decoded).type = (char *)lua_tostring(L, -1);
            lua_settop(L, 6);
        
            lua_pushstring(L, "timestamp");
            lua_gettable(L, -2);
            (*answer_decoded).timestamp = lua_tointeger(L, -1);
            lua_settop(L, 6);
        
            lua_pushstring(L, "value");
            lua_gettable(L, -2);
            (*answer_decoded).value = (float)lua_tonumber(L, -1);
            lua_settop(L, 4);
            
            sb_reset(sb);
            // free(line); // test for heap error
        
            return answer_decoded;
        }
    }
    (*answer_decoded).status = 3;
    (*answer_decoded).requestid = 0;
    time_t now;
    time(&now);
    (*answer_decoded).timestamp = now;
    (*answer_decoded).measurand = "";
    (*answer_decoded).device = "";
    (*answer_decoded).unit = "";
    (*answer_decoded).type = "";
    (*answer_decoded).value = 0;
    sb_reset(sb);
    //free(line); // test for heap error
    return answer_decoded;
}

char *startLuaTask(char *task) {
    answer_decoded_t *answer_decoded;
    char *result;
    answer_decoded = demandLuaInterpreter(L, task);

    result = encode(answer_decoded);
    free(answer_decoded);
    //Encode answer to bitstring
    return result;
}

void startLuaEnvironment() {
    L = luaL_newstate();
    initLuaLibraries(L);
    init_i2c();
    /*ESP_LOGI(TAG, "Start test of BME");
    test_bme280();
    ESP_LOGI(TAG, "End test of BME");*/
    stacks = mapped_stacks_new ();
    producers = producers_new();
    producer_clients = producer_clients_new();
    aconsumers = aconsumers_new();
    aconsumer_clients = aconsumer_clients_new();
    cconsumers = cconsumers_new();
   
    stackId = 0;
    taskId = 0;
    
    ESP_LOGI(TAG, "Started dle");
}

void execute() {
    startLuaEnvironment();
    start_control_system();    
}

static const struct luaL_Reg dle [] = {
    {"temperature", bmp180_temperature},
    {"pressure", bmp180_pressure},
    {"co2", gassensor_co2},
    //{"lcd", lcd_stack},
    //{"net_value", net_value},
    {"net", net_stack},
    {"net_dynamic", net_stack_us}, // added for demo
    {"stop", stop},
    {NULL, NULL}  /* sentinel */
};

int luaopen_dle (lua_State *L) {
    luaL_newlib(L, dle);
    return 1;
}
