#ifndef QDMIBACKENDIBM_H
#define QDMIBACKENDIBM_H

#include "private/qdmi_internal.h"

#include <dlfcn.h>
#include <string.h>
#include <stdbool.h>
#include <jansson.h>
#include <unistd.h>

extern json_error_t error;
extern json_t *ibm_root;
extern json_t *ibm_properties;
extern char **gate_set;

extern char *backend_properties[];

extern char * qubit_properties[];

int fetch_configuration();
#endif // QDMIBACKENDIBM_H