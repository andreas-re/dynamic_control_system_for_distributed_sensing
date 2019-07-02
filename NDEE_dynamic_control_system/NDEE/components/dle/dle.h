#ifndef COMPONENTS_DLE_DLE_H_
#define COMPONENTS_DLE_DLE_H_

#include "lua.h"
#include "stack.h"

void execute();

int luaopen_dle (lua_State *L);

char *startLuaTask(char *task);

#endif /* COMPONENTS_DLE_DLE_H_ */
