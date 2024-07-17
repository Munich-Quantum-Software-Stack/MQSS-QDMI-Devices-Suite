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

const char *backend_properties[] = 
{
    "backend_name", "backend_version",
    "n_qubits", "basis_gates", "gates", "coupling_map"
};

const char * qubit_properties[] =
{
    "T1", "T2", "readout_error", "readout_length"
};
int fetch_configuration();
#endif // QDMIBACKENDIBM_H