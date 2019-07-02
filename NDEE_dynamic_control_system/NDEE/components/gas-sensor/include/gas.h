/*
 *  Created on: 18 Mar 2018
 *      Author: Alex Moroz
 */

#ifndef COMPONENTS_GAS_SENSOR_INCLUDE_GAS_H_
#define COMPONENTS_GAS_SENSOR_INCLUDE_GAS_H_

#include "stack.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define GAS_SENSOR_ADC ADC1_CHANNEL_0
#define GAS_SENSOR_GPIO 39
answer_decoded_t *gassensor_get_answer_gas ();

int gassensor_get_values (lua_State *L);
void read_gas_value();

int luaopen_gas (lua_State *L);

#endif /* COMPONENTS_GAS_SENSOR_INCLUDE_GAS_H_ */





