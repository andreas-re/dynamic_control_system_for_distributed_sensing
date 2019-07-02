/*
 *  Created on: 18 Mar 2018
 *      Author: Alex Moroz
 */
#include "driver/adc.h"
#include "driver/gpio.h"
#include "gas.h"

int level = 0;
int val = 0;

void read_gas_value() {
    printf("GAS");
	gpio_set_direction(GAS_SENSOR_GPIO, GPIO_MODE_INPUT);

	adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(GAS_SENSOR_ADC, ADC_ATTEN_DB_11);

	level = gpio_get_level(GAS_SENSOR_GPIO);
    val = adc1_get_voltage(GAS_SENSOR_ADC);
    
    printf ("This is the gas level: %d \n", level);
    printf ("This is the gas value: %d \n", val);
}

int gassensor_get_values (lua_State *L)
{
    read_gas_value();
    lua_pushnumber(L, level);
    lua_pushnumber(L, val);
    return 2;
}

answer_decoded_t *gassensor_get_answer_gas ()
{
    read_gas_value();
    answer_decoded_t *answer;
    answer = answer_decoded_new();
    (*answer).measurand = "CO2";
    (*answer).device = "GASSENSOR";
    (*answer).unit = "PPM";
    (*answer).type = "INTEGER";
    (*answer).status = 1;
    time_t now;
    time(&now);
    (*answer).timestamp = now;
    (*answer).value = val;
    return answer;
}

static const struct luaL_Reg gas [] = {
    {"get", gassensor_get_values},
    {NULL, NULL}  /* sentinel */
};

int luaopen_gas (lua_State *L) {
    luaL_newlib(L, gas);
    return 1;
}
