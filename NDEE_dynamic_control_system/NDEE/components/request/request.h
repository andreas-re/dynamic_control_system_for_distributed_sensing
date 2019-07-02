#ifndef COMPONENTS_REQUEST_REQUEST_H_
#define COMPONENTS_REQUEST_REQUEST_H_

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "stack.h"

typedef struct {
    int taskid;
    answer_decoded_t *(*handle_sensor) ();
    TickType_t frequency;
    int duration;
    stack_t *handle_stack;
    int requestid;
} request_config_t;

int bmp180_temperature(lua_State *L);

int bmp180_pressure(lua_State *L);

int gassensor_co2(lua_State *L);

#endif /* COMPONENTS_REQUEST_REQUEST_H_ */
