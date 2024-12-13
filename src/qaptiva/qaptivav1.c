

#include <Python.h>
#include <qdmi/device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCRIPT_LOCATION "SCRIPT_LOCATION"
#define SCRIPT_NAME "SCRIPT_NAME"
#define FUNCTION_NAME "FUNCTION_NAME"

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

} QDMI_Job_impl_t;

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

int handlePythonError() {
  PyErr_Print();
  Py_Finalize();
  return QDMI_ERROR_FATAL;
}

int createQLMJob(char *qasm_string, PyObject **result) {

  Py_Initialize();
  PyObject *sysPath = PyImport_ImportModule("sys");
  PyObject *path = PyObject_GetAttrString(sysPath, "path");

  PyList_Append(path, PyUnicode_FromString(getenv(SCRIPT_LOCATION)));

  PyObject *pName = PyUnicode_DecodeFSDefault(getenv(SCRIPT_NAME));
  PyObject *pModule = PyImport_Import(pName);

  if (pModule == NULL)
    return handlePythonError();

  PyObject *pFunc = PyObject_GetAttrString(pModule, getenv(FUNCTION_NAME));
  if (pFunc == NULL)
    return handlePythonError();

  PyObject *pArgs = PyTuple_Pack(1, PyUnicode_FromString(qasm_string));

  if (pArgs == NULL)
    return handlePythonError();

  *result = PyObject_CallObject(pFunc, pArgs);
  if (*result == NULL)
    return handlePythonError();

  Py_Finalize();

  return QDMI_SUCCESS;
}

int QDMI_control_initialize_dev() {
  QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
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
  return createQLMJob((char *)prog, &((*job)->qlm_job));
  ;
}

int QDMI_control_cancel_dev(QDMI_Job job) { return QDMI_ERROR_NOTSUPPORTED; }

int QDMI_control_check_dev(QDMI_Job job, QDMI_Job_Status *status) {
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  *status = job->status;

  return QDMI_SUCCESS;
}

void *submit_job(void *arg) {

  QDMI_Job pJob = arg;

  Py_Initialize();
  PyObject *sysPath = PyImport_ImportModule("sys");
  PyObject *path = PyObject_GetAttrString(sysPath, "path");

  PyList_Append(path, PyUnicode_FromString(getenv(SCRIPT_LOCATION)));

  PyObject *pName = PyUnicode_DecodeFSDefault(getenv(SCRIPT_NAME));
  PyObject *pModule = PyImport_Import(pName);

  if (pModule == NULL) {
    handlePythonError();
    return NULL;
  }

  PyObject *pFunc = PyObject_GetAttrString(pModule, "submit_job");
  if (pFunc == NULL) {
    handlePythonError();
    return NULL;
  }

  PyObject *pArgs = PyTuple_Pack(1, pJob->qlm_job);

  if (pArgs == NULL) {
    handlePythonError();
    return NULL;
  }
  pJob->status = QDMI_JOB_STATUS_RUNNING;
  PyObject *pResults = PyObject_CallObject(pFunc, pArgs);

  Py_DECREF(pFunc);
  Py_DECREF(pModule);

  if (!pResults || !PyList_Check(pResults)) {
    PyErr_Print();
    fprintf(stderr, "Function did not return a dictionary\n");
    Py_XDECREF(pResults);
    Py_Finalize();
    return NULL;
  }

  size_t resultSize = PyList_GET_SIZE(pResults);
  PyObject *pKeys = PyList_GET_ITEM(pResults, 0);

  asprintf(&(pJob->probability_keys), "%s", PyUnicode_AsUTF8(pKeys));
  pJob->probability_values = malloc(sizeof(double) * resultSize);
  PyObject *pProbability;
  for (size_t i = 1; i < resultSize; i++) {
    pProbability = PyList_GetItem(pResults, i);
    pJob->probability_values[i - 1] = PyFloat_AS_DOUBLE(pProbability);
  }

  pJob->results_size = resultSize;

  // Py_XDECREF(pKeys);
  // Py_XDECREF(pProbability);
  // Py_XDECREF(pResults);

  Py_Finalize();

  pJob->status = QDMI_JOB_STATUS_DONE;

  return NULL;
}

int QDMI_control_submit_job_dev(QDMI_Job job) {
  pthread_create(&(job->offload_thread), NULL, submit_job, (void *)job);
  job->status = QDMI_JOB_STATUS_RUNNING;
  return QDMI_SUCCESS;
}

int QDMI_control_wait_dev(QDMI_Job job) {
  pthread_join((job->offload_thread), NULL);
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
  }

  if (result == QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS) {
    data = job->probability_keys;
  }

  if (result == QDMI_JOB_RESULT_PROBABILITIES_DENSE) {
    char *bitstream;
    if (job->probability_dense == NULL) {
      
      asprintf(&bitstream, "%s", job->probability_keys);
      char *key = strtok(bitstream, ",");

      int n_qubit = strlen(key);
      size_t n_state = 1 << n_qubit;

      job->n_state = n_state;
      job->probability_dense = malloc(sizeof(double) * n_state);

      char *endptr;
      int stream_index = 0;
      memset(job->probability_dense, 0, sizeof(double) * n_state);

      double *data_ptr = data;
      
      while (key != NULL) {
        int index = strtol(key, &endptr, 2);
        key = strtok(NULL, ",");
        data_ptr[index] = job->probability_values[stream_index++];
      }
      
      *size_ret = sizeof(double) * n_state;
    }
  }

  return QDMI_SUCCESS;
}


/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/
