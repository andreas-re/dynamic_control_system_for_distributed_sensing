#ifndef COMPONENTS_TASKMANAGER_TASKMANAGER_H_
#define COMPONENTS_TASKMANAGER_TASKMANAGER_H_

#include "stack.h"

typedef struct {
    int stackid;
    char *measurand;
    stack_t *stack;
} mapped_stack_t;

typedef struct {
    int size;
    mapped_stack_t *mapped_stacks[10];
} mapped_stacks_t;

mapped_stack_t *mapped_stack_new (char *measurand, stack_t *stack);

mapped_stacks_t *mapped_stacks_new ();

stack_t *stack_by_stackid (mapped_stacks_t *stacks, int stackid);

char *measurand_by_stackid (mapped_stacks_t *stacks, int stackid);

int stackid_by_measurand (mapped_stacks_t *stacks, char *measurand);

bool exists_stack (mapped_stacks_t *mapped_stacks, int stackid);

int insert_stack (mapped_stacks_t *mapped_stacks, char *measurand, stack_t *stack);

bool delete_stack (mapped_stacks_t *mapped_stacks, int stackid);

typedef struct {
    int taskid;
    int frequency;
    int duration;
    int stackid;
    char* measurand;
} producer_t;

typedef struct {
    int size;
    producer_t *producers [10];
} producers_t;

producer_t *producer_new (int frequency, int duration, int stackid, char* measurand);

producers_t *producers_new();

int frequency_by_producer (producers_t *producers, int taskid);

int duration_by_producer (producers_t *producers, int taskid);

int stack_by_producer (producers_t *producers, int taskid);

char *measurand_by_producer (producers_t *producers, int taskid);

int producer_by_stack (producers_t *producers, int stackid);

int producer_by_measurand (producers_t *producers, char *measurand);

bool exists_producer (producers_t *producers, int taskid);

int insert_producer (producers_t *producers, int frequency, int duration, int stackid, char *measurand);

bool delete_producer(producers_t *producers, int taskid);

bool change_producer_frequency (producers_t *producers, int taskid, int frequency);

bool change_producer_duration (producers_t *producers, int taskid, int duration);

typedef struct {
    int taskid;
    int clientid;
} producer_client_t;

typedef struct {
    int size;
    producer_client_t *producer_clients [100];
} producer_clients_t;

producer_client_t *producer_client_new (int identifier, int clientid);

producer_clients_t *producer_clients_new();

int *clients_by_producer (producer_clients_t *producer_clients, int taskid);

int noclients_by_producer (producer_clients_t *producer_clients, int taskid);

int insert_client_for_producer (producer_clients_t *producer_clients, int taskid, int clientid);

bool delete_client_for_producer (producer_clients_t *producer_clients, int taskid, int clientid);

int insert_producer_client (producers_t *producers, producer_clients_t *producer_clients, int frequency, int duration, int stackid, char *measurand, int clientid);

typedef struct {
    int taskid;
    int frequency;
    int duration;
    int stackid;
    char* actuator;
} aconsumer_t;

typedef struct {
    int size;
    aconsumer_t *aconsumers [10];
} aconsumers_t;

aconsumer_t *aconsumer_new (int frequency, int duration, int stackid, char* actuator);

aconsumers_t *aconsumers_new();

int frequency_by_aconsumer (aconsumers_t *aconsumers, int taskid);

int duration_by_aconsumer (aconsumers_t *aconsumers, int taskid);

int stack_by_aconsumer (aconsumers_t *aconsumers, int taskid);

char *actuator_by_aconsumer (aconsumers_t *aconsumers, int taskid);

int aconsumer_by_actuator (aconsumers_t *aconsumers, char *actuator);

bool exists_aconsumer (aconsumers_t *aconsumers, int taskid);

int insert_aconsumer (aconsumers_t *aconsumers, int frequency, int duration, int stackid, char *actuator);

bool delete_aconsumer(aconsumers_t *aconsumers, int taskid);

bool change_aconsumer_frequency (aconsumers_t *aconsumers, int taskid, int frequency);

bool change_aconsumer_duration (aconsumers_t *aconsumers, int taskid, int duration);

typedef struct {
    int taskid;
    int clientid;
} aconsumer_client_t;

typedef struct {
    int size;
    aconsumer_client_t *aconsumer_clients [100];
} aconsumer_clients_t;

aconsumer_client_t *aconsumer_client_new (int taskid, int clientid);

aconsumer_clients_t *aconsumer_clients_new();

int *clients_by_aconsumer (aconsumer_clients_t *aconsumer_clients, int taskid);

int noclients_by_aconsumer (aconsumer_clients_t *aconsumer_clients, int taskid);

int insert_client_for_aconsumer (aconsumer_clients_t *aconsumer_clients, int taskid, int clientid);

bool delete_client_for_aconsumer (aconsumer_clients_t *aconsumer_clients, int taskid, int clientid);

int insert_aconsumer_client (aconsumers_t *aconsumers, aconsumer_clients_t *aconsumer_clients, int frequency, int duration, int stackid, char *actuator, int clientid);

typedef struct {
    int taskid;
    int frequency;
    int duration;
    int stackid;
    int clientid;
} cconsumer_t;

typedef struct {
    int size;
    cconsumer_t *cconsumers [10];
} cconsumers_t;

cconsumer_t *cconsumer_new (int frequency, int duration, int stackid, int clientid);

cconsumers_t *cconsumers_new();

int frequency_by_cconsumer (cconsumers_t *cconsumers, int taskid);

int duration_by_cconsumer (cconsumers_t *cconsumers, int taskid);

int stack_by_cconsumer (cconsumers_t *cconsumers, int taskid);

int client_by_cconsumer (cconsumers_t *cconsumers, int taskid);

int cconsumer_by_stackclient (cconsumers_t *cconsumers, int stackid, int clientid);

bool exists_cconsumer (cconsumers_t *cconsumers, int taskid);

int insert_cconsumer (cconsumers_t *cconsumers, int frequency, int duration, int stackid, int clientid);

bool delete_cconsumer(cconsumers_t *cconsumers, int taskid);

bool change_cconsumer_frequency (cconsumers_t *cconsumers, int taskid, int frequency);

bool change_cconsumer_duration (cconsumers_t *cconsumers, int taskid, int duration);

#endif /* COMPONENTS_TASKMANAGER_TASKMANAGER_H_ */
