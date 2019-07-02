#ifndef COMPONENTS_ACTUATORREPLYSTACK_ACTUATORREPLYSTACK_H_
#define COMPONENTS_ACTUATORREPLYSTACK_ACTUATORREPLYSTACK_H_

#include "lua.h"
#include "stack.h"

typedef struct {
    int (*write_value) (int);
    stack_t *reply_stack;
    TickType_t frequency;
    int duration;
    int taskid;
} actuator_reply_stack_config_t;

int lcd_stack(lua_State *L);

#endif /* COMPONENTS_ACTUATORREPLYSTACK_ACTUATORREPLYSTACK_H_ */
