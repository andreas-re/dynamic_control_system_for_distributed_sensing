#include <stdio.h>
#include "metadata.h"

// function to compare a location
bool compare_location(int cmp_type, char *key, metadata_kv *client_metadata, metadata_kv *cmp_metadata) {
	printf("client_metadata: \n");
	metadata_print_kv(client_metadata);
	metadata_print_kv(cmp_metadata);
	printf("cmp_type: %i, key: %s\n", cmp_type, key);
	if(strcmp(key, "location") == 0) return true;	
		else return false;
}

void register_compare_functions() {
	metadata_add_compare_function(&compare_location, 30);
}
