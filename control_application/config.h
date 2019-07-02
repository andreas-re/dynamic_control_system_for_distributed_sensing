#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "metadata.h"

extern metadata_kv *config_metadata_g;

int evaluate_json_configuration(char *json);
int read_configuration();

#endif /* _CONFIG_H_ */
