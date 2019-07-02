#ifndef COMPONENTS_STACK_STACK_H_
#define COMPONENTS_STACK_STACK_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "answer.h"

typedef struct {
    struct element_t *next;
    answer_decoded_t *data;
} element_t;

typedef struct{
    int size;
    SemaphoreHandle_t xMutex;
    struct element_t *top;
} stack_t;

stack_t *stack_new ();

void push (stack_t *stack, answer_decoded_t *data);
void push_without_semaphore (stack_t *stack, answer_decoded_t *data);

answer_decoded_t *pop (stack_t *stack);

#endif /* COMPONENTS_STACK_STACK_H_ */
