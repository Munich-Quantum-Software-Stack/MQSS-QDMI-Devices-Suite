
#include "qdmi/constants.h"
#include <Python.h>

#include <qlm_qdmi/device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SCRIPT_LOCATION "QLM_AUXILIARY_SCRIPT_LOCATION"
#define SCRIPT_NAME "QLM_AUXILIARY_SCRIPT_NAME"
#define REMOTE_QPU_CREATE_FUNCTION_NAME "create_remote_qpu"
#define SUBMIT_JOB_FUNCTION_NAME "submit_job"

#define PORT_INIT_VALUE -1
#define DEFAULT_NUM_SHOT 0

enum QLM_QDMI_DEVICE_SESSION_STATUS { ALLOCATED, INITIALIZED };

enum QDMI_Device_Session_Parameter { QDMI_DEVICE_SESSION_PARAMETER_PORT };

typedef struct QLM_QDMI_Device_Session_impl_d {
  char *base_url;
  int port;
  enum QLM_QDMI_DEVICE_SESSION_STATUS status;
} QLM_QDMI_Device_Session_impl_t;

typedef struct QLM_QDMI_Device_Job_impl_d {
  QLM_QDMI_Device_Session session;
  int id;
  QDMI_Job_Status status;
  size_t num_shots;

  pthread_t offload_thread;
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

#define CHECK_PYTHON_ERROR(value)                                              \
  {                                                                            \
    if (value == NULL) {                                                       \
      PyErr_Print();                                                           \
      Py_Finalize();                                                           \
      return NULL;                                                             \
    }                                                                          \
  }

#define CHECK_QDMI_ERROR(value)                                                \
  {                                                                            \
    if (value != QDMI_SUCCESS) {                                               \
      return value;                                                            \
    }                                                                          \
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

int QLM_QDMI_device_session_query_operation_property(
    QLM_QDMI_Device_Session session, QLM_QDMI_Operation operation,
    const size_t num_sites, const QLM_QDMI_Site *sites, const size_t num_params,
    const double *params, const QDMI_Operation_Property prop, const size_t size,
    void *value, size_t *size_ret) {
  if (session == NULL || operation == NULL ||
      (sites != NULL && num_sites == 0) ||
      (params != NULL && num_params == 0) || (value != NULL && size == 0) ||
      (prop >= QDMI_OPERATION_PROPERTY_MAX &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM1 &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM2 &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM3 &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM4 &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
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
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_SITES, QLM_QDMI_Site, DEVICE_SITES, 5,
                    prop, size, value, size_ret)
  // Since it is a emulator, there is no coupling map.
  return QDMI_ERROR_NOTSUPPORTED;
}

int QLM_QDMI_device_session_query_site_property(QLM_QDMI_Device_Session session,
                                                QLM_QDMI_Site site,
                                                const QDMI_Site_Property prop,
                                                const size_t size, void *value,
                                                size_t *size_ret) {
  if (session == NULL || site == NULL || (value != NULL && size == 0) ||
      (prop >= QDMI_SITE_PROPERTY_MAX && prop != QDMI_SITE_PROPERTY_CUSTOM1 &&
       prop != QDMI_SITE_PROPERTY_CUSTOM2 &&
       prop != QDMI_SITE_PROPERTY_CUSTOM3 &&
       prop != QDMI_SITE_PROPERTY_CUSTOM4 &&
       prop != QDMI_SITE_PROPERTY_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_ID, uint64_t, site->id, prop,
                            size, value, size_ret)
  // Since it is a emulator, there is no qubit property.
  return QDMI_ERROR_NOTSUPPORTED;
}

/* QUERY INTERFACE ENDS*/

/* CONTROL INTERFACE STARTS*/

int QLM_QDMI_device_session_create_device_job(QLM_QDMI_Device_Session session,
                                              QLM_QDMI_Device_Job *job) {

  if (session == NULL || job == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (session->status != INITIALIZED) {
    return QDMI_ERROR_BADSTATE;
  }

  *job = (QLM_QDMI_Device_Job)malloc(sizeof(QLM_QDMI_Device_Job_impl_t));
  (*job)->session = session;
  // set job id to random number for demonstration purposes
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
  // printf("submit_job1\n");
  PyObject *pFunc = PyObject_GetAttrString(*get_custom_python_module(),
                                           SUBMIT_JOB_FUNCTION_NAME);
  CHECK_PYTHON_ERROR(pFunc)

  // // printf("submit_job2 %zu\n", pJob->num_shots);
  // // printf("submit_job2,5 %s\n", pJob->program);
  // printf("1\n");
  PyObject **remote_qpu = get_remote_qpu();
  CHECK_PYTHON_ERROR(remote_qpu)

  // printf("2\n");
  PyObject *qasm_string = PyUnicode_FromString(pJob->program);
  CHECK_PYTHON_ERROR(remote_qpu)
  // printf("3\n");
  PyObject *num_shot = PyLong_FromUnsignedLong(pJob->num_shots);
  CHECK_PYTHON_ERROR(num_shot)

  printf("%s\n",pJob->program );
  PyObject *pArgs = PyTuple_Pack(3, *remote_qpu, qasm_string, num_shot);

  CHECK_PYTHON_ERROR(pArgs)

  // printf("6\n");

  // printf("submit_job3\n");
  PyObject *pResults = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pResults)
/*  // printf("submit_job4\n");
  unsigned long resultSize = (unsigned long)PyList_GET_SIZE(pResults);
  PyObject *pBitring = PyList_GET_ITEM(pResults, 0);

  asprintf(&(pJob->probability_keys), "%s", PyUnicode_AsUTF8(pBitring));
  pJob->probability_values = malloc(sizeof(double) * resultSize);
  PyObject *pProbability;
  for (size_t i = 1; i < resultSize; i++) {
    pProbability = PyList_GetItem(pResults, (long)i);
    pJob->probability_values[i - 1] = PyFloat_AS_DOUBLE(pProbability);
  }
  // printf("submit_job5\n");


  pJob->results_size = resultSize;
  */

  return 0;
}

int QLM_QDMI_device_job_submit(QLM_QDMI_Device_Job job) {
  // printf("QLM_QDMI_device_job_submit1\n");
  
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;
  // printf("QLM_QDMI_device_job_submit2\n");
  if (job->status != QDMI_JOB_STATUS_CREATED || job->program == NULL ||
      job->num_shots == DEFAULT_NUM_SHOT)
    return QDMI_ERROR_INVALIDARGUMENT;
  // printf("QLM_QDMI_device_job_submit3\n");
  QLM_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);


  job->status = QDMI_JOB_STATUS_RUNNING;
  
  
  submit_job((void *)job);
  /*
  int isErr =
      pthread_create(&(job->offload_thread), NULL, submit_job, (void *)job);

   if (isErr)
    return QDMI_ERROR_FATAL;
  
  */
  
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_job_cancel(QLM_QDMI_Device_Job job) {
  // printf("QLM_QDMI_device_job_cancel 1\n");
  /*
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  // printf("QLM_QDMI_device_job_cancel 2\n");
  if (job->status != QDMI_JOB_STATUS_RUNNING)
    return QDMI_ERROR_INVALIDARGUMENT;
  // printf("QLM_QDMI_device_job_cancel 3\n");

  // printf("%lu\n", job->offload_thread);
  int isErr = pthread_cancel(job->offload_thread);
  if (!isErr)
    return QDMI_SUCCESS;
  // printf("QLM_QDMI_device_job_cancel 3\n");

  job->status = QDMI_JOB_STATUS_CANCELED;
  QLM_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  */
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_job_check(QLM_QDMI_Device_Job job,
                              QDMI_Job_Status *status) {
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  *status = job->status;

  return QDMI_SUCCESS;
}

int QLM_QDMI_device_job_wait(QLM_QDMI_Device_Job job) {
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status != QDMI_JOB_STATUS_RUNNING)
    return QDMI_ERROR_INVALIDARGUMENT;

  pthread_join(job->offload_thread, NULL);
  job->status = QDMI_JOB_STATUS_DONE;
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_job_get_results(QLM_QDMI_Device_Job job,
                                    QDMI_Job_Result result, size_t size,
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

      size_t n_qubit = strlen(key);
      size_t n_state = 1 << n_qubit;

      job->n_state = n_state;
      job->probability_dense = malloc(sizeof(double) * n_state);

      char *endptr;
      int stream_index = 0;
      memset(job->probability_dense, 0, sizeof(double) * n_state);

      while (key != NULL) {
        long index = strtol(key, &endptr, 2);
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

void QLM_QDMI_device_job_free(QLM_QDMI_Device_Job job) {
  free(job->probability_dense);
  free(job->probability_keys);
  free(job->probability_values);

  free(job->qlm_job);
  free(job->results);

  free(job);
}

int initialize_python(void) {

  char *script_location = getenv(SCRIPT_LOCATION);
  char *script_name = getenv(SCRIPT_NAME);

  Py_Initialize();

  PyObject *sysPath = PyImport_ImportModule("sys");
  PyObject *path = PyObject_GetAttrString(sysPath, "path");

  PyList_Append(path, PyUnicode_FromString(script_location));

  PyObject *pName = PyUnicode_DecodeFSDefault(script_name);
  CHECK_PYTHON_ERROR(pName)
  PyImport_Import(pName);

  *get_custom_python_module() = PyImport_Import(pName);

  CHECK_PYTHON_ERROR(*get_custom_python_module());

  return QDMI_SUCCESS;
}

int create_remote_qpu(const char *hostname, const int port) {
  //// printf("create_remote_qpu\n");
  PyObject *pFunc = PyObject_GetAttrString(*get_custom_python_module(),
                                           REMOTE_QPU_CREATE_FUNCTION_NAME);

  CHECK_PYTHON_ERROR(pFunc)

  //// printf("create_remote_qpu1 %s %d\n", hostname, port);
  PyObject *pArgs =
      PyTuple_Pack(2, PyUnicode_FromString(hostname), PyLong_FromLong(port));
  CHECK_PYTHON_ERROR(pArgs)

  //// printf("create_remote_qpu2\n");
  PyObject *pResult = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pResult)
  //// printf("create_remote_qpu3\n");
  // Py_DECREF(pFunc);

  *get_remote_qpu() = pResult;

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
  Py_DECREF(get_custom_python_module());
  Py_DECREF(get_remote_qpu());
  Py_Finalize();
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_session_alloc(QLM_QDMI_Device_Session *session) {
  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  *session =
      (QLM_QDMI_Device_Session)malloc(sizeof(QLM_QDMI_Device_Session_impl_t));
  (*session)->base_url = NULL;
  (*session)->port = PORT_INIT_VALUE;
  (*session)->status = ALLOCATED;
  return QDMI_SUCCESS;
}

int QLM_QDMI_device_session_init(QLM_QDMI_Device_Session session) {
  // // printf("QLM_QDMI_device_session_init\n");
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

  if (session->base_url == NULL || session->port == PORT_INIT_VALUE) {
    return QDMI_ERROR_BADSTATE;
  }

  int err = create_remote_qpu(session->base_url, session->port);
  CHECK_QDMI_ERROR(err)

  session->status = INITIALIZED;
  return QDMI_SUCCESS;
}

void QLM_QDMI_device_session_free(QLM_QDMI_Device_Session session) {
  if (session != NULL) {
    free(session->base_url);
    free(session);
  }
}

int QLM_QDMI_device_session_set_parameter(
    QLM_QDMI_Device_Session session, const QDMI_Device_Session_Parameter param,
    const size_t size, const void *value) {
  // // printf("%d\n", param);
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
    session->base_url = (char *)malloc(size);
    strcpy(session->base_url, (const char *)value);
  }

  if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1) {
    session->port = *(int *)value;
  }

  return QDMI_SUCCESS;
}
