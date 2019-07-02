// copied from https://github.com/UncleRus/esp-idf-lib/blob/master/examples/ultrasonic/main/main.c

#ifndef COMPONENTS_NETREPLYSTACK_ULTRASONIC_H_
#define COMPONENTS_NETRREPLYSTACK_ULTRASONIC_H_

#define MAX_DISTANCE_CM 500 // 5m max
#define TRIGGER_GPIO 26
#define ECHO_GPIO 25

int ultrasonic_read();

#endif /* COMPONENTS_NETREPLYSTACK_ULTRASONIC_H_ */
