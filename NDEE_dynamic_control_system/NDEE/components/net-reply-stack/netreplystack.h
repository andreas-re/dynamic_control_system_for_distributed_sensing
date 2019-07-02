#ifndef COMPONENTS_NETREPLYSTACK_NETREPLYSTACK_H_
#define COMPONENTS_NETRREPLYSTACK_NETREPLYSTACK_H_

#include "lua.h"
#include "stack.h"

typedef enum {
	NETREPLYSTACK_MODE_NORMAL = 0,
	NETREPLYSTACK_MODE_ULTRASONIC = 1
} net_reply_stack_mode;

typedef struct {
    int (*write_value) (int, answer_decoded_t *);
    int receiver;
    stack_t *reply_stack;
    TickType_t frequency;
    int duration;
    int taskid;
    int mode;
    int dynamic_threshold;
} net_reply_stack_config_t;

int net_stack(lua_State *L);
int net_stack_us(lua_State *L);

#endif /* COMPONENTS_NETREPLYSTACK_NETREPLYSTACK_H_ */
