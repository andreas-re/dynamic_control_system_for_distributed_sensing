/*
 Copyright (c) 2017 Teemu Kärkkäinen
 */
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "bmp180.h"
#include "stack.h"
#include "bmp180_lua.h"
#include "sys/time.h"

static const char* TAG = "BMP180";

uint16_t raw_temperature;
float temperature = 0.0;
int status_tmp = 1;

int64_t pressure = 0;
int status_pressure = 1;

void bmp180_access_temperature() {
    bmp180_eeprom_t bmp180_eeprom = { 0 };
    esp_err_t result;
    int32_t tmp;
    result = bmp180_read_eeprom( I2C_NUM_0, &bmp180_eeprom );
    if ( result == ESP_OK ) {
        status_tmp = 1;
    } else {
        status_tmp = 4;
        return;
    }
    result = bmp180_read_raw_temperature( I2C_NUM_0, &raw_temperature );
    if ( result == ESP_OK ) {
        tmp = bmp180_true_temperature( raw_temperature, bmp180_eeprom );
        temperature = tmp / 10.0;
        ESP_LOGI(TAG, "Temperature: %f °C", temperature);
        status_tmp = 1;
    } else {
        status_tmp = 4;
    }
}

void bmp180_access_pressure() {
    bmp180_eeprom_t bmp180_eeprom = { 0 };
    esp_err_t result;
    result = bmp180_read_eeprom( I2C_NUM_0, &bmp180_eeprom );
    if ( result == ESP_OK ) {
        status_pressure = 1;
    } else {
        status_pressure = 4;
        return;
    }
    uint16_t raw_temperature = 0;
    result = bmp180_read_raw_temperature( I2C_NUM_0, &raw_temperature );
    if ( result == ESP_OK ) {
        status_pressure = 1;
    } else {
        status_pressure = 4;
        return;
    }
    bmp180_oversampling_t sampling = BMP180_SAMPLING_HIGH;
    uint32_t raw_pressure = 0;
    result = bmp180_read_raw_pressure( I2C_NUM_0, sampling, &raw_pressure );
    if ( result == ESP_OK ) {
        int64_t pressure = bmp180_true_pressure( raw_pressure, raw_temperature, bmp180_eeprom,
                                                sampling );
        status_pressure = 1;
        ESP_LOGI(TAG, "Pressure: %lld In. Hg.", pressure);
    } else {
        status_pressure = 4;
    }
}

answer_decoded_t *bmp180_get_answer_temperature ()
{
    bmp180_access_temperature();
    //temperature = 20.0;
    status_tmp = 1;
    answer_decoded_t *answer_temperature;
    answer_temperature = answer_decoded_new();
    (*answer_temperature).status = status_tmp;
    (*answer_temperature).measurand = "TEMPERATURE";
    (*answer_temperature).device = "BMP180";
    struct timeval time;
    gettimeofday(&time, NULL);
    unsigned long long now = (unsigned long long)(time.tv_sec) * 1000 + (unsigned long long)(time.tv_usec) / 1000;
    (*answer_temperature).timestamp = now;
    (*answer_temperature).value = temperature;
    if (status_tmp == 1) {
        (*answer_temperature).unit = "DEGC";
        (*answer_temperature).type = "FLOAT";
    }
    else {
        (*answer_temperature).unit = "";
        (*answer_temperature).type = "";
    }
    
    return answer_temperature;
}

answer_decoded_t *bmp180_get_answer_pressure ()
{
    bmp180_access_pressure();
    
    answer_decoded_t *answer_pressure;
    answer_pressure = answer_decoded_new();
    
    (*answer_pressure).status = status_pressure;
    (*answer_pressure).measurand = "AIRPRESSURE";
    (*answer_pressure).device = "BMP180";
    struct timeval time;
    gettimeofday(&time, NULL);
    unsigned long long now = (unsigned long long)(time.tv_sec) * 1000 + (unsigned long long)(time.tv_usec) / 1000;
    (*answer_pressure).timestamp = now;
    (*answer_pressure).value = pressure;
    
    if (status_pressure == 1){
        (*answer_pressure).unit = "BAR";
        (*answer_pressure).type = "INTEGER";
    }
    else {
        (*answer_pressure).unit = "";
        (*answer_pressure).type = "";
    }
    
    return answer_pressure;
}

int lua_get_temperature (lua_State *L)
{
    answer_decoded_t *answer;
    if((lua_gettop(L) != 1) || !lua_isinteger(L, -1)) {
        status_tmp = 5;
        answer = answer_decoded_new();
        (*answer).status = status_tmp;
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
    }
    else {
        int requestid;
        requestid = luaL_checkinteger(L, -1);
        answer = bmp180_get_answer_temperature();
        (*answer).requestid = requestid;
    }
    
    lua_settop(L, 0);
    
    lua_newtable(L);
    
    lua_pushstring(L, "status");
    lua_pushinteger(L, (*answer).status);
    //ESP_LOGI(TAG, "Status: %d", (*answer).status);
    lua_settable(L, 1);
    
    lua_pushstring(L, "requestid");
    lua_pushinteger(L, (*answer).requestid);
    //ESP_LOGI(TAG, "RequestId: %d", (*answer).requestid);
    lua_settable(L, 1);
    
    lua_pushstring(L, "measurand");
    lua_pushstring(L, (*answer).measurand);
    //ESP_LOGI(TAG, "Measurand: %s", (*answer).measurand);
    lua_settable(L, 1);
    
    lua_pushstring(L, "device");
    lua_pushstring(L, (*answer).device);
    //ESP_LOGI(TAG, "Device: %s", (*answer).device);
    lua_settable(L, 1);
    
    lua_pushstring(L, "unit");
    lua_pushstring(L, (*answer).unit);
    //ESP_LOGI(TAG, "Unit: %s", (*answer).unit);
    lua_settable(L, 1);
    
    lua_pushstring(L, "type");
    lua_pushstring(L, (*answer).type);
    //ESP_LOGI(TAG, "Type: %s", (*answer).type);
    lua_settable(L, 1);
    
    lua_pushstring(L, "timestamp");
    lua_pushinteger(L, (*answer).timestamp);
    //ESP_LOGI(TAG, "Timestamp: %d", (*answer).timestamp);
    lua_settable(L, 1);
    
    lua_pushstring(L, "value");
    lua_pushnumber(L, (*answer).value);
    //ESP_LOGI(TAG, "Value: %f", (*answer).value);
    lua_settable(L, 1);
    
    // disabled for evaluation
    //ESP_LOGI(TAG, "Sending back temperature of %f °C", temperature);
    
    return 1;
}

int lua_get_pressure (lua_State *L)
{
    answer_decoded_t *answer;
    if((lua_gettop(L) != 1) || !lua_isinteger(L, -1)) {
        answer = answer_decoded_new();
        status_pressure = 5;
        (*answer).status = status_pressure;
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
    }
    else {
        int requestid;
        requestid = luaL_checkinteger(L, -1);
        answer = bmp180_get_answer_pressure();
        (*answer).requestid = requestid;
    }
    
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

static const struct luaL_Reg bmp180 [] = {
    {"temperature", lua_get_temperature},
    {"pressure", lua_get_pressure},
    {NULL, NULL}  /* sentinel */
};

int luaopen_bmp180 (lua_State *L) {
    luaL_newlib(L, bmp180);
    return 1;
}
