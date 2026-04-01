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
#include <wmi_qdmi/device.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TOKEN_SIZE 65
#define base_url "https://badwwmi-cloudapi.wmi.badw.de"
#define DEFAULT_NUM_SHOT 0

// macros from docs
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

// return only when error
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

enum WMI_QDMI_DEVICE_SESSION_STATUS { ALLOCATED, INITIALIZED };

enum QDMI_Device_Session_Parameter {
  QDMI_DEVICE_SESSION_PARAMETER_PORT
}; // needed ?

typedef struct WMI_QDMI_Device_Session_impl_d {
  char *url;
  char *token;
  enum WMI_QDMI_DEVICE_SESSION_STATUS status;
} WMI_QDMI_Device_Session_impl_t;

typedef struct WMI_QDMI_Device_Job_impl_d {
  WMI_QDMI_Device_Session session;
  int id;
  char *id_json;
  QDMI_Job_Status status;
  size_t num_shots;

  size_t results_size; // number of states in result histogram
  char *result_hist_keys;
  size_t *result_hist_values;

  QDMI_Program_Format format;
  size_t sizebuffer;
  char *program;
} WMI_QDMI_Device_Job_impl_t;

typedef struct WMI_QDMI_Site_impl_d {
  size_t id;
} WMI_QDMI_Site_impl_t;

// three qubit dedicated device
const WMI_QDMI_Site DEVICE_SITES[] =
    { // should maybe be the implementation above
        &(WMI_QDMI_Site_impl_t){0}, &(WMI_QDMI_Site_impl_t){1},
        &(WMI_QDMI_Site_impl_t){2}};

typedef struct QDMI_Operation_impl_d {
  char *name;
  size_t qubitsnum;
  size_t parametersnum;
} WMI_QDMI_Operation_impl_t;

// not sure, these names are understood by the compiler.
const WMI_QDMI_Operation DEVICE_OPERATIONS[] = {
    (struct WMI_QDMI_Operation_impl_d *)&(WMI_QDMI_Operation_impl_t){"cz", 2,
                                                                     0},
    (struct WMI_QDMI_Operation_impl_d *)&(WMI_QDMI_Operation_impl_t){"id", 1,
                                                                     0},
    (struct WMI_QDMI_Operation_impl_d *)&(WMI_QDMI_Operation_impl_t){"x", 1, 0},
    (struct WMI_QDMI_Operation_impl_d *)&(WMI_QDMI_Operation_impl_t){"y", 1, 0},
    (struct WMI_QDMI_Operation_impl_d *)&(WMI_QDMI_Operation_impl_t){"sx", 1,
                                                                     0},
    (struct WMI_QDMI_Operation_impl_d *)&(WMI_QDMI_Operation_impl_t){"rz", 1,
                                                                     1},
    (struct WMI_QDMI_Operation_impl_d *)&(WMI_QDMI_Operation_impl_t){"mz", 2,
                                                                     0}};

// import api token
char *get_token() {

  char *token = getenv("TOKEN_WMI");

  if (strlen(token) == 0) {
    printf("   [Backend].............WMI token not set in environment.\n");
    free(token);
    return NULL;
  }

  token[TOKEN_SIZE - 1] = '\0';

  return token;
}

struct ResponseStruct {
  cJSON *json;
  size_t size;
};

// directly parsing response to json
size_t parse_json(void *contents, size_t size, size_t nmemb,
                  struct ResponseStruct *response) {
  size_t realsize = size * nmemb;

  struct ResponseStruct *mem = (struct ResponseStruct *)response;
  cJSON *ptr = cJSON_ParseWithLength(contents, realsize);

  if (!ptr) {
    /* out of memory! */
    printf(
        "   [Backend].............Not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->json = ptr;
  mem->size += realsize;

  return realsize;
}

static CURLcode send_curl_request(const char *url, const char *token,
                                  struct curl_slist *headers,
                                  const char *method, const char *payload,
                                  curl_mime *form,
                                  struct ResponseStruct *response_struct,
                                  long *http_code) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return CURLE_FAILED_INIT;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, parse_json);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_struct);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); // debugging requests

  // build token header
  int token_size_int = snprintf(NULL, 0, "access-token: %s", token) + 1;
  // if (token_size_int < 0)
  //{
  //   return NULL;
  // }

  size_t token_size = (size_t)token_size_int + 1;

  char *token_header = NULL;
  token_header = malloc(token_size);

  if (token_header != NULL) {
    snprintf(token_header, token_size, "access-token: %s", token);
  }

  headers = curl_slist_append(headers, token_header);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  // payload in the form of multipart or single part request
  if (form)
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

  if (payload)
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);

  CURLcode result = curl_easy_perform(curl);

  if (result != CURLE_OK) {
    fprintf(stderr, "[Backend].............Request problem: %s\n",
            curl_easy_strerror(result));
    return result;
  }

  if (http_code)
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);

  curl_easy_cleanup(curl);
  return result;
}

cJSON *backend_configuration() {
  char *configuration_string = "{ \
    \"backend_name\": \"dedicatedSimulator\", \
    \"backend_version\": \"1.0.0\", \
    \"n_qubits\": 5, \
    \"basis_gates\": [\"id\", \"x\", \"y\", \"sx\", \"rz\", \"cz\", \"mz\"], \
    \"coupling_map\": null, \
    \"simulator\": false, \
    \"local\": false, \
    \"conditional\": false, \
    \"open_pulse\": false, \
    \"memory\": true, \
    \"max_shots\": 65536, \
    \"gates\": []}";

  size_t len = strlen(configuration_string);

  cJSON *configuration = cJSON_ParseWithLength(configuration_string, len);

  return configuration;
}

cJSON *backend_options(size_t shots) {
  char *option_string = NULL;

  int option_len_int = snprintf(NULL, 0, "{ \"shots\": %zu}", shots);
  if (option_len_int < 0) {
    return NULL;
  }

  size_t option_len = (size_t)option_len_int + 1;

  option_string = malloc(option_len);
  if (!option_string) {
    return NULL;
  }
  snprintf(option_string, option_len, "{ \"shots\": %zu}", shots);

  cJSON *options = cJSON_ParseWithLength(option_string, option_len);

  return options;
}

/* QUERY INTERFACE STARTS*/

// For the simulator it should always be online, but for the real device, this
// needs to be filled
QDMI_Device_Status WMI_QDMI_query_device_status(void) {
  return QDMI_DEVICE_STATUS_IDLE;
}

int WMI_QDMI_device_session_query_operation_property(
    WMI_QDMI_Device_Session session, WMI_QDMI_Operation operation,
    const size_t num_sites, const WMI_QDMI_Site *sites, const size_t num_params,
    const double *params, const QDMI_Operation_Property prop, const size_t size,
    void *value, size_t *size_ret) {

  // 2q gate
  if (operation == DEVICE_OPERATIONS[0]) {

    ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME, "cz", prop, size, value,
                        size_ret)
    if (sites != NULL && num_sites != 2) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }
    ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t, 0,
                              prop, size, value, size_ret)
    if (sites == NULL) {
      ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_QUBITSNUM, size_t, 2,
                                prop, size, value, size_ret)
      return QDMI_ERROR_NOTSUPPORTED;
    }
    if (sites[0] == sites[1]) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }
  } else if (operation ==
             DEVICE_OPERATIONS[1]) { // all 1q gates have different names and
                                     // number of parameters. Assuming qubit
                                     // location is not a parameter
    ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME, "id", prop, size, value,
                        size_ret)
    ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t, 0,
                              prop, size, value, size_ret)
  } else if (operation == DEVICE_OPERATIONS[2]) {
    ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME, "x", prop, size, value,
                        size_ret)
    ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t, 0,
                              prop, size, value, size_ret)
  } else if (operation == DEVICE_OPERATIONS[3]) {
    ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME, "y", prop, size, value,
                        size_ret)
    ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t, 0,
                              prop, size, value, size_ret)
  } else if (operation == DEVICE_OPERATIONS[4]) {
    ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME, "sx", prop, size, value,
                        size_ret)
    ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t, 0,
                              prop, size, value, size_ret)
  } else if (operation == DEVICE_OPERATIONS[5]) {
    ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME, "rz", prop, size, value,
                        size_ret)
    ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t, 1,
                              prop, size, value, size_ret)
  } else if (operation == DEVICE_OPERATIONS[6]) {
    ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME, "mz", prop, size, value,
                        size_ret)
    ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t, 0,
                              prop, size, value, size_ret)
  } else {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  // common for all single qubit operations
  if ((sites != NULL && num_sites != 1) ||
      (params != NULL && num_params != 1)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_QUBITSNUM, size_t, 1, prop,
                            size, value, size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

int WMI_QDMI_device_session_query_device_property(
    WMI_QDMI_Device_Session session, const QDMI_Device_Property prop,
    const size_t size, void *value, size_t *size_ret) {
  if (prop >= QDMI_DEVICE_PROPERTY_MAX || (value == NULL && size_ret == NULL)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_NAME, "WMI", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_VERSION, "0.1.0", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "1.1.0", prop, size,
                      value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_QUBITSNUM, size_t, 3, prop,
                            size, value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_STATUS, QDMI_Device_Status,
                            WMI_QDMI_query_device_status(), prop, size, value,
                            size_ret)
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_SITES, WMI_QDMI_Site, DEVICE_SITES, 3,
                    prop, size, value, size_ret)
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_OPERATIONS, WMI_QDMI_Operation,
                    DEVICE_OPERATIONS, 7, prop, size, value, size_ret)
  // assume all-to-all connectivity
  ADD_LIST_PROPERTY(
      QDMI_DEVICE_PROPERTY_COUPLINGMAP, WMI_QDMI_Site,
      ((WMI_QDMI_Site[]){DEVICE_SITES[0], DEVICE_SITES[1], DEVICE_SITES[1],
                         DEVICE_SITES[0], DEVICE_SITES[1], DEVICE_SITES[2],
                         DEVICE_SITES[2], DEVICE_SITES[1], DEVICE_SITES[0],
                         DEVICE_SITES[2], DEVICE_SITES[2], DEVICE_SITES[0]}),
      12, prop, size, value, size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

// assume site ID is the only relevant property for now
int WMI_QDMI_device_session_query_site_property(WMI_QDMI_Device_Session session,
                                                WMI_QDMI_Site site,
                                                const QDMI_Site_Property prop,
                                                const size_t size, void *value,
                                                size_t *size_ret) {

  if (session == NULL || site == NULL || (value != NULL && size == 0) ||
      prop >= QDMI_SITE_PROPERTY_MAX) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (prop != QDMI_SITE_PROPERTY_INDEX)
    return QDMI_ERROR_NOTSUPPORTED;

  ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_INDEX, size_t, site->id, prop,
                            size, value, size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

/* QUERY INTERFACE ENDS*/

/* CONTROL INTERFACE STARTS*/

int WMI_QDMI_device_session_create_device_job(WMI_QDMI_Device_Session session,
                                              WMI_QDMI_Device_Job *job) {

  if (session == NULL || job == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (session->status != INITIALIZED) {
    return QDMI_ERROR_BADSTATE;
  }

  size_t num_qubits = 3;

  *job = (WMI_QDMI_Device_Job)malloc(sizeof(WMI_QDMI_Device_Job_impl_t));
  (*job)->session = session;
  (*job)->id = rand();
  char *job_id_json;
  size_t sz;
  sz = (size_t)snprintf(NULL, 0, "{\"job_id\": \"%i\"}", (*job)->id);
  (*job)->id_json = (char *)malloc(sz + 1);
  snprintf((*job)->id_json, sz + 1, "{\"job_id\": \"%i\"}", (*job)->id);

  (*job)->status = QDMI_JOB_STATUS_CREATED;
  (*job)->num_shots = DEFAULT_NUM_SHOT;
  (*job)->result_hist_keys = NULL;
  (*job)->result_hist_values = NULL;
  (*job)->program = NULL;

  return QDMI_SUCCESS;
}

int WMI_QDMI_device_job_set_parameter(WMI_QDMI_Device_Job job,
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
  // assume this is correct, what we had previously was a bitcode module
  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT) {
    QDMI_Program_Format format = *(const QDMI_Program_Format *)value;
    if (format != QDMI_PROGRAM_FORMAT_QIRBASESTRING)
      return QDMI_ERROR_NOTSUPPORTED;
    job->format = format;
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAM) {
    if (value != NULL) {
      job->program = malloc(size); // different in documentation
      memcpy(job->program, value, size);
      job->sizebuffer = size;
    }
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM) {
    if (value != NULL) {
      job->num_shots = *(const size_t *)value;
    }
    return QDMI_SUCCESS;
  }

  return QDMI_ERROR_NOTSUPPORTED;
}

int WMI_QDMI_device_job_submit(WMI_QDMI_Device_Job job) {

  if (job == NULL || job->status != QDMI_JOB_STATUS_CREATED ||
      job->program == NULL || job->num_shots == DEFAULT_NUM_SHOT)
    return QDMI_ERROR_INVALIDARGUMENT;

  printf("   [Backend].............Circuit received\n");

  CURL *curl = curl_easy_init();
  if (!curl)
    return QDMI_ERROR_OUTOFMEM;

  // init variables
  struct curl_slist *headers = NULL;
  curl_mime *form = NULL;
  curl_mimepart *field = NULL;
  struct ResponseStruct response;
  response.json = NULL;
  response.size = 0;
  char job_id_str[20];
  snprintf(job_id_str, sizeof(job_id_str), "%i", job->id);

  // set general options
  char url[256];
  snprintf(url, sizeof(url), "%s%s", job->session->url,
           "/1/qiskitSimulator/qir");

  // set headers
  headers = curl_slist_append(headers, "Content-Type: multipart/form-data");

  // payload
  cJSON *configuration = backend_configuration();
  char *configuration_string = cJSON_PrintUnformatted(configuration);
  cJSON *options = backend_options(job->num_shots);
  char *options_string = cJSON_PrintUnformatted(options);

  form = curl_mime_init(curl);

  field = curl_mime_addpart(form);
  curl_mime_name(field, "qir");
  curl_mime_type(field, "application/form-data");
  curl_mime_filename(field, "bitcode.bc"); // for the backend to see this as a
                                           // file and not convert it to string.
  curl_mime_data(field, job->program, job->sizebuffer);

  field = curl_mime_addpart(form);
  curl_mime_name(field, "configuration");
  curl_mime_type(field, "application/json");
  curl_mime_data(field, configuration_string, CURL_ZERO_TERMINATED);

  field = curl_mime_addpart(form);
  curl_mime_name(field, "options");
  curl_mime_type(field, "application/json");
  curl_mime_data(field, options_string, CURL_ZERO_TERMINATED);

  field = curl_mime_addpart(form);
  curl_mime_name(field, "job_id");
  curl_mime_type(field, "application/json");
  curl_mime_data(field, job_id_str, CURL_ZERO_TERMINATED);

  // exucute request
  long http_code = 0;
  CURLcode result = send_curl_request(url, job->session->token, headers, "PUT",
                                      NULL, form, &response, &http_code);

  job->status = QDMI_JOB_STATUS_SUBMITTED;

  // process data
  cJSON *message = cJSON_GetObjectItemCaseSensitive(response.json, "message");
  char *string = cJSON_Print(message);

  if (http_code != 200) {
    fprintf(stderr, "   [Backend].............Request problem: %ld - %s\n",
            http_code, string);
    return QDMI_ERROR_FATAL;
  } else {
    job->status = QDMI_JOB_STATUS_RUNNING;
  }

  free(string);
  free(configuration_string);
  free(options_string);

  cJSON_Delete(response.json);
  cJSON_Delete(configuration);
  cJSON_Delete(options);

  curl_mime_free(form);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return QDMI_SUCCESS;
}

int WMI_QDMI_device_job_cancel(WMI_QDMI_Device_Job job) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int WMI_QDMI_device_job_check(WMI_QDMI_Device_Job job,
                              QDMI_Job_Status *status) {

  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  // If the job is not created
  if (job->status < QDMI_JOB_STATUS_CREATED ||
      job->status > QDMI_JOB_STATUS_FAILED)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status == QDMI_JOB_STATUS_CREATED) {
    *status = QDMI_JOB_STATUS_CREATED;
    return QDMI_SUCCESS;
  }

  CURL *curl = curl_easy_init();

  if (!curl) {
    fprintf(stderr, "[Backend].............Curl init failed\n");
    return QDMI_ERROR_OUTOFMEM;
  }

  int err = 0;

  struct ResponseStruct response;
  response.json = NULL;
  response.size = 0;

  char url[256];
  snprintf(url, sizeof(url), "%s%s", job->session->url,
           "/1/qiskitSimulator/qobj");

  // headers
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  // exucute request
  long http_code = 0;
  CURLcode result =
      send_curl_request(url, job->session->token, headers, "POST", job->id_json,
                        NULL, &response, &http_code);

  if (http_code == 200) {
    // printf("   [Backend].............Job finished\n");

    unsigned int i;
    size_t numbits = 3;

    long bitstring_idx;
    char *bitstring_string;

    cJSON *counts_array =
        cJSON_GetObjectItemCaseSensitive(response.json, "counts");
    if (counts_array == NULL || !cJSON_IsArray(counts_array)) {
      fprintf(stderr, "   [Backend].............Invalid or missing 'counts' "
                      "array in response JSON\n");

      cJSON_Delete(response.json);
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);

      job->status = QDMI_JOB_STATUS_FAILED;
      *status = QDMI_JOB_STATUS_FAILED;
      return QDMI_ERROR_FATAL;
    }

    // IMPORTANT: assume just one circuit will be sent !!
    const cJSON *count_object = cJSON_GetArrayItem(counts_array, 0);

    job->results_size = (size_t)cJSON_GetArraySize(count_object);

    free(job->result_hist_keys);
    free(job->result_hist_values);
    job->result_hist_keys =
        malloc(job->results_size * sizeof(char) * (numbits + 1));
    job->result_hist_values = malloc(job->results_size * sizeof(size_t));

    char *result_keys_ptr = job->result_hist_keys;
    size_t *result_values_ptr = job->result_hist_values;

    cJSON *count;
    cJSON_ArrayForEach(count, count_object) {
      // keys as a long string with bitstrings separated by ","
      strncpy(result_keys_ptr, count->string, numbits);
      strcat(result_keys_ptr, ",");
      *result_values_ptr = (size_t)count->valueint;
      result_values_ptr++;
    }

    cJSON_Delete(count);

    if (result_keys_ptr[job->results_size] == ',') {
      result_keys_ptr[job->results_size] = '\0';
    }

    job->status = QDMI_JOB_STATUS_DONE;
    *status = QDMI_JOB_STATUS_DONE;
  } else if (http_code == 202) {
    // printf("   [Backend].............Job Running\n");
    job->status = QDMI_JOB_STATUS_RUNNING;
    *status = QDMI_JOB_STATUS_RUNNING;
  } else {
    cJSON *message = cJSON_GetObjectItemCaseSensitive(response.json, "message");
    char *string = cJSON_Print(message);

    fprintf(stderr, "   [Backend].............Request problem: %ld - %s\n",
            http_code, string);

    cJSON_Delete(response.json);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    job->status = QDMI_JOB_STATUS_DONE;
    *status = QDMI_JOB_STATUS_DONE;
    return QDMI_ERROR_FATAL;
  }

  cJSON_Delete(response.json);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  *status = job->status;

  return QDMI_SUCCESS;
}

int WMI_QDMI_device_job_wait(WMI_QDMI_Device_Job job, size_t timeout) {
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status > QDMI_JOB_STATUS_CANCELED)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status == QDMI_JOB_STATUS_RUNNING ||
      job->status == QDMI_JOB_STATUS_SUBMITTED) {

    QDMI_Job_Status status = QDMI_JOB_STATUS_CREATED;
    size_t time_s = 0;

    while (time_s < timeout) {
      WMI_QDMI_device_job_check(job, &status);

      if (status == QDMI_JOB_STATUS_DONE) {
        break;
      } else if (status == QDMI_JOB_STATUS_FAILED) {
        return QDMI_ERROR_FATAL;
      }
      sleep(5);
      time_s += 5;
    }
  }
  return QDMI_SUCCESS;
}

int WMI_QDMI_device_job_get_results(WMI_QDMI_Device_Job job,
                                    QDMI_Job_Result result, size_t size,
                                    void *data, size_t *size_ret) {
  if (job->status != QDMI_JOB_STATUS_DONE)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (result == QDMI_JOB_RESULT_STATEVECTOR_DENSE ||
      result == QDMI_JOB_RESULT_STATEVECTOR_SPARSE_KEYS ||
      result == QDMI_JOB_RESULT_STATEVECTOR_SPARSE_VALUES ||
      result == QDMI_JOB_RESULT_PROBABILITIES_DENSE)
    return QDMI_ERROR_NOTSUPPORTED;

  size_t required_size;
  if (result == QDMI_JOB_RESULT_HIST_KEYS) {
    required_size = strlen(job->result_hist_keys) + 1;
    if (data) {
      if (size < required_size)
        return QDMI_ERROR_INVALIDARGUMENT;
      strncpy(data, job->result_hist_keys, required_size);
      return QDMI_SUCCESS;
    }
    if (size_ret)
      *size_ret = required_size;
    return QDMI_SUCCESS;
  }

  if (result == QDMI_JOB_RESULT_HIST_VALUES) {
    required_size = job->results_size * sizeof(size_t);
    if (data) {
      if (size_ret) { // allow nullptr
        *size_ret = required_size;
      }
      memcpy(data, job->result_hist_values, required_size);
      return QDMI_SUCCESS;
    }
    if (size_ret)
      *size_ret = required_size;

    return QDMI_SUCCESS;
  }

  if (result == QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS) {
    required_size = strlen(job->result_hist_keys) + 1;
    if (data) {
      if (size < required_size)
        return QDMI_ERROR_INVALIDARGUMENT;
      strncpy(data, job->result_hist_keys, required_size);
      return QDMI_SUCCESS;
    }
    if (size_ret)
      *size_ret = required_size;
    return QDMI_SUCCESS;
  }

  if (result == QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES) {
    required_size = job->results_size * sizeof(double);
    if (data) {
      if (size_ret) { // allow nullptr
        *size_ret = required_size;
      }

      for (size_t i = 0; i < job->results_size; ++i) {
        ((double *)data)[i] =
            (double)job->result_hist_values[i] / (double)job->num_shots;
      }

      return QDMI_SUCCESS;
    }
    if (size_ret)
      *size_ret = required_size;

    return QDMI_SUCCESS;
  }

  return QDMI_SUCCESS;
}
// to my understanding this should not free session, as it is freed later.
void WMI_QDMI_device_job_free(WMI_QDMI_Device_Job job) {
  free(job->id_json);
  job->id_json = NULL;
  free(job->result_hist_keys);
  job->result_hist_keys = NULL;
  free(job->result_hist_values);
  job->result_hist_values = NULL;

  free(job->program);
  job->program = NULL;

  free(job);
  job = NULL;
}

int check_env_variable(void) {
  if (getenv("TOKEN_WMI") == NULL)
    return QDMI_ERROR_FATAL;
  return QDMI_SUCCESS;
}

int WMI_QDMI_device_initialize(void) {
  int err = check_env_variable();
  CHECK_QDMI_ERROR(err)

  return QDMI_SUCCESS;
}

int WMI_QDMI_device_finalize(void) { return QDMI_SUCCESS; }

int WMI_QDMI_device_session_alloc(WMI_QDMI_Device_Session *session) {
  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  *session =
      (WMI_QDMI_Device_Session)malloc(sizeof(WMI_QDMI_Device_Session_impl_t));
  (*session)->url = (char *)malloc(strlen(base_url) + 1);
  (*session)->token = (char *)malloc(TOKEN_SIZE);
  (*session)->status = ALLOCATED;
  return QDMI_SUCCESS;
}

int WMI_QDMI_device_session_init(WMI_QDMI_Device_Session session) {
  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  switch (WMI_QDMI_query_device_status()) {
  case QDMI_DEVICE_STATUS_ERROR:
  case QDMI_DEVICE_STATUS_OFFLINE:
  case QDMI_DEVICE_STATUS_MAINTENANCE:
    return QDMI_ERROR_FATAL;
  default:
    break;
  }

  int retval = WMI_QDMI_device_session_set_parameter(
      session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL, strlen(base_url) + 1,
      base_url);
  if (retval != QDMI_SUCCESS) {
    return retval;
  }

  char *token = get_token();
  retval = WMI_QDMI_device_session_set_parameter(
      session, QDMI_DEVICE_SESSION_PARAMETER_TOKEN, TOKEN_SIZE, token);
  if (retval != QDMI_SUCCESS) {
    free(token);
    return retval;
  }

  session->status = INITIALIZED;

  return QDMI_SUCCESS;
}

void WMI_QDMI_device_session_free(WMI_QDMI_Device_Session session) {
  if (session != NULL) {
    free(session->url);
    session->url = NULL;
    free(session->token);
    session->token = NULL;
    free(session);
    session = NULL;
  }
}

int WMI_QDMI_device_session_set_parameter(
    WMI_QDMI_Device_Session session, const QDMI_Device_Session_Parameter param,
    const size_t size, const void *value) {
  if (session == NULL || (value != NULL && size == 0) ||
      (param >= QDMI_DEVICE_SESSION_PARAMETER_MAX &&
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
    strcpy(session->url, (const char *)value);
  }

  if (param == QDMI_DEVICE_SESSION_PARAMETER_TOKEN) {
    strncpy(session->token, (const char *)value, TOKEN_SIZE);
    session->token[TOKEN_SIZE - 1] = '\0';
  }

  return QDMI_SUCCESS;
}
int WMI_QDMI_device_job_query_property(WMI_QDMI_Device_Job job,
                                       QDMI_Device_Job_Property prop,
                                       size_t size, void *value,
                                       size_t *size_ret) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}
