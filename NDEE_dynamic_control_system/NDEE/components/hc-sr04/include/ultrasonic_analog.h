/*
 *  Created on: 29 Mar 2018
 *      Author: Alex Moroz
 */

#ifndef COMPONENTS_HC_SR04_INCLUDE_ULTRASONIC_H_
#define COMPONENTS_HC_SR04_INCLUDE_ULTRASONIC_H_

//#define TRIG_PIN 5
#define TRIG_PIN 32
#define ECHO_PIN ADC1_CHANNEL_0

int luaopen_ultrasonic (lua_State *L);
int get_distance(lua_State *L);


#endif /* COMPONENTS_HC_SR04_INCLUDE_ULTRASONIC_H_ */
