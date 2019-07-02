#ifndef COMPONENTS_CONTROLSYSTEM_CONTROLSYSTEM_H_
#define COMPONENTS_CONTROLSYSTEM_CONTROLSYSTEM_H_

void start_control_system();

void send_metadata();

int evaluate_request(char *msg, int length);

void encode_msgpack_answer(answer_decoded_t *answer, char **msg, int *size);

#endif /* COMPONENTS_CONTROLSYSTEM_CONTROLSYSTEM_H_ */
