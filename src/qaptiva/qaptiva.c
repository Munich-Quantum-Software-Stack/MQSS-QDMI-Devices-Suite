
#include "qdmi/common/enums.h"
#include <Python.h>
#include <qdmi/device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SCRIPT_LOCATION "QLM_AUXILIARY_SCRIPT_LOCATION"
#define SCRIPT_NAME "QLM_AUXILIARY_SCRIPT_NAME"
#define REMOTE_QPU_CREATE_FUNCTION_NAME "create_remote_qpu"
#define SUBMIT_JOB_FUNCTION_NAME "submit_job"


typedef struct QDMI_Job_impl_d {
  int id;
  QDMI_Job_Status status;
  size_t num_shots;

  pthread_t offload_thread;
  PyObject *qlm_job;
  PyObject *results;

  double *probability_dense;
  char *probability_keys;
  double *probability_values;
  int n_state;
  size_t results_size;

  char *qasm_prog;

} QDMI_Job_impl_t;

typedef struct QDMI_Site_impl_d {
  size_t id;
} QDMI_Site_impl_t;

const QDMI_Site DEVICE_SITES[] = {
    &(QDMI_Site_impl_t){0},  &(QDMI_Site_impl_t){1},
    &(QDMI_Site_impl_t){2},  &(QDMI_Site_impl_t){3},
    &(QDMI_Site_impl_t){4},  &(QDMI_Site_impl_t){5},
    &(QDMI_Site_impl_t){6},  &(QDMI_Site_impl_t){7},
    &(QDMI_Site_impl_t){8},  &(QDMI_Site_impl_t){9},
    &(QDMI_Site_impl_t){10}, &(QDMI_Site_impl_t){11},
    &(QDMI_Site_impl_t){12}, &(QDMI_Site_impl_t){13},
    &(QDMI_Site_impl_t){14}, &(QDMI_Site_impl_t){15},
    &(QDMI_Site_impl_t){16}, &(QDMI_Site_impl_t){17},
    &(QDMI_Site_impl_t){18}, &(QDMI_Site_impl_t){19},
    &(QDMI_Site_impl_t){20}, &(QDMI_Site_impl_t){21},
    &(QDMI_Site_impl_t){22}, &(QDMI_Site_impl_t){23},
    &(QDMI_Site_impl_t){24}, &(QDMI_Site_impl_t){25},
    &(QDMI_Site_impl_t){26}, &(QDMI_Site_impl_t){27},
    &(QDMI_Site_impl_t){28}, &(QDMI_Site_impl_t){29},
    &(QDMI_Site_impl_t){30}, &(QDMI_Site_impl_t){31},
    &(QDMI_Site_impl_t){32}, &(QDMI_Site_impl_t){33},
    &(QDMI_Site_impl_t){34}, &(QDMI_Site_impl_t){35},
    &(QDMI_Site_impl_t){36}, &(QDMI_Site_impl_t){37},
    &(QDMI_Site_impl_t){38}

};

#define ADD_STRING_PROPERTY(prop_name, prop_value, prop, size, value,          \
                            size_ret)                                          \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != NULL) {                                                   \
        if ((size) < strlen(prop_value) + 1) {                                 \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        strncpy((char *)(value), prop_value, (size)-1);                        \
        ((char *)(value))[(size)-1] = '\0';                                    \
      }                                                                        \
      if ((size_ret) != NULL) {                                                \
        *(size_ret) = strlen(prop_value) + 1;                                  \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

#define ADD_SINGLE_VALUE_PROPERTY(prop_name, prop_type, prop_value, prop,      \
                                  size, value, size_ret)                       \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != NULL) {                                                   \
        if ((size) < sizeof(prop_type)) {                                      \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        *(prop_type *)(value) = prop_value;                                    \
      }                                                                        \
      if ((size_ret) != NULL) {                                                \
        *(size_ret) = sizeof(prop_type);                                       \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

#define CHECK_PYTHON_ERROR(value)                                              \
  {                                                                            \
    if (value == NULL) {                                                       \
      PyErr_Print();                                                           \
      Py_Finalize();                                                           \
      return NULL;                                                             \
    }                                                                          \
  }

/* QUERY INTERFACE STARTS*/

static QDMI_Device_Status *QDMI_get_device_status(void) {
  static QDMI_Device_Status device_status = QDMI_DEVICE_STATUS_OFFLINE;
  return &device_status;
}

QDMI_Device_Status QDMI_read_device_status(void) {
  return *QDMI_get_device_status();
}

void QDMI_set_device_status(QDMI_Device_Status status) {
  *QDMI_get_device_status() = status;
}

int QDMI_query_get_sites_dev(const size_t num_entries, QDMI_Site *sites,
                             size_t *num_sites) {
  if ((sites != NULL && num_entries == 0) ||
      (sites == NULL && num_sites == NULL)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  const size_t device_sites_size =
      sizeof(DEVICE_SITES) / sizeof(DEVICE_SITES[0]);
  if (sites != NULL) {
    const size_t copy_size =
        (num_entries < device_sites_size ? num_entries : device_sites_size);
    memcpy((void *)sites, (const void *)DEVICE_SITES,
           copy_size * sizeof(QDMI_Site));
  }
  if (num_sites != NULL) {
    *num_sites = device_sites_size;
  }
  return QDMI_SUCCESS;
}

int QDMI_query_get_operations_dev(size_t num_entries,
                                  QDMI_Operation *operations,
                                  size_t *num_operations) {
  // Since it is a emulator, there is no native gate set.
  return QDMI_ERROR_NOTSUPPORTED;
}

int QDMI_query_device_property_dev(QDMI_Device_Property prop, size_t size,
                                   void *value, size_t *size_ret) {
  if (prop >= QDMI_DEVICE_PROPERTY_MAX || (value == NULL && size_ret == NULL)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_NAME, "QLM", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_VERSION, "0.0.1", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "0.0.1", prop, size,
                      value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_QUBITSNUM, size_t, 38, prop,
                            size, value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_STATUS, QDMI_Device_Status,
                            QDMI_read_device_status(), prop, size, value,
                            size_ret)
  // Since it is a emulator, there is no coupling map.
  return QDMI_ERROR_NOTSUPPORTED;
}

int QDMI_query_site_property_dev(QDMI_Site site, QDMI_Site_Property prop,
                                 size_t size, void *value, size_t *size_ret) {
  // Since it is a emulator, there is no qubit property.
  return QDMI_ERROR_NOTSUPPORTED;
}

int QDMI_query_operation_property_dev(QDMI_Operation operation,
                                      size_t num_sites, const QDMI_Site *sites,
                                      QDMI_Operation_Property prop, size_t size,
                                      void *value, size_t *size_ret) {
  // Since it is a emulator, there is no operation property.
  return QDMI_ERROR_NOTSUPPORTED;
}

/* QUERY INTERFACE ENDS*/

/* CONTROL INTERFACE STARTS*/

int QDMI_control_create_job_dev(const QDMI_Program_Format format,
                                const size_t size, const void *prog,
                                QDMI_Job *job) {

  if (QDMI_read_device_status() != QDMI_DEVICE_STATUS_IDLE) {
    return QDMI_ERROR_FATAL;
  }
  if (size == 0 || prog == NULL || *job != NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (format != QDMI_PROGRAM_FORMAT_QASM2) {
    return QDMI_ERROR_NOTSUPPORTED;
  }

  *job = (QDMI_Job)malloc(sizeof(QDMI_Job_impl_t));
  (*job)->id = rand();
  (*job)->status = QDMI_JOB_STATUS_CREATED;
  (*job)->num_shots = 0;
  (*job)->results = NULL;
  (*job)->qasm_prog = malloc(size);
  (*job)->probability_dense = NULL;
  strncpy((*job)->qasm_prog, prog, size);
  return QDMI_SUCCESS;
}

int QDMI_control_set_parameter_dev(QDMI_Job job, const QDMI_Job_Parameter param,
                                   const size_t size, const void *value) {
  if (job == NULL || param >= QDMI_JOB_PARAMETER_MAX || size == 0 ||
      job->status != QDMI_JOB_STATUS_CREATED) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (param == QDMI_JOB_PARAMETER_SHOTS_NUM) {
    job->num_shots = *(const size_t *)value;
    return QDMI_SUCCESS;
  }
  return QDMI_ERROR_NOTSUPPORTED;
}

static PyObject **get_remote_qpu(void) {
  static PyObject *remote_qpu = NULL;
  return &remote_qpu;
}

static PyObject **get_custom_python_module(void) {
  static PyObject *custom_python_module = NULL;
  return &custom_python_module;
}

void *submit_job(void *arg) {

  QDMI_Job pJob = arg;

  PyObject *pFunc = PyObject_GetAttrString(*get_custom_python_module(),
                                           SUBMIT_JOB_FUNCTION_NAME);
  CHECK_PYTHON_ERROR(pFunc)

  PyObject *pArgs =
      PyTuple_Pack(3, *get_remote_qpu(), PyUnicode_FromString(pJob->qasm_prog),
                   PyLong_FromLong(pJob->num_shots));

  CHECK_PYTHON_ERROR(pArgs)

  PyObject *pResults = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pResults)

  size_t resultSize = PyList_GET_SIZE(pResults);
  PyObject *pBitring = PyList_GET_ITEM(pResults, 0);

  asprintf(&(pJob->probability_keys), "%s", PyUnicode_AsUTF8(pBitring));
  pJob->probability_values = malloc(sizeof(double) * resultSize);
  PyObject *pProbability;
  for (size_t i = 1; i < resultSize; i++) {
    pProbability = PyList_GetItem(pResults, i);
    pJob->probability_values[i - 1] = PyFloat_AS_DOUBLE(pProbability);
  }

  pJob->results_size = resultSize;

  return 0;
}

int QDMI_control_submit_job_dev(QDMI_Job job) {
  QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);
  pthread_create(&(job->offload_thread), NULL, submit_job, (void *)job);
  job->status = QDMI_JOB_STATUS_RUNNING;
  return QDMI_SUCCESS;
}

int QDMI_control_cancel_dev(QDMI_Job job) {
  int isErr = pthread_cancel(job->offload_thread);
  if (!isErr)
    return QDMI_SUCCESS;
  return QDMI_ERROR_FATAL;
}

int QDMI_control_check_dev(QDMI_Job job, QDMI_Job_Status *status) {
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  *status = job->status;

  return QDMI_SUCCESS;
}

int QDMI_control_wait_dev(QDMI_Job job) {
  pthread_join(job->offload_thread, NULL);
  job->status = QDMI_JOB_STATUS_DONE;
  return QDMI_SUCCESS;
}

int QDMI_control_get_data_dev(QDMI_Job job, QDMI_Job_Result result, size_t size,
                              void *data, size_t *size_ret) {

  if (job->status != QDMI_JOB_STATUS_DONE)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (result != QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS &&
      result != QDMI_JOB_RESULT_PROBABILITIES_DENSE &&
      result != QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES)
    return QDMI_ERROR_NOTSUPPORTED;

  *size_ret = job->results_size;
  void *src;

  if (result == QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES) {
    data = job->probability_values;
    *size_ret *= sizeof(double);
    return QDMI_SUCCESS;
  }

  if (result == QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS) {
    data = job->probability_keys;
    return QDMI_SUCCESS;
  }

  if (result == QDMI_JOB_RESULT_PROBABILITIES_DENSE) {
    double *data_ptr = data;
    if (job->probability_dense == NULL) {
      char *bitstream;

      asprintf(&bitstream, "%s", job->probability_keys);
      char *key = strtok(bitstream, ",");

      int n_qubit = strlen(key);
      size_t n_state = 1 << n_qubit;

      job->n_state = n_state;
      job->probability_dense = malloc(sizeof(double) * n_state);

      char *endptr;
      int stream_index = 0;
      memset(job->probability_dense, 0, sizeof(double) * n_state);

      while (key != NULL) {
        int index = strtol(key, &endptr, 2);
        key = strtok(NULL, ",");
        job->probability_dense[index] = job->probability_values[stream_index];
        data_ptr[index] = job->probability_values[stream_index++];
      }
      *size_ret = sizeof(double) * n_state;
    } else {
      for (int index = 0; index < 1 << job->n_state; index++) {
        data_ptr[index] = job->probability_dense[index];
      }
    }
    return QDMI_SUCCESS;
  }

  return QDMI_ERROR_NOTSUPPORTED;
}

void QDMI_control_free_job_dev(QDMI_Job job) {
  free(job->probability_dense);
  free(job->probability_keys);
  free(job->probability_values);

  free(job->qlm_job);
  free(job->results);

  free(job);
}

void *initialize_python() {

  Py_Initialize();

  PyObject *sysPath = PyImport_ImportModule("sys");
  PyObject *path = PyObject_GetAttrString(sysPath, "path");
  PyList_Append(path, PyUnicode_FromString(getenv(SCRIPT_LOCATION)));

  PyObject *pName = PyUnicode_DecodeFSDefault(getenv(SCRIPT_NAME));
  CHECK_PYTHON_ERROR(pName)
  *get_custom_python_module() = PyImport_Import(pName);

  return 0;
}

void *create_remote_qpu() {

  PyObject *pFunc = PyObject_GetAttrString(*get_custom_python_module(),
                                           REMOTE_QPU_CREATE_FUNCTION_NAME);

  CHECK_PYTHON_ERROR(pFunc)

  PyObject *result = PyObject_CallObject(pFunc, NULL);

  CHECK_PYTHON_ERROR(result)
  Py_DECREF(pFunc);

  *get_remote_qpu() = result;

  return NULL;
}

int QDMI_control_initialize_dev() {
  initialize_python();
  create_remote_qpu();
  QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  return QDMI_SUCCESS;
}

int QDMI_control_finalize_dev(void) {
  QDMI_set_device_status(QDMI_DEVICE_STATUS_OFFLINE);
  Py_Finalize();
  return QDMI_SUCCESS;
}
