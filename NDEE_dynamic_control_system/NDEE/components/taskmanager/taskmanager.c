#include "taskmanager.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "TASKMANAGER";

int taskId;
int stackId;

mapped_stack_t *mapped_stack_new (char *measurand, stack_t *stack) {
    mapped_stack_t *mapped_stack;
    mapped_stack = malloc(sizeof(*mapped_stack));
    (*mapped_stack).stackid = stackId;
    (*mapped_stack).measurand = measurand;
    (*mapped_stack).stack = stack;
    stackId++;
    return mapped_stack;
}

mapped_stacks_t *mapped_stacks_new () {
    mapped_stacks_t *mapped_stacks;
    mapped_stacks = malloc(sizeof(*mapped_stacks));
    (*mapped_stacks).size = 0;
    return mapped_stacks;
}

stack_t *stack_by_stackid (mapped_stacks_t *stacks, int stackid) {
    int size;
    mapped_stack_t **mapped_stacks;
    int i;
    mapped_stack_t *mapped_stack;
    int current_stackid;
    stack_t *current_stack;
    
    size = (*stacks).size;
    mapped_stacks = (*stacks).mapped_stacks;
    
    for (i = 0; i < size; i++) {
        mapped_stack = mapped_stacks[i];
        current_stackid = (*mapped_stack).stackid;
        current_stack = (*mapped_stack).stack;
        
        if (current_stackid == stackid) {
            return current_stack;
        }
    }
    return NULL;
}

char *measurand_by_stackid (mapped_stacks_t *stacks, int stackid) {
    int size;
    mapped_stack_t **mapped_stacks;
    int i;
    mapped_stack_t *mapped_stack;
    int current_stackid;
    char *current_measurand;
    
    size = (*stacks).size;
    mapped_stacks = (*stacks).mapped_stacks;
    
    for (i = 0; i < size; i++) {
        mapped_stack = mapped_stacks[i];
        current_stackid = (*mapped_stack).stackid;
        current_measurand = (*mapped_stack).measurand;
        
        if (current_stackid == stackid) {
            return current_measurand;
        }
    }
    return "";
}

int stackid_by_measurand (mapped_stacks_t *stacks, char *measurand) {
    int size;
    mapped_stack_t **mapped_stacks;
    int i;
    mapped_stack_t *mapped_stack;
    int current_stackid;
    char *current_measurand;
    
    size = (*stacks).size;
    mapped_stacks = (*stacks).mapped_stacks;
    
    for (i = 0; i < size; i++) {
        mapped_stack = mapped_stacks[i];
        current_stackid = (*mapped_stack).stackid;
        current_measurand = (*mapped_stack).measurand;
        
        if (strcmp(measurand, current_measurand) == 0) {
            return current_stackid;
        }
    }
    return -1;
}


bool exists_stack (mapped_stacks_t *stacks, int stackid) {
    int size;
    mapped_stack_t **mapped_stacks;
    int i;
    mapped_stack_t *mapped_stack;
    int current_stackid;
    stack_t *current_stack;
    
    size = (*stacks).size;
    mapped_stacks = (*stacks).mapped_stacks;
    
    for (i = 0; i < size; i++) {
        mapped_stack = mapped_stacks[i];
        current_stackid = (*mapped_stack).stackid;
        
        if (current_stackid == stackid) {
            return true;
        }
    }
    return false;
}

int insert_stack (mapped_stacks_t *mapped_stacks, char *measurand, stack_t *stack) {
    int stackid;
    mapped_stack_t *mapped_stack;
    stackid = stackid_by_measurand (mapped_stacks, measurand);
    if (stackid < 0) {
        mapped_stack = mapped_stack_new (measurand, stack);
        stackid = (*mapped_stack).stackid;
        (*mapped_stacks).mapped_stacks[(*mapped_stacks).size] = mapped_stack;
        (*mapped_stacks).size++;
    }
    return stackid;
}

bool delete_stack (mapped_stacks_t *mapped_stacks, int stackid) {
    int size;
    mapped_stack_t **stacks;
    int i;
    mapped_stack_t *stack;
    int current_stackid;
    
    size = (*mapped_stacks).size;
    stacks = (*mapped_stacks).mapped_stacks;
    
    for (i = 0; i < size; i++) {
        stack = stacks[i];
        current_stackid = (*stack).stackid;
        
        if (current_stackid == stackid) {
            (*mapped_stacks).mapped_stacks[i] = stacks[size-1];
            (*mapped_stacks).mapped_stacks[size-1] = NULL;
            free(stack);
            (*mapped_stacks).size--;
            return true;
        }
    }
    return false;
}

producer_t *producer_new (int frequency, int duration, int stackid, char* measurand) {
    producer_t *producer;
    producer = malloc(sizeof(*producer));
    (*producer).taskid = taskId;
    (*producer).frequency = frequency;
    (*producer).duration = duration;
    (*producer).stackid = stackid;
    (*producer).measurand = measurand;
    taskId++;
    return producer;
}

producer_client_t *producer_client_new (int taskid, int clientid) {
    producer_client_t *producer_client;
    producer_client = malloc(sizeof(*producer_client));
    (*producer_client).taskid = taskid;
    (*producer_client).clientid = clientid;
    return producer_client;
}

producers_t *producers_new() {
    producers_t *producers;
    producers = malloc(sizeof(*producers));
    (*producers).size = 0;
    return producers;
}

producer_clients_t *producer_clients_new() {
    producer_clients_t *producer_clients;
    producer_clients = malloc(sizeof(*producer_clients));
    (*producer_clients).size = 0;
    return producer_clients;
}

int frequency_by_producer (producers_t *producers, int taskid) {
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    int current_taskid;
    int current_frequency;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_taskid = (*producer).taskid;
        current_frequency = (*producer).frequency;
        if (current_taskid == taskid) {
            return current_frequency;
        }
    }
    return -1;
}

int duration_by_producer (producers_t *producers, int taskid) {
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    int current_taskid;
    int current_duration;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_taskid = (*producer).taskid;
        current_duration = (*producer).duration;
        if (current_taskid == taskid) {
            return current_duration;
        }
    }
    return -1;
}

int stack_by_producer (producers_t *producers, int taskid) {
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    int current_taskid;
    int current_stackid;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_taskid = (*producer).taskid;
        current_stackid = (*producer).stackid;
        if (current_taskid == taskid) {
            return current_stackid;
        }
    }
    return -1;
}

char *measurand_by_producer (producers_t *producers, int taskid) {
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    int current_taskid;
    char *current_measurand;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_taskid = (*producer).taskid;
        current_measurand = (*producer).measurand;
        if (current_taskid == taskid) {
            return current_measurand;
        }
    }
    return "";
}

int producer_by_stack (producers_t *producers, int stackid) {
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    int current_stackid;
    int current_taskid;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_stackid = (*producer).stackid;
        current_taskid = (*producer).taskid;
        if (current_stackid == stackid) {
            return current_taskid;
        }
    }
    return -1;
}

int producer_by_measurand (producers_t *producers, char *measurand) {
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    char *current_measurand;
    int current_taskid;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_measurand = (*producer).measurand;
        current_taskid = (*producer).taskid;
        if (strcmp(current_measurand, measurand) == 0) {
            return current_taskid;
        }
    }
    return -1;
}

bool exists_producer (producers_t *producers, int taskid) {
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    int current_taskid;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_taskid = (*producer).taskid;
        
        if (current_taskid == taskid) {
            return true;
        }
    }
    return false;
}

int insert_producer (producers_t *producers, int frequency, int duration, int stackid, char *measurand) {
    int taskid;
    producer_t *producer;
    taskid = producer_by_stack (producers, stackid);
    if (taskid >= 0) {
        return taskid;
    }
    taskid = producer_by_measurand (producers, measurand);
    if (taskid >= 0) {
        return taskid;
    }
    producer = producer_new (frequency, duration, stackid, measurand);
    taskid = (*producer).taskid;
    (*producers).producers[(*producers).size] = producer;
    (*producers).size++;
    return taskid;
}

bool delete_producer(producers_t *producers, int taskid) {
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    int current_producerid;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_producerid = (*producer).taskid;
        
        if (current_producerid == taskid) {
            (*producers).producers[i] = mapped_producers[size-1];
            (*producers).producers[size-1] = NULL;
            free(producer);
            (*producers).size--;
            return true;
        }
    }
    return false;
}

bool change_producer_frequency (producers_t *producers, int taskid, int frequency){
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    int current_producerid;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_producerid = (*producer).taskid;
        
        if (current_producerid == taskid) {
            producers->producers[i]->frequency = frequency;
            return true;
        }
    }
    return false;
}

bool change_producer_duration (producers_t *producers, int taskid, int duration) {
    int size;
    producer_t **mapped_producers;
    int i;
    producer_t *producer;
    int current_producerid;
    
    size = (*producers).size;
    mapped_producers = (*producers).producers;
    
    for (i = 0; i < size; i++) {
        producer = mapped_producers[i];
        current_producerid = (*producer).taskid;
        
        if (current_producerid == taskid) {
            producers->producers[i]->duration = duration;
            return true;
        }
    }
    return false;
}

int *clients_by_producer (producer_clients_t *mapped_producer_clients, int taskid) {
    int size;
    producer_client_t **producer_clients;
    int i;
    producer_client_t *producer_client;
    int current_taskid;
    int current_clientid;
    int result_size;
    int results [100];
    
    size = (*mapped_producer_clients).size;
    producer_clients = (*mapped_producer_clients).producer_clients;
    result_size = 0;
    
    for (i = 0; i < size; i++) {
        producer_client = producer_clients[i];
        current_clientid = (*producer_client).clientid;
        current_taskid = (*producer_client).taskid;
        if (taskid == current_taskid) {
            results[result_size] = current_clientid;
            result_size++;
        }
    }
    return results;
}

int noclients_by_producer (producer_clients_t *mapped_producer_clients, int taskid) {
    int size;
    producer_client_t **producer_clients;
    int i;
    producer_client_t *producer_client;
    int current_taskid;
    int current_clientid;
    int number;
    
    size = (*mapped_producer_clients).size;
    producer_clients = (*mapped_producer_clients).producer_clients;
    number = 0;
    
    for (i = 0; i < size; i++) {
        producer_client = producer_clients[i];
        current_clientid = (*producer_client).clientid;
        current_taskid = (*producer_client).taskid;
        if (taskid == current_taskid) {
            number++;
        }
    }
    return number;
}

int insert_client_for_producer (producer_clients_t *producer_clients, int taskid, int clientid) {
    int clients [100];
    int *temp;
    int size;
    int i;
    int current_clientid;
    temp = clients_by_producer (producer_clients,taskid);
    memcpy(&clients, &temp, sizeof clients);
    size = noclients_by_producer (producer_clients, taskid);
    for (i=0; i<size; i++) {
        current_clientid = clients[i];
        if (current_clientid == clientid){
            return taskid;
        }
    }
    producer_client_t *producer_client;
    producer_client = producer_client_new(taskid, clientid);
    (*producer_clients).producer_clients[(*producer_clients).size] = producer_client;
    (*producer_clients).size++;
    return taskid;
}

bool delete_client_for_producer (producer_clients_t *producer_clients, int taskid, int clientid) {
    int size;
    producer_client_t **mapped_producer_clients;
    int i;
    producer_client_t *producer_client;
    int current_producerid;
    int current_clientid;
    
    size = (*producer_clients).size;
    mapped_producer_clients = (*producer_clients).producer_clients;
    
    for (i = 0; i < size; i++) {
        producer_client = mapped_producer_clients[i];
        current_producerid = (*producer_client).taskid;
        current_clientid = (*producer_client).clientid;
        
        if (current_producerid == taskid) {
            (*producer_clients).producer_clients[i] = mapped_producer_clients[size-1];
            (*producer_clients).producer_clients[size-1] = NULL;
            free(producer_client);
            (*producer_clients).size--;
            return true;
        }
    }
    return false;
}

int insert_producer_client (producers_t *producers, producer_clients_t *producer_clients, int frequency, int duration, int stackid, char *measurand, int clientid) {
    int taskid;
    taskid = insert_producer (producers, frequency, duration, stackid, measurand);
    taskid = insert_client_for_producer (producer_clients, taskid, clientid);
    return taskid;
}

aconsumer_t *aconsumer_new (int frequency, int duration, int stackid, char* actuator) {
    aconsumer_t *aconsumer;
    aconsumer = malloc(sizeof(*aconsumer));
    (*aconsumer).taskid = taskId;
    (*aconsumer).frequency = frequency;
    (*aconsumer).duration = duration;
    (*aconsumer).stackid = stackid;
    (*aconsumer).actuator = actuator;
    taskId++;
    return aconsumer;
}

aconsumers_t *aconsumers_new(){
    aconsumers_t *aconsumers;
    aconsumers = malloc(sizeof(*aconsumers));
    (*aconsumers).size = 0;
    return aconsumers;
}

int frequency_by_aconsumer (aconsumers_t *aconsumers, int taskid) {
    int size;
    aconsumer_t *mapped_aconsumers[10];
    int i;
    aconsumer_t *aconsumer;
    int current_taskid;
    int current_frequency;
    
    size = (*aconsumers).size;
    memcpy(&mapped_aconsumers, &(*aconsumers).aconsumers, sizeof mapped_aconsumers);
    
    for (i = 0; i < size; i++) {
        aconsumer = mapped_aconsumers[i];
        current_taskid = (*aconsumer).taskid;
        current_frequency = (*aconsumer).frequency;
        if (current_taskid == taskid) {
            return current_frequency;
        }
    }
    return -1;
}

int duration_by_aconsumer (aconsumers_t *aconsumers, int taskid) {
    int size;
    aconsumer_t *mapped_aconsumers[10];
    int i;
    aconsumer_t *aconsumer;
    int current_taskid;
    int current_duration;
    
    size = (*aconsumers).size;
    memcpy(&mapped_aconsumers, &(*aconsumers).aconsumers, sizeof mapped_aconsumers);
    
    for (i = 0; i < size; i++) {
        aconsumer = mapped_aconsumers[i];
        current_taskid = (*aconsumer).taskid;
        current_duration = (*aconsumer).duration;
        if (current_taskid == taskid) {
            return current_duration;
        }
    }
    return -1;
}

int stack_by_aconsumer (aconsumers_t *aconsumers, int taskid) {
    int size;
    aconsumer_t *mapped_aconsumers[10];
    int i;
    aconsumer_t *aconsumer;
    int current_taskid;
    int current_stackid;
    
    size = (*aconsumers).size;
    memcpy(&mapped_aconsumers, &(*aconsumers).aconsumers, sizeof mapped_aconsumers);
    
    for (i = 0; i < size; i++) {
        aconsumer = mapped_aconsumers[i];
        current_taskid = (*aconsumer).taskid;
        current_stackid = (*aconsumer).stackid;
        if (current_taskid == taskid) {
            return current_stackid;
        }
    }
    return -1;
}

char *actuator_by_aconsumer (aconsumers_t *aconsumers, int taskid) {
    int size;
    aconsumer_t *mapped_aconsumers[10];
    int i;
    aconsumer_t *aconsumer;
    int current_taskid;
    char *current_actuator;
    
    size = (*aconsumers).size;
    memcpy(&mapped_aconsumers, &(*aconsumers).aconsumers, sizeof mapped_aconsumers);
    
    for (i = 0; i < size; i++) {
        aconsumer = mapped_aconsumers[i];
        current_taskid = (*aconsumer).taskid;
        current_actuator = (*aconsumer).actuator;
        if (current_taskid == taskid) {
            return current_actuator;
        }
    }
    return "";
}

int aconsumer_by_actuator (aconsumers_t *aconsumers, char *actuator) {
    int size;
    aconsumer_t *mapped_aconsumers[10];
    int i;
    aconsumer_t *aconsumer;
    char *current_actuator;
    int current_taskid;
    
    size = (*aconsumers).size;
    memcpy(&mapped_aconsumers, &(*aconsumers).aconsumers, sizeof mapped_aconsumers);
    
    for (i = 0; i < size; i++) {
        aconsumer = mapped_aconsumers[i];
        current_actuator = (*aconsumer).actuator;
        current_taskid = (*aconsumer).taskid;
        if (strcmp(current_actuator, actuator) == 0) {
            return current_taskid;
        }
    }
    return -1;
}

bool exists_aconsumer (aconsumers_t *aconsumers, int taskid) {
    int size;
    aconsumer_t *mapped_aconsumers[10];
    int i;
    aconsumer_t *aconsumer;
    int current_taskid;
    
    size = (*aconsumers).size;
    memcpy(&mapped_aconsumers, &(*aconsumers).aconsumers, sizeof mapped_aconsumers);
    
    for (i = 0; i < size; i++) {
        aconsumer = mapped_aconsumers[i];
        current_taskid = (*aconsumer).taskid;
        
        if (current_taskid == taskid) {
            return true;
        }
    }
    return false;
}

int insert_aconsumer (aconsumers_t *aconsumers, int frequency, int duration, int stackid, char *actuator) {
    int taskid;
    aconsumer_t *aconsumer;
    
    taskid = aconsumer_by_actuator (aconsumers, actuator);
    if (taskid >= 0) {
        return taskid;
    }
    aconsumer = aconsumer_new (frequency, duration, stackid, actuator);
    taskid = (*aconsumer).taskid;
    (*aconsumers).aconsumers[(*aconsumers).size] = aconsumer;
    (*aconsumers).size++;
    return taskid;
}

bool delete_aconsumer(aconsumers_t *aconsumers, int taskid) {
    int size;
    aconsumer_t *mapped_aconsumers[10];
    int i;
    aconsumer_t *aconsumer;
    int current_aconsumerid;
    
    size = (*aconsumers).size;
    memcpy(&mapped_aconsumers, &(*aconsumers).aconsumers, sizeof mapped_aconsumers);
    
    for (i = 0; i < size; i++) {
        aconsumer = mapped_aconsumers[i];
        current_aconsumerid = (*aconsumer).taskid;
        
        if (current_aconsumerid == taskid) {
            (*aconsumers).aconsumers[i] = mapped_aconsumers[size-1];
            (*aconsumers).aconsumers[size-1] = NULL;
            free(aconsumer);
            (*aconsumers).size--;
            return true;
        }
    }
    return false;
}

bool change_aconsumer_frequency (aconsumers_t *aconsumers, int taskid, int frequency) {
    int size;
    aconsumer_t *mapped_aconsumers[10];
    int i;
    aconsumer_t *aconsumer;
    int current_aconsumerid;
    
    size = (*aconsumers).size;
    memcpy(&mapped_aconsumers, &(*aconsumers).aconsumers, sizeof mapped_aconsumers);
    
    for (i = 0; i < size; i++) {
        aconsumer = mapped_aconsumers[i];
        current_aconsumerid = (*aconsumer).taskid;
        
        if (current_aconsumerid == taskid) {
            aconsumers->aconsumers[i]->frequency = frequency;
            return true;
        }
    }
    return false;
}

bool change_aconsumer_duration (aconsumers_t *aconsumers, int taskid, int duration){
    int size;
    aconsumer_t *mapped_aconsumers[10];
    int i;
    aconsumer_t *aconsumer;
    int current_aconsumerid;
    
    size = (*aconsumers).size;
    memcpy(&mapped_aconsumers, &(*aconsumers).aconsumers, sizeof mapped_aconsumers);
    
    for (i = 0; i < size; i++) {
        aconsumer = mapped_aconsumers[i];
        current_aconsumerid = (*aconsumer).taskid;
        
        if (current_aconsumerid == taskid) {
            aconsumers->aconsumers[i]->duration = duration;
            return true;
        }
    }
    return false;
}

aconsumer_client_t *aconsumer_client_new (int taskid, int clientid) {
    aconsumer_client_t *aconsumer_client;
    aconsumer_client = malloc(sizeof(*aconsumer_client));
    (*aconsumer_client).taskid = taskid;
    (*aconsumer_client).clientid = clientid;
    return aconsumer_client;
}

aconsumer_clients_t *aconsumer_clients_new() {
    aconsumer_clients_t *aconsumer_clients;
    aconsumer_clients = malloc(sizeof(*aconsumer_clients));
    (*aconsumer_clients).size = 0;
    return aconsumer_clients;
}

int *clients_by_aconsumer (aconsumer_clients_t *mapped_aconsumer_clients, int taskid) {
    int size;
    aconsumer_client_t *aconsumer_clients [100];
    int i;
    aconsumer_client_t *aconsumer_client;
    int current_taskid;
    int current_clientid;
    int result_size;
    int results [100];
    
    size = (*mapped_aconsumer_clients).size;
    memcpy(&aconsumer_clients, &(*mapped_aconsumer_clients).aconsumer_clients, sizeof aconsumer_clients);
    result_size = 0;
    
    for (i = 0; i < size; i++) {
        aconsumer_client = aconsumer_clients[i];
        current_clientid = (*aconsumer_client).clientid;
        current_taskid = (*aconsumer_client).taskid;
        if (taskid == current_taskid) {
            results[result_size] = current_clientid;
            result_size++;
        }
    }
    return results;
}

int noclients_by_aconsumer (aconsumer_clients_t *mapped_aconsumer_clients, int taskid) {
    int size;
    aconsumer_client_t *aconsumer_clients [100];
    int i;
    aconsumer_client_t *aconsumer_client;
    int current_taskid;
    int result_size;
    
    size = (*mapped_aconsumer_clients).size;
    memcpy(&aconsumer_clients, &(*mapped_aconsumer_clients).aconsumer_clients, sizeof aconsumer_clients);
    result_size = 0;
    
    for (i = 0; i < size; i++) {
        aconsumer_client = aconsumer_clients[i];
        current_taskid = (*aconsumer_client).taskid;
        if (taskid == current_taskid) {
            result_size++;
        }
    }
    return result_size;
}
    

int insert_client_for_aconsumer (aconsumer_clients_t *aconsumer_clients, int taskid, int clientid) {
    int *clients;
    int size;
    int i;
    int current_clientid;
    
    clients = clients_by_aconsumer (aconsumer_clients,taskid);
    size = noclients_by_aconsumer (aconsumer_clients, taskid);
    
    for (i=0; i<size; i++) {
        current_clientid = clients[i];
        if (current_clientid == clientid){
            return taskid;
        }
    }
    
    aconsumer_client_t *aconsumer_client;
    aconsumer_client = aconsumer_client_new(taskid, clientid);
    (*aconsumer_clients).aconsumer_clients[(*aconsumer_clients).size] = aconsumer_client;
    (*aconsumer_clients).size++;
    return taskid;
}

bool delete_client_for_aconsumer (aconsumer_clients_t *aconsumer_clients, int taskid, int clientid) {
    int size;
    aconsumer_client_t *mapped_aconsumer_clients[100];
    int i;
    aconsumer_client_t *aconsumer_client;
    int current_aconsumerid;
    int current_clientid;
    
    size = (*aconsumer_clients).size;
    memcpy(&aconsumer_clients, &(*aconsumer_clients).aconsumer_clients, sizeof aconsumer_clients);
    
    for (i = 0; i < size; i++) {
        aconsumer_client = mapped_aconsumer_clients[i];
        current_aconsumerid = (*aconsumer_client).taskid;
        current_clientid = (*aconsumer_client).clientid;
        
        if (current_aconsumerid == taskid) {
            (*aconsumer_clients).aconsumer_clients[i] = mapped_aconsumer_clients[size-1];
            (*aconsumer_clients).aconsumer_clients[size-1] = NULL;
            free(aconsumer_client);
            (*aconsumer_clients).size--;
            return true;
        }
    }
    return false;
}

int insert_aconsumer_client (aconsumers_t *aconsumers, aconsumer_clients_t *aconsumer_clients, int frequency, int duration, int stackid, char *actuator, int clientid) {
    int taskid;
    taskid = insert_aconsumer (aconsumers, frequency, duration, stackid, actuator);
    taskid = insert_client_for_aconsumer (aconsumer_clients, taskid, clientid);
    return taskid;
}

cconsumer_t *cconsumer_new (int frequency, int duration, int stackid, int clientid){
    cconsumer_t *cconsumer;
    cconsumer = malloc(sizeof(*cconsumer));
    (*cconsumer).taskid = taskId;
    (*cconsumer).frequency = frequency;
    (*cconsumer).duration = duration;
    (*cconsumer).stackid = stackid;
    (*cconsumer).clientid = clientid;
    taskId++;
    return cconsumer;
}

cconsumers_t *cconsumers_new() {
    cconsumers_t *cconsumers;
    cconsumers = malloc(sizeof(*cconsumers));
    (*cconsumers).size = 0;
    return cconsumers;
}

int frequency_by_cconsumer (cconsumers_t *cconsumers, int taskid) {
    int size;
    cconsumer_t *mapped_cconsumers[10];
    int i;
    cconsumer_t *cconsumer;
    int current_taskid;
    int current_frequency;
    
    size = (*cconsumers).size;
    memcpy(&mapped_cconsumers, &(*cconsumers).cconsumers, sizeof mapped_cconsumers);
    
    for (i = 0; i < size; i++) {
        cconsumer = mapped_cconsumers[i];
        current_taskid = (*cconsumer).taskid;
        current_frequency = (*cconsumer).frequency;
        if (current_taskid == taskid) {
            return current_frequency;
        }
    }
    return -1;
}

int duration_by_cconsumer (cconsumers_t *cconsumers, int taskid) {
    int size;
    cconsumer_t *mapped_cconsumers[10];
    int i;
    cconsumer_t *cconsumer;
    int current_taskid;
    int current_duration;
    
    size = (*cconsumers).size;
    memcpy(&mapped_cconsumers, &(*cconsumers).cconsumers, sizeof mapped_cconsumers);
    
    for (i = 0; i < size; i++) {
        cconsumer = mapped_cconsumers[i];
        current_taskid = (*cconsumer).taskid;
        current_duration = (*cconsumer).duration;
        if (current_taskid == taskid) {
            return current_duration;
        }
    }
    return -1;
}

int stack_by_cconsumer (cconsumers_t *cconsumers, int taskid) {
    int size;
    cconsumer_t *mapped_cconsumers[10];
    int i;
    cconsumer_t *cconsumer;
    int current_taskid;
    int current_stackid;
    
    size = (*cconsumers).size;
    memcpy(&mapped_cconsumers, &(*cconsumers).cconsumers, sizeof mapped_cconsumers);
    
    for (i = 0; i < size; i++) {
        cconsumer = mapped_cconsumers[i];
        current_taskid = (*cconsumer).taskid;
        current_stackid = (*cconsumer).stackid;
        if (current_taskid == taskid) {
            return current_stackid;
        }
    }
    return -1;
}

int client_by_cconsumer (cconsumers_t *cconsumers, int taskid) {
    int size;
    cconsumer_t *mapped_cconsumers[10];
    int i;
    cconsumer_t *cconsumer;
    int current_taskid;
    int current_clientid;
    
    size = (*cconsumers).size;
    memcpy(&mapped_cconsumers, &(*cconsumers).cconsumers, sizeof mapped_cconsumers);
    
    for (i = 0; i < size; i++) {
        cconsumer = mapped_cconsumers[i];
        current_taskid = (*cconsumer).taskid;
        current_clientid = (*cconsumer).clientid;
        if (current_taskid == taskid) {
            return current_clientid;
        }
    }
    return -1;
}

int cconsumer_by_stackclient (cconsumers_t *cconsumers, int stackid, int clientid) {
    int size;
    cconsumer_t *mapped_cconsumers[10];
    int i;
    cconsumer_t *cconsumer;
    int current_taskid;
    int current_stackid;
    int current_clientid;
    
    size = (*cconsumers).size;
    memcpy(&mapped_cconsumers, &(*cconsumers).cconsumers, sizeof mapped_cconsumers);
    
    for (i = 0; i < size; i++) {
        cconsumer = mapped_cconsumers[i];
        current_taskid = (*cconsumer).taskid;
        current_stackid = (*cconsumer).stackid;
        current_clientid = (*cconsumer).clientid;
        if ((current_stackid == stackid) && (current_clientid == clientid)) {
            return current_taskid;
        }
    }
    return -1;
}

bool exists_cconsumer (cconsumers_t *cconsumers, int taskid) {
    int size;
    cconsumer_t *mapped_cconsumers[10];
    int i;
    cconsumer_t *cconsumer;
    int current_taskid;
    
    size = (*cconsumers).size;
    memcpy(&mapped_cconsumers, &(*cconsumers).cconsumers, sizeof mapped_cconsumers);
    
    for (i = 0; i < size; i++) {
        cconsumer = mapped_cconsumers[i];
        current_taskid = (*cconsumer).taskid;
        
        if (current_taskid == taskid) {
            return true;
        }
    }
    return false;
}

int insert_cconsumer (cconsumers_t *cconsumers, int frequency, int duration, int stackid, int clientid) {
    int taskid;
    cconsumer_t *cconsumer;
    taskid = cconsumer_by_stackclient (cconsumers, stackid, clientid);
    if (taskid >= 0) {
        return taskid;
    }
    
    cconsumer = cconsumer_new (frequency, duration, stackid, clientid);
    taskid = (*cconsumer).taskid;
    (*cconsumers).cconsumers[(*cconsumers).size] = cconsumer;
    (*cconsumers).size++;
    return taskid;
}

bool delete_cconsumer(cconsumers_t *cconsumers, int taskid){
    int size;
    cconsumer_t *mapped_cconsumers[10];
    int i;
    cconsumer_t *cconsumer;
    int current_cconsumerid;
    
    size = (*cconsumers).size;
    memcpy(&mapped_cconsumers, &(*cconsumers).cconsumers, sizeof mapped_cconsumers);
    
    for (i = 0; i < size; i++) {
        cconsumer = mapped_cconsumers[i];
        current_cconsumerid = (*cconsumer).taskid;
        
        if (current_cconsumerid == taskid) {
            (*cconsumers).cconsumers[i] = mapped_cconsumers[size-1];
            (*cconsumers).cconsumers[size-1] = NULL;
            free(cconsumer);
            (*cconsumers).size--;
            return true;
        }
    }
    return false;
}

bool change_cconsumer_frequency (cconsumers_t *cconsumers, int taskid, int frequency) {
    int size;
    cconsumer_t *mapped_cconsumers[10];
    int i;
    cconsumer_t *cconsumer;
    int current_cconsumerid;
    
    size = (*cconsumers).size;
    memcpy(&mapped_cconsumers, &(*cconsumers).cconsumers, sizeof mapped_cconsumers);
    
    for (i = 0; i < size; i++) {
        cconsumer = mapped_cconsumers[i];
        current_cconsumerid = (*cconsumer).taskid;
        
        if (current_cconsumerid == taskid) {
            cconsumers->cconsumers[i]->frequency = frequency;
            return true;
        }
    }
    return false;
}

bool change_cconsumer_duration (cconsumers_t *cconsumers, int taskid, int duration) {
    int size;
    cconsumer_t *mapped_cconsumers[10];
    int i;
    cconsumer_t *cconsumer;
    int current_cconsumerid;
    
    size = (*cconsumers).size;
    memcpy(&mapped_cconsumers, &(*cconsumers).cconsumers, sizeof mapped_cconsumers);
    
    for (i = 0; i < size; i++) {
        cconsumer = mapped_cconsumers[i];
        current_cconsumerid = (*cconsumer).taskid;
        
        if (current_cconsumerid == taskid) {
            cconsumers->cconsumers[i]->duration = duration;
            return true;
        }
    }
    return false;
}
