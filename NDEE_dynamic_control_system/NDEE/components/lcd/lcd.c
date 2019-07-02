/*
 *  Created on: 18 Mar 2018
 *      Author: Alex Moroz
 */
#include "lcd.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "rom/uart.h"

#include "i2c.h"
#include "smbus.h"
#include "i2c-lcd1602.h"
#include "answer.h"
#include "sys/time.h"

const char* TAG = "LCD";

static i2c_lcd1602_info_t * lcd_info;
static smbus_info_t * smbus_info;
int status;

int init(lua_State *L) {
	i2c_port_t i2c_num = I2C_MASTER_NUM;
	uint8_t address = CONFIG_LCD1602_I2C_ADDRESS;

	// Set up the SMBus
	smbus_info = smbus_malloc();
	smbus_init(smbus_info, i2c_num, address);
	smbus_set_timeout(smbus_info, 1000 / portTICK_RATE_MS);

	// Set up the LCD1602 device with backlight off
	lcd_info = i2c_lcd1602_malloc();
	i2c_lcd1602_init(lcd_info, smbus_info, true);
	return 0;
}

int lcd_set_pointer(lua_State *L) {
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);

	i2c_lcd1602_set_cursor(lcd_info, true);
	i2c_lcd1602_move_cursor(lcd_info, x, y);

	return 0;
}

void write_value (char *s)
{
    //i2c_lcd1602_write_string(lcd_info, s);
    ESP_LOGI(TAG, "%s", s);
}

int check_integer (lua_State *L, char *key)
{
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    int result = -1;
    status = 5;
    if (lua_isinteger (L, -1)) {
        result = lua_tointeger(L, -1);
        status = 0;
    }
    lua_settop(L, 2);
    return result;
}

char *check_string (lua_State *L, char *key)
{
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    char *result = "";
    status = 5;
    if (lua_isstring (L, -1)) {
        result = (char *)lua_tostring(L, -1);
        status = 0;
    }
    lua_settop(L, 4);
    return result;
}

float check_float (lua_State *L, char *key)
{
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    float result = -1;
    status = 5;
    if (lua_isnumber (L, -1)) {
        result = lua_tonumber(L, -1);
        status = 0;
    }
    lua_settop(L, 4);
    return result;
}

answer_decoded_t *error (int status)
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

answer_decoded_t *okay (int requestid)
{
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


answer_decoded_t *read (lua_State *L) {

    answer_decoded_t *request;
    request = answer_decoded_new();
    int requestid;
    char *s;
    s = malloc(sizeof("")+1);
    answer_decoded_t *result;
    
    if (lua_gettop(L) != 2){
        result = error(5);
        return result;
    }
    
    if (!lua_isinteger(L, 1)) {
        result = error(5);
        return result;
    }
    requestid = luaL_checkinteger(L, 1);
    
    if (!lua_istable(L, -1)) {
        result = error(5);
        return result;
    }
    
    (*request).status = check_integer(L, "status");
    if (status == 5) {
        result = error(status);
        return result;
    }
    
    if ((*request).status == 1) {
        (*request).measurand = check_string(L, "measurand");
        if (status == 5) {
            result = error(status);
            return result;
        }
        (*request).unit = check_string(L, "unit");
        if (status == 5) {
            result = error(status);
            return result;
        }
        (*request).value = check_float(L, "value");
        if (status == 5) {
            result = error(status);
            return result;
        }
        sprintf(s, "%s: %f %s", (*request).measurand, (*request).value, (*request).unit);
        write_value(s);
        result = okay(requestid);
        return result;
    }
    result = error(6);
    return result;
}

int lcd_write(lua_State *L) {
    answer_decoded_t *answer;
    answer = read (L);
    
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

int lcd_clear(lua_State *L) {
	i2c_lcd1602_clear(lcd_info);
	return 0;
}

static const struct luaL_Reg lcd[] = {
		{ "init", init },
		{ "set", lcd_set_pointer},
		{ "write", lcd_write},
		{ "clear", lcd_clear},
		{ NULL, NULL } /* sentinel */
};

int luaopen_lcd1602 (lua_State *L) {
        luaL_newlib(L, lcd);
        return 1;
}
