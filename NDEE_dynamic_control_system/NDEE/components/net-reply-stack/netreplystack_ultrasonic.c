// copied from https://github.com/UncleRus/esp-idf-lib/blob/master/examples/ultrasonic/main/main.c

#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "ultrasonic.h"
#include "netreplystack_ultrasonic.h"


bool ultrasonic_init_done = false;

int ultrasonic_read() {
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };

    if(!ultrasonic_init_done) {
        ultrasonic_init(&sensor);
        ultrasonic_init_done = true;
    }

    uint32_t distance;
    esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
    if (res != ESP_OK)
    {
        printf("Error: ");
        switch (res)
        {
            case ESP_ERR_ULTRASONIC_PING:
                printf("Cannot ping (device is in invalid state)\n");
                break;
            case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                printf("Ping timeout (no device found)\n");
                break;
            case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                printf("Echo timeout (i.e. distance too big)\n");
                break;
            default:
                printf("ultrasonic measurement result: %d\n", res);
        }
    }
    else {
        printf("Distance: %d cm, %.02f m\n", distance, distance / 100.0);
        return distance;
    }

    return (-1);
}
