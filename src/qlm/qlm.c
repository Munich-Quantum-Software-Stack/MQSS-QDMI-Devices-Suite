/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

#include "qdmi/constants.h"
#include <Python.h>
#include <qlm_qdmi/device.h>

#define SCRIPT_LOCATION "QLM_AUXILIARY_SCRIPT_LOCATION"
#define SCRIPT_NAME "QLM_AUXILIARY_SCRIPT_NAME"
#define REMOTE_QPU_CREATE_FUNCTION_NAME "create_remote_qpu"
#define SUBMIT_JOB_FUNCTION_NAME "submit_job"

#define DEFAULT_NUM_SHOT 0

enum QLM_QDMI_DEVICE_SESSION_STATUS { ALLOCATED, INITIALIZED };

enum QDMI_Device_Session_Parameter { QDMI_DEVICE_SESSION_PARAMETER_PORT };

typedef struct QLM_QDMI_Device_Session_impl_d {
  char *url;
  enum QLM_QDMI_DEVICE_SESSION_STATUS status;
} QLM_QDMI_Device_Session_impl_t;

typedef struct QLM_QDMI_Device_Job_impl_d {
  QLM_QDMI_Device_Session session;
  int id;
  QDMI_Job_Status status;
  size_t num_shots;

  pthread_t offload_thread;
  pthread_rwlock_t mutex;
  PyObject *qlm_job;
  PyObject *results;

  double *probability_dense;
  char *probability_keys;
  double *probability_values;
  size_t n_state;
  size_t results_size;

  QDMI_Program_Format *format;
  char *program;
} QLM_QDMI_Device_Job_impl_t;

typedef struct QLM_QDMI_Site_impl_d {
  size_t id;
} QLM_QDMI_Site_impl_t;

const QLM_QDMI_Site DEVICE_SITES[] = {
    &(QLM_QDMI_Site_impl_t){0},  &(QLM_QDMI_Site_impl_t){1},
    &(QLM_QDMI_Site_impl_t){2},  &(QLM_QDMI_Site_impl_t){3},
    &(QLM_QDMI_Site_impl_t){4},  &(QLM_QDMI_Site_impl_t){5},
    &(QLM_QDMI_Site_impl_t){6},  &(QLM_QDMI_Site_impl_t){7},
    &(QLM_QDMI_Site_impl_t){8},  &(QLM_QDMI_Site_impl_t){9},
    &(QLM_QDMI_Site_impl_t){10}, &(QLM_QDMI_Site_impl_t){11},
    &(QLM_QDMI_Site_impl_t){12}, &(QLM_QDMI_Site_impl_t){13},
    &(QLM_QDMI_Site_impl_t){14}, &(QLM_QDMI_Site_impl_t){15},
    &(QLM_QDMI_Site_impl_t){16}, &(QLM_QDMI_Site_impl_t){17},
    &(QLM_QDMI_Site_impl_t){18}, &(QLM_QDMI_Site_impl_t){19},
    &(QLM_QDMI_Site_impl_t){20}, &(QLM_QDMI_Site_impl_t){21},
    &(QLM_QDMI_Site_impl_t){22}, &(QLM_QDMI_Site_impl_t){23},
    &(QLM_QDMI_Site_impl_t){24}, &(QLM_QDMI_Site_impl_t){25},
    &(QLM_QDMI_Site_impl_t){26}, &(QLM_QDMI_Site_impl_t){27},
    &(QLM_QDMI_Site_impl_t){28}, &(QLM_QDMI_Site_impl_t){29},
    &(QLM_QDMI_Site_impl_t){30}, &(QLM_QDMI_Site_impl_t){31},
    &(QLM_QDMI_Site_impl_t){32}, &(QLM_QDMI_Site_impl_t){33},
    &(QLM_QDMI_Site_impl_t){34}, &(QLM_QDMI_Site_impl_t){35},
    &(QLM_QDMI_Site_impl_t){36}, &(QLM_QDMI_Site_impl_t){37},
    &(QLM_QDMI_Site_impl_t){38}

};

#define ADD_STRING_PROPERTY(prop_name, prop_value, prop, size, value,          \
                            size_ret)                                          \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != NULL) {                                                   \
        if ((size) < strlen(prop_value) + 1) {                                 \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        strncpy((char *)(value), prop_value, (size) - 1);                      \
        ((char *)(value))[(size) - 1] = '\0';                                  \
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

#define ADD_LIST_PROPERTY(prop_name, prop_type, prop_values, prop_length,      \
                          prop, size, value, size_ret)                         \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != NULL) {                                                   \
        if ((size) < (prop_length) * sizeof(prop_type)) {                      \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        memcpy((void *)(value), (const void *)(prop_values),                   \
               (prop_length) * sizeof(prop_type));                             \
      }                                                                        \
      if ((size_ret) != NULL) {                                                \
        *(size_ret) = (prop_length) * sizeof(prop_type);                       \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

#define CHECK_PYTHON_ERROR(value, ret_val, from_python)                        \
  {                                                                            \
    if (value == Py_None || value == NULL) {                                   \
                                                                               \
      PyErr_Print();                                                           \
      if (!from_python) {                                                      \
        PyGILState_Release(gstate);                                            \
      }                                                                        \
      QLM_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);                     \
      return ret_val;                                                          \
    }                                                                          \
  }

#define CHECK_QDMI_ERROR(value)                                                \
  {                                                                            \
    if (value != QDMI_SUCCESS) {                                               \
      return value;                                                            \
    }                                                                          \
  }

#define GET_VALUE_DATA(type, multiplier)                                       \
  {                                                                            \
    required_size = job->results_size * sizeof(type);                          \
    if (data) {                                                                \
      if (size < required_size)                                                \
        return QDMI_ERROR_INVALIDARGUMENT;                                     \
      type *data_ptr = data;                                                   \
      int max_index = (int)(size / sizeof(type));                              \
      for (int index = 0; index < max_index; index++) {                        \
        *data_ptr++ = (type)(job->probability_values[index] * multiplier);     \
      }                                                                        \
    }                                                                          \
    if (size_ret)                                                              \
      *size_ret = required_size;                                               \
    return QDMI_SUCCESS;                                                       \
  }

/* QUERY INTERFACE STARTS*/

static QDMI_Device_Status *QLM_QDMI_get_device_status(void) {
  static QDMI_Device_Status device_status = QDMI_DEVICE_STATUS_OFFLINE;
  return &device_status;
}

QDMI_Device_Status QLM_QDMI_read_device_status(void) {
  return *QLM_QDMI_get_device_status();
}

void QLM_QDMI_set_device_status(QDMI_Device_Status status) {
  *QLM_QDMI_get_device_status() = status;
}

static int *isFromPython(void) {
  static int isFromPython = 0;
  return &isFromPython;
}

int QLM_QDMI_device_session_query_operation_property(
    QLM_QDMI_Device_Session session, QLM_QDMI_Operation operation,
    const size_t num_sites, const QLM_QDMI_Site *sites, const size_t num_params,
    const double *params, const QDMI_Operation_Property prop, const size_t size,
    void *value, size_t *size_ret) {
  // Since it is a emulator, there is no native gate set.
  return QDMI_ERROR_NOTSUPPORTED;
}

int QLM_QDMI_device_session_query_device_property(
    QLM_QDMI_Device_Session session, const QDMI_Device_Property prop,
    const size_t size, void *value, size_t *size_ret) {
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
                            QLM_QDMI_read_device_status(), prop, size, value,
                            size_ret)
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_SITES, QLM_QDMI_Site, DEVICE_SITES, 38,
                    prop, size, value, size_ret)
  // Since it is a emulator, there is no coupling map.
  return QDMI_ERROR_NOTSUPPORTED;
}

int QLM_QDMI_device_session_query_site_property(QLM_QDMI_Device_Session session,
                                                QLM_QDMI_Site site,
                                                const QDMI_Site_Property prop,
                                                const size_t size, void *value,
                                                size_t *size_ret) {

  // Since it is a emulator, there is no qubit property.
  if (session == NULL || site == NULL || (value != NULL && size == 0) ||
      prop >= QDMI_SITE_PROPERTY_MAX) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (prop != QDMI_SITE_PROPERTY_ID)
    return QDMI_ERROR_NOTSUPPORTED;

  ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_ID, uint64_t, site->id, prop,
                            size, value, size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

/* QUERY INTERFACE ENDS*/

/* CONTROL INTERFACE STARTS*/

int QLM_QDMI_device_session_create_device_job(QLM_QDMI_Device_Session session,
                                              QLM_QDMI_Device_Job *job) {

  if (session == NULL || job == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  *job = (QLM_QDMI_Device_Job)malloc(sizeof(QLM_QDMI_Device_Job_impl_t));
  (*job)->session = session;

  (*job)->id = rand();
  (*job)->status = QDMI_JOB_STATUS_CREATED;
  (*job)->num_shots = DEFAULT_NUM_SHOT;
  (*job)->program = NULL;
  (*job)->qlm_job = NULL;

  (*job)->results = NULL;
  (*job)->probability_dense = NULL;
  (*job)->probability_keys = NULL;
  (*job)->probability_values = NULL;

  return QDMI_SUCCESS;
}

int QLM_QDMI_device_job_set_parameter(QLM_QDMI_Device_Job job,
                                      const QDMI_Device_Job_Parameter param,
                                      const size_t size, const void *value) {
  if (job == NULL || (value != NULL && size == 0) || (value == NULL) ||
      (param >= QDMI_DEVICE_JOB_PARAMETER_MAX &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM2 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM3 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM4 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (job->status != QDMI_JOB_STATUS_CREATED) {
    return QDMI_ERROR_BADSTATE;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT) {
    QDMI_Program_Format format = *(const QDMI_Program_Format *)value;
    if (format != QDMI_PROGRAM_FORMAT_QASM2)
      return QDMI_ERROR_NOTSUPPORTED;
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAM) {
    job->program = malloc(size);
    strcpy(job->program, (const char *)value);
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM) {
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
  QLM_QDMI_Device_Job pJob = arg;
  PyGILState_STATE gstate;

  if (!*isFromPython())
    gstate = PyGILState_Ensure();

  PyObject *custom_python_module = *get_custom_python_module();

  PyObject *pFunc =
      PyObject_GetAttrString(custom_python_module, SUBMIT_JOB_FUNCTION_NAME);
  CHECK_PYTHON_ERROR(pFunc, (void *)(QDMI_ERROR_FATAL), *isFromPython())

  PyObject **remote_qpu = get_remote_qpu();
  CHECK_PYTHON_ERROR(*remote_qpu, (void *)(QDMI_ERROR_FATAL), *isFromPython())
  PyObject *qasm_string = PyUnicode_FromString(pJob->program);
  CHECK_PYTHON_ERROR(qasm_string, (void *)(QDMI_ERROR_FATAL), *isFromPython())
  PyObject *num_shot = PyLong_FromUnsignedLong(pJob->num_shots);
  CHECK_PYTHON_ERROR(num_shot, (void *)(QDMI_ERROR_FATAL), *isFromPython())

  PyObject *pArgs = PyTuple_Pack(3, *remote_qpu, qasm_string, num_shot);
  CHECK_PYTHON_ERROR(pArgs, (void *)(QDMI_ERROR_FATAL), *isFromPython())

  pJob->status = QDMI_JOB_STATUS_RUNNING;
  QLM_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);

  PyObject *pResults = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pResults, (void *)(QDMI_ERROR_FATAL), *isFromPython())

  unsigned long resultSize = (unsigned long)PyList_GET_SIZE(pResults);
  PyObject *pBitring = PyList_GET_ITEM(pResults, 0);

  asprintf(&(pJob->probability_keys), "%s", PyUnicode_AsUTF8(pBitring));
  pJob->probability_values = malloc(sizeof(double) * resultSize);
  PyObject *pProbability;
  for (size_t i = 1; i < resultSize; i++) {
    pProbability = PyList_GetItem(pResults, (long)i);
    pJob->probability_values[i - 1] = PyFloat_AS_DOUBLE(pProbability);
  }
  pJob->results_size = resultSize - 1;

  pJob->status = QDMI_JOB_STATUS_DONE;
  QLM_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);

  if (!*isFromPython())
    PyGILState_Release(gstate);

  return 0;
}

int QLM_QDMI_device_job_submit(QLM_QDMI_Device_Job job) {

  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status != QDMI_JOB_STATUS_CREATED || job->program == NULL ||
      job->num_shots == DEFAULT_NUM_SHOT)
    return QDMI_ERROR_INVALIDARGUMENT;

  job->status = QDMI_JOB_STATUS_SUBMITTED;
  int isErr =
      pthread_create(&(job->offload_thread), NULL, submit_job, (void *)job);

  if (isErr)
    return QDMI_ERROR_FATAL;

  return QDMI_SUCCESS;
}

int QLM_QDMI_device_job_cancel(QLM_QDMI_Device_Job job) {
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status != QDMI_JOB_STATUS_RUNNING &&
      job->status != QDMI_JOB_STATUS_SUBMITTED &&
      job->status != QDMI_JOB_STATUS_CREATED)
    return QDMI_ERROR_INVALIDARGUMENT;
  if (job->status != QDMI_JOB_STATUS_CREATED) {
    void *ret_val;
    int isErr = pthread_join(job->offload_thread, &(ret_val));

    if (ret_val != NULL || isErr)
      return QDMI_ERROR_FATAL;
  }

  job->status = QDMI_JOB_STATUS_CANCELED;
  QLM_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);

  return QDMI_SUCCESS;
}

int QLM_QDMI_device_job_check(QLM_QDMI_Device_Job job,
                              QDMI_Job_Status *status) {

  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  // If the job is canceled
  if (job->status < QDMI_JOB_STATUS_CREATED ||
      job->status > QDMI_JOB_STATUS_CANCELED)
    return QDMI_ERROR_INVALIDARGUMENT;

  *status = job->status;

  return QDMI_SUCCESS;
}

int QLM_QDMI_device_job_wait(QLM_QDMI_Device_Job job) {
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status == QDMI_JOB_STATUS_CANCELED)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status != QDMI_JOB_STATUS_DONE &&
      job->status != QDMI_JOB_STATUS_CREATED) {
    void *ret_val = NULL;
    pthread_join(job->offload_thread, &ret_val);
    if (ret_val != NULL)
      job->status = QDMI_JOB_STATUS_CANCELED;
    else
      job->status = QDMI_JOB_STATUS_DONE;
  }
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_job_get_results(QLM_QDMI_Device_Job job,
                                    QDMI_Job_Result result, size_t size,
                                    void *data, size_t *size_ret) {
  if (job->status != QDMI_JOB_STATUS_DONE)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (result == QDMI_JOB_RESULT_STATEVECTOR_DENSE ||
      result == QDMI_JOB_RESULT_STATEVECTOR_SPARSE_KEYS ||
      result == QDMI_JOB_RESULT_STATEVECTOR_SPARSE_VALUES)
    return QDMI_ERROR_NOTSUPPORTED;

  size_t required_size;
  if (result == QDMI_JOB_RESULT_HIST_KEYS ||
      result == QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS) {
    required_size = strlen(job->probability_keys);
    if (data) {
      if (size < required_size)
        return QDMI_ERROR_INVALIDARGUMENT;
      strncpy(data, job->probability_keys, size);
      return QDMI_SUCCESS;
    }
    if (size_ret)
      *size_ret = required_size;
    return QDMI_SUCCESS;
  }

  if (result == QDMI_JOB_RESULT_HIST_VALUES)
    GET_VALUE_DATA(int, job->num_shots)

  if (result == QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES)
    GET_VALUE_DATA(double, 1)

  if (result == QDMI_JOB_RESULT_PROBABILITIES_DENSE) {
    if (job->probability_dense == NULL) {
      char *bitstream, *key, *endptr;

      size_t n_qubit = 0;
      int stream_index = 0;

      asprintf(&bitstream, "%s", job->probability_keys);
      while (bitstream[++n_qubit] != ',')
        ;
      job->n_state = 1 << n_qubit;
      required_size = job->n_state * sizeof(double);

      job->probability_dense = malloc(required_size);
      memset(job->probability_dense, 0, sizeof(double) * job->n_state);

      key = strtok(bitstream, ",");
      while (key != NULL) {
        long index = strtol(key, &endptr, 2);
        key = strtok(NULL, ",");
        job->probability_dense[index] = job->probability_values[stream_index++];
      }
    }
    required_size = job->n_state * sizeof(double);
    if (size_ret)
      *size_ret = required_size;
    if (data) {
      if (size < required_size)
        return QDMI_ERROR_INVALIDARGUMENT;
      double *data_ptr = data;
      memset(data_ptr, 0, sizeof(double) * job->n_state);
      for (int index = 0; index < required_size / sizeof(double); index++) {
        *data_ptr++ = job->probability_dense[index];
      }
    }
  }

  return QDMI_SUCCESS;
}

void QLM_QDMI_device_job_free(QLM_QDMI_Device_Job job) {
  if (job->status == QDMI_JOB_STATUS_SUBMITTED ||
      job->status == QDMI_JOB_STATUS_RUNNING) {
    pthread_join(job->offload_thread, NULL);
  }

  free(job->probability_dense);
  job->probability_dense = NULL;
  free(job->probability_keys);
  job->probability_keys = NULL;
  free(job->probability_values);
  job->probability_values = NULL;

  free(job->qlm_job);
  job->qlm_job = NULL;
  free(job->results);
  job->results = NULL;

  free(job);
  job = NULL;
}

int initialize_python(void) {

  char *script_location = getenv(SCRIPT_LOCATION);
  char *script_name = getenv(SCRIPT_NAME);

  *isFromPython() = Py_IsInitialized();
  PyGILState_STATE gstate;
  if (!*isFromPython()) {
    Py_Initialize();
    PyThreadState *_save = PyEval_SaveThread();
    gstate = PyGILState_Ensure();
  }

  PyObject *sysPath = PyImport_ImportModule("sys");
  PyObject *path = PyObject_GetAttrString(sysPath, "path");

  PyList_Append(path, PyUnicode_FromString(script_location));

  PyObject *pName = PyUnicode_DecodeFSDefault(script_name);
  CHECK_PYTHON_ERROR(pName, QDMI_ERROR_FATAL, *isFromPython())

  *get_custom_python_module() = PyImport_Import(pName);

  CHECK_PYTHON_ERROR(*get_custom_python_module(), QDMI_ERROR_FATAL,
                     *isFromPython());

  Py_XDECREF(pName);

  if (!*isFromPython())
    PyGILState_Release(gstate);

  return QDMI_SUCCESS;
}

int create_remote_qpu(const char *hostname) {

  PyGILState_STATE gstate = PyGILState_Ensure();
  PyObject *pFunc = PyObject_GetAttrString(*get_custom_python_module(),
                                           REMOTE_QPU_CREATE_FUNCTION_NAME);

  CHECK_PYTHON_ERROR(pFunc, QDMI_ERROR_FATAL, *isFromPython())

  PyObject *pArgs = PyTuple_Pack(1, PyUnicode_FromString(hostname));
  CHECK_PYTHON_ERROR(pArgs, QDMI_ERROR_FATAL, *isFromPython())

  PyObject *pResult = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pResult, QDMI_ERROR_FATAL, *isFromPython())

  *get_remote_qpu() = pResult;
  PyGILState_Release(gstate);
  return QDMI_SUCCESS;
}

int check_env_variable(void) {
  if (getenv(SCRIPT_LOCATION) == NULL || getenv(SCRIPT_NAME) == NULL)
    return QDMI_ERROR_FATAL;
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_initialize(void) {
  int err = check_env_variable();
  CHECK_QDMI_ERROR(err)

  err = initialize_python();
  CHECK_QDMI_ERROR(err)

  QLM_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_finalize(void) {

  QLM_QDMI_set_device_status(QDMI_DEVICE_STATUS_OFFLINE);
  if (*get_custom_python_module() != NULL) {
    Py_DECREF(*get_custom_python_module());
    *get_custom_python_module() = NULL;
  }

  if (*get_remote_qpu() != NULL) {
    Py_DECREF(*get_remote_qpu());
    *get_remote_qpu() = NULL;
  }

  if (Py_IsInitialized() && !_Py_IsFinalizing() && !*isFromPython()) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    Py_Finalize();
  }
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_session_alloc(QLM_QDMI_Device_Session *session) {
  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  *session =
      (QLM_QDMI_Device_Session)malloc(sizeof(QLM_QDMI_Device_Session_impl_t));
  (*session)->url = NULL;
  (*session)->status = ALLOCATED;
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_session_init(QLM_QDMI_Device_Session session) {
  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  switch (QLM_QDMI_read_device_status()) {
  case QDMI_DEVICE_STATUS_ERROR:
  case QDMI_DEVICE_STATUS_OFFLINE:
  case QDMI_DEVICE_STATUS_MAINTENANCE:
    return QDMI_ERROR_FATAL;
  default:
    break;
  }

  if (session->url == NULL) {
    return QDMI_ERROR_BADSTATE;
  }

  int err = create_remote_qpu(session->url);
  CHECK_QDMI_ERROR(err)

  session->status = INITIALIZED;
  return QDMI_SUCCESS;
}

void QLM_QDMI_device_session_free(QLM_QDMI_Device_Session session) {
  if (session != NULL) {
    free(session->url);
    free(session);
  }
}

int QLM_QDMI_device_session_set_parameter(
    QLM_QDMI_Device_Session session, const QDMI_Device_Session_Parameter param,
    const size_t size, const void *value) {
  if (session == NULL || (value != NULL && size == 0) ||
      (param >= QDMI_DEVICE_SESSION_PARAMETER_MAX &&
       param != QDMI_DEVICE_SESSION_PARAMETER_TOKEN &&
       param != QDMI_DEVICE_SESSION_PARAMETER_BASEURL &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM3 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM4 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (session->status != ALLOCATED) {
    return QDMI_ERROR_BADSTATE;
  }

  if (param == QDMI_DEVICE_SESSION_PARAMETER_BASEURL) {
    session->url = (char *)malloc(size);
    strcpy(session->url, (const char *)value);
  }

  return QDMI_SUCCESS;
}
