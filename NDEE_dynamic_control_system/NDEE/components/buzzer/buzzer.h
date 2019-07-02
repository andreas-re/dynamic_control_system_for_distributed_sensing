#ifndef COMPONENTS_BUZZER_BUZZER_H_
#define COMPONENTS_BUZZER_BUZZER_H_

#include "stack.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define GPIO_BUZZER (33)
#define DUTY_MAX (4095)

int play_song(lua_State *L, stack_t *stack);

#endif /* COMPONENTS_BUZZER_BUZZER_H_ */
