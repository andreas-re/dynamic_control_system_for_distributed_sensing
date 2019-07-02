#ifndef COMPONENTS_BMP180_BMP180_LUA_H_
#define COMPONENTS_BMP180_BMP180_LUA_H_

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "answer.h"

answer_decoded_t *bmp180_get_answer_temperature();

answer_decoded_t *bmp180_get_answer_pressure();

int lua_get_temperature (lua_State *L);

int lua_get_pressure (lua_State *L);

int luaopen_bmp180 (lua_State *L);

#endif /* COMPONENTS_BMP180_BMP180_LUA_H_ */
