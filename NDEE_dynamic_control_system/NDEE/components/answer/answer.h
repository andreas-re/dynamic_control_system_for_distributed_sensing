#ifndef COMPONENTS_ANSWER_ANSWER_H_
#define COMPONENTS_ANSWER_ANSWER_H_

#include "time.h"

typedef struct {
    int status;
    int requestid;
    int timestamp;
    char *measurand;
    char *device;
    char *unit;
    char *type;
    float value;
} answer_decoded_t;

typedef struct {
    char *status;
    //int requestid;
    /* in answer.c the return value of the function integer_to_signed_bitstring is stored
       into this variable (requestid) and the return type is char *
    */
    char *requestid;
    char *timestamp;
    char *measurand;
    char *device;
    char *unit;
    char *type;
    char *value;
} answer_encoded_t;

answer_decoded_t *answer_decoded_new ();

char *encode (answer_decoded_t *answer_decoded);

#endif /* COMPONENTS_ANSWER_ANSWER_H_ */
