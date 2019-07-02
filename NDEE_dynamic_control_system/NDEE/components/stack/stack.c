#include "stack.h"
#include "esp_log.h"

static const char *TAG = "STACK";

//Create new stack
stack_t *stack_new() {
    stack_t *stack;
    stack = malloc(sizeof(*stack));
    (*stack).top = NULL;
    (*stack).size = 0;
    (*stack).xMutex = xSemaphoreCreateMutex();
    return stack;
}

//Get data from top of stack (without removing element)
answer_decoded_t *pop(stack_t *stack) {
    SemaphoreHandle_t xMutex;
    xMutex = (*stack).xMutex;
    xSemaphoreTake(xMutex, portMAX_DELAY); //Lock
    if (stack != NULL && stack->top != NULL) {
        element_t *element = stack->top;
        xSemaphoreGive((*stack).xMutex); //Unlock
        answer_decoded_t *data = element->data;
        return data;
    }
    xSemaphoreGive(stack->xMutex); //Lock
    return NULL;
}

//Get element from top of stack (with removing it)
element_t *pop_element(stack_t *stack) {
    if (stack != NULL && stack->top != NULL) {
        element_t *element = stack->top;
        stack->top = element->next;
        stack->size--;
        return element;
    }
    return NULL;
}

//Empty a stack
void empty (stack_t *stack) {
    bool empty = false;
    while (empty == false) {
        element_t *element;
        element = pop_element(stack);
        if (element == NULL) {
            empty = true;
        }
        else {
            free (element);
        }
    }
}

//Remove 5 elements from botton of stack (without getting them)
stack_t *remove_element(stack_t *stack) {
    stack_t *helper;
    int i;
    helper = stack_new();
    for (i = 0; i < 5; i++) {
        element_t *element;
        element = pop_element(stack);
        push(helper, element->data);
    }
    empty(stack);
    for (i = 0; i < 5; i++) {
        element_t *element;
        element = pop_element(helper);
        push_without_semaphore(stack, element->data);
    }
    free(helper);
    return stack;
}

//Push element to top of stack
void push(stack_t *stack, answer_decoded_t *data) {
    element_t *element = malloc(sizeof(*element));
    if (stack != NULL && element != NULL) {
        element->data = data;
        element->next = stack->top;
        xSemaphoreTake((*stack).xMutex, portMAX_DELAY); //Lock
        stack->top = element;
        stack->size++;
        if (stack->size >= 10){
            stack = remove_element(stack);
        }
        xSemaphoreGive((*stack).xMutex); //Unlock
    }
}

void push_without_semaphore(stack_t *stack, answer_decoded_t *data) {
    element_t *element = malloc(sizeof(*element));
    if (stack != NULL && element != NULL) {
        element->data = data;
        element->next = stack->top;
        stack->top = element;
        stack->size++;
        if (stack->size >= 10){
            stack = remove_element(stack);
        }
    }
}
