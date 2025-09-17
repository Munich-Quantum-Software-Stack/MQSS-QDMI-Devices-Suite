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
#define _GNU_SOURCE
#include "lrz_qdmi/device.h"
#include "lrz_qdmi/types.h"
#include "qdmi/constants.h"

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LRZ_DEVICE_HOST getenv("LRZ_DEVICE_HOST")

#define HTTP_OK 200
#define HTTP_FORBIDDEN 403

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

#define NUM_OF_HW 8

#define NUM_OF_OP 4
const char *DEVICE_HARDWARES[NUM_OF_HW] = {
    "AQT20", "MAQCS", "MUNIQC-Atoms20", "Q20", "Q5", "QExa20", "WMI3", "QLM"};

static QDMI_Device_Status *LRZ_QDMI_get_device_status(void) {
  static QDMI_Device_Status device_status = QDMI_DEVICE_STATUS_OFFLINE;
  return &device_status;
}
QDMI_Device_Status LRZ_QDMI_read_device_status(void) {
  return *LRZ_QDMI_get_device_status();
}
void LRZ_QDMI_set_device_status(QDMI_Device_Status status) {
  *LRZ_QDMI_get_device_status() = status;
}
struct ResponseStruct {
  char *response;
  size_t size;
};

static size_t WriteCallback(char *data, size_t size, size_t nmemb,
                            void *clientp) {
  size_t realsize = size * nmemb;
  struct ResponseStruct *mem = (struct ResponseStruct *)clientp;
  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if (!ptr)
    return 0;

  mem->response = ptr;
  void *dest = memcpy(&(mem->response[mem->size]), data, realsize);
  if (dest == NULL)
    fprintf(stderr, "Memory error");
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

enum LRZ_QDMI_DEVICE_SESSION_STATUS {
  /// The session is allocated but not yet initialized
  ALLOCATED,
  /// The session is successfully initialized
  INITIALIZED
};

typedef struct LRZ_Hardware_impl_d {
  char *name;

  size_t n_qubit;

  LRZ_QDMI_Site *sites;

  LRZ_QDMI_Site *coupling_map;

  LRZ_QDMI_Operation operations[NUM_OF_OP];

  size_t n_op;

  size_t coupling_map_size;
} LRZ_Hardware;

/**
 * @brief Implementation of the LRZ_QDMI_Device_Session structure.
 * @details This structure can, e.g., be used to store a token to access an API.
 */
typedef struct LRZ_QDMI_Device_Session_impl_d {

  /// The url of the Munich Quantum Portam
  char *base_url;

  /// The token for authentication.
  char *token;

  /// The selected device for the sessio;

  LRZ_Hardware *hardware;

  /// Current status of the session
  enum LRZ_QDMI_DEVICE_SESSION_STATUS status;

} LRZ_QDMI_Device_Session_impl_t;

/**
 * @brief Implementation of the LRZ_QDMI_Device_Job structure.
 * @details This structure can, e.g., be used to store the job id.
 */
struct LRZ_QDMI_Device_Job_impl_d {
  char *circuit;
  QDMI_Program_Format *circuit_format;
  char *hw;
  size_t *shots;
  QDMI_Job_Status status;
  LRZ_QDMI_Device_Session session;
  char *uuid;

  char *raw_results;
  char *hist_keys;

  int *hist_values;
  double *prob_values;
  double *prob_dense;
  int n_state;

  size_t hist_size;
  size_t prob_value_size;
  size_t prob_dense_size;
};

/**
 * @brief Implementation of the LRZ_QDMI_Site structure.
 * @details This structure can, e.g., be used to store the site id.
 */
struct LRZ_QDMI_Site_impl_d {
  size_t index;
};

/**
 * @brief Implementation of the LRZ_QDMI_Operation structure.
 * @details This structure can, e.g., be used to store the operation id.
 */
typedef struct LRZ_QDMI_Operation_impl_d {
  char *name;
  size_t n_qubit;
  size_t n_param;

} LRZ_QDMI_Operation_impl_t;

LRZ_QDMI_Operation_impl_t meassure = {"measure", 1, 0};
LRZ_QDMI_Operation_impl_t id = {"id", 1, 0};
LRZ_QDMI_Operation_impl_t r = {"r", 1, 3};
LRZ_QDMI_Operation_impl_t cz = {"cz", 2, 0};

LRZ_QDMI_Operation OPERATIONS[NUM_OF_OP] = {&meassure, &id, &r, &cz};

int LRZ_QDMI_device_initialize(void) {
  CURL *curl;
  CURLcode res;
  long response_code;

  struct ResponseStruct response = {0};
  curl = curl_easy_init();
  if (!curl)
    return QDMI_ERROR_FATAL;

  curl_easy_setopt(curl, CURLOPT_URL,
                   "https://portal.quantum.lrz.de:4000/v1");
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  res = curl_easy_perform(curl);

  if (res != CURLE_OK)
    return QDMI_ERROR_FATAL;

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

  if (response_code != 200 && response_code != 403)
    return QDMI_ERROR_FATAL;

  curl_easy_cleanup(curl);

  LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);

  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_finalize(void) {
  LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_OFFLINE);
  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_session_alloc(LRZ_QDMI_Device_Session *session) {
  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  *session =
      (LRZ_QDMI_Device_Session)malloc(sizeof(LRZ_QDMI_Device_Session_impl_t));
  (*session)->base_url = NULL;
  (*session)->token = NULL;
  (*session)->status = ALLOCATED;
  (*session)->hardware = (LRZ_Hardware *)malloc(sizeof(LRZ_Hardware));

  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_session_init(LRZ_QDMI_Device_Session session) {

  switch (LRZ_QDMI_read_device_status()) {
  case QDMI_DEVICE_STATUS_ERROR:
  case QDMI_DEVICE_STATUS_OFFLINE:
  case QDMI_DEVICE_STATUS_MAINTENANCE:
    return QDMI_ERROR_FATAL;
  default:
    break;
  }

  if (session == NULL || session->token == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  char *base_url =
      session->base_url == NULL ? LRZ_DEVICE_HOST : session->base_url;

  char *url;
  if (session->hardware != NULL)
    asprintf(&url, "%s/resources/%s", base_url, session->hardware->name);
  else
    asprintf(&url, "%s/resources", base_url);

  CURL *curl = curl_easy_init();
  if (!curl)
    return QDMI_ERROR_FATAL;

  struct ResponseStruct response = {0};
  struct curl_slist *headers = NULL;
  char *auth_header;
  CURLcode res;
  long response_code;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  asprintf(&auth_header, "Authorization: Bearer %s", session->token);
  headers = curl_slist_append(headers, auth_header);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
    return QDMI_ERROR_BADSTATE;

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  curl_slist_free_all(headers);
  if (response_code == HTTP_FORBIDDEN)
    return QDMI_ERROR_PERMISSIONDENIED;

  if (response_code != HTTP_OK || response.response == NULL)
    return QDMI_ERROR_FATAL;

  cJSON *response_json = cJSON_Parse(response.response);
  char *string = cJSON_Print(response_json);
  if (session->hardware == NULL) {
    for (int i = 0; i < NUM_OF_HW; i++) {
      cJSON *hw = cJSON_GetObjectItem(response_json, DEVICE_HARDWARES[i]);
      if (hw != NULL) {
        cJSON *hw_status = cJSON_GetObjectItem(hw, "online");
        if (cJSON_IsTrue(hw_status)) {
          session->status = INITIALIZED;
          return QDMI_SUCCESS;
        }
      }
    }
  } else {
    cJSON *hw_status = cJSON_GetObjectItem(response_json, "online");
    if (cJSON_IsFalse(hw_status)) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }
    cJSON *qubit_num = cJSON_GetObjectItem(response_json, "qubits");
    if (!qubit_num) {
      return QDMI_ERROR_FATAL;
    }

    size_t _n_qubit = (size_t)cJSON_GetNumberValue(qubit_num);
    session->hardware->n_qubit = _n_qubit;
    session->hardware->sites = malloc(sizeof(LRZ_QDMI_Site) * _n_qubit);
    for (size_t i = 0; i < session->hardware->n_qubit; i++) {
      session->hardware->sites[i] = malloc(sizeof(LRZ_QDMI_Site));
      session->hardware->sites[i]->index = i;
    }

    cJSON *coupling_maps = cJSON_GetObjectItem(response_json, "connectivity");
    if (!coupling_maps) {
      return QDMI_ERROR_FATAL;
    }
    char *coupling_map = cJSON_GetStringValue(coupling_maps);

    const char *p = &(*++coupling_map);
    size_t index = 0;
    session->hardware->coupling_map = malloc(sizeof(LRZ_QDMI_Site) * strlen(p));
    const char coupling_map_pair_format[] = "[%d, %d]";
    while (*p) {
      int a, b;
      if (sscanf(p, coupling_map_pair_format, &a, &b) == 2) {
        session->hardware->coupling_map[index++] = session->hardware->sites[a];
        session->hardware->coupling_map[index++] = session->hardware->sites[b];
        p += strlen(coupling_map_pair_format) - 1;
      }
      *p++;
    }
    session->hardware->coupling_map_size = index;
    cJSON *operations = cJSON_GetObjectItem(response_json, "instructions");
    char *operations_s = cJSON_GetStringValue(operations);
    char *ops = operations_s;
    size_t start_index = 0, end_index = 0;
    size_t count = 0;
    size_t num_op = 0;
    while (*ops) {
      char *name;
      if (*ops == '\'') {
        ops++;
        count++;
        start_index = count;
        while (*ops != '\'') {
          count++;
          ops++;
        }
        end_index = count;
        name = malloc(sizeof(end_index - start_index));
        strncpy(name, operations_s + start_index, end_index - start_index);
        name[end_index - start_index] = '\0';
        for (int i = 0; i < NUM_OF_OP; i++) {
          if (!strcmp(OPERATIONS[i]->name, name)) {
            session->hardware->operations[num_op] =
                malloc(sizeof(LRZ_QDMI_Operation_impl_t));
            session->hardware->operations[num_op]->name =
                malloc(strlen(name) + 1);
            strcpy(session->hardware->operations[num_op]->name,
                   OPERATIONS[i]->name);
            session->hardware->operations[num_op]->n_param =
                OPERATIONS[i]->n_param;
            session->hardware->operations[num_op]->n_qubit =
                OPERATIONS[i]->n_qubit;
            num_op++;
          }
        }
      }
      count++;
      ops++;
    }

    session->hardware->n_op = num_op;
    session->status = INITIALIZED;
    return QDMI_SUCCESS;
  }

  return QDMI_ERROR_FATAL;
}

void LRZ_QDMI_device_session_free(LRZ_QDMI_Device_Session session) {
  if (!session) {
    return;
  }
  if (session->hardware != NULL) {
    LRZ_Hardware *hardware = session->hardware;
    if (hardware->coupling_map == NULL)
      free(session->hardware->coupling_map);

    if (hardware->name != NULL) {
      free(hardware->name);
    }
    if (hardware->sites != NULL) {
      free(hardware->sites);
    }
    free(hardware);
  }

  if (session->base_url != NULL) {
    free(session->base_url);
  }

  if (session->token != NULL) {
    free(session->token);
  }

  free(session);
}

int LRZ_QDMI_device_session_set_parameter(LRZ_QDMI_Device_Session session,
                                          QDMI_Device_Session_Parameter param,
                                          const size_t size,
                                          const void *value) {
  if (session == NULL || (value != NULL && size == 0) ||
      (param >= QDMI_DEVICE_SESSION_PARAMETER_MAX &&
       param != QDMI_DEVICE_SESSION_PARAMETER_TOKEN &&
       param != QDMI_DEVICE_SESSION_PARAMETER_BASEURL &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (session->status != ALLOCATED) {
    return QDMI_ERROR_BADSTATE;
  }

  switch (param) {

  case QDMI_DEVICE_SESSION_PARAMETER_BASEURL:
    session->base_url = (char *)malloc(size);
    strcpy(session->base_url, (const char *)value);
    return QDMI_SUCCESS;
  case QDMI_DEVICE_SESSION_PARAMETER_TOKEN:
    session->token = (char *)malloc(size);
    strcpy(session->token, (const char *)value);
    return QDMI_SUCCESS;
  case QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1:
    session->hardware->name = (char *)malloc(size);
    strcpy(session->hardware->name, (const char *)value);
    return QDMI_SUCCESS;
  default:
    break;
  }

  return QDMI_ERROR_NOTIMPLEMENTED;
}

int LRZ_QDMI_device_session_create_device_job(LRZ_QDMI_Device_Session session,
                                              LRZ_QDMI_Device_Job *job) {
  if (session == NULL || job == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  *job = (LRZ_QDMI_Device_Job)malloc(sizeof(struct LRZ_QDMI_Device_Job_impl_d));
  (*job)->circuit = NULL;
  (*job)->circuit_format = NULL;
  (*job)->shots = NULL;
  (*job)->session = session;
  (*job)->hw = malloc(strlen(session->hardware->name) + 1);
  strcpy((*job)->hw, session->hardware->name);

  (*job)->status = QDMI_JOB_STATUS_CREATED;
  (*job)->raw_results = NULL;
  (*job)->uuid = NULL;

  return QDMI_SUCCESS;
}

void LRZ_QDMI_device_job_free(LRZ_QDMI_Device_Job job) {
  if (job == NULL)
    return;

  if (job->circuit != NULL)
    free(job->circuit);

  if (job->circuit_format != NULL)
    free(job->circuit_format);

  if (job->hw != NULL)
    free(job->hw);

  if (job->shots != NULL)
    free(job->shots);

  if (job->uuid != NULL)
    free(job->uuid);

  if (job->raw_results != NULL) {

    free(job->raw_results);

    if (job->hist_keys != NULL)
      free(job->hist_keys);

    if (job->hist_values != NULL)
      free(job->hist_values);

    if (job->prob_values != NULL)
      free(job->prob_values);
  }

  free(job);
  job = NULL;
}

int LRZ_QDMI_device_job_set_parameter(LRZ_QDMI_Device_Job job,
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
    if (format != QDMI_PROGRAM_FORMAT_QASM2 &&
        format != QDMI_PROGRAM_FORMAT_QASM3)
      return QDMI_ERROR_NOTSUPPORTED;
    job->circuit_format = malloc(sizeof(QDMI_Program_Format));
    memcpy(job->circuit_format, (const QDMI_Program_Format *)value,
           sizeof(QDMI_Program_Format));
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAM) {
    job->circuit = malloc(size);
    strcpy(job->circuit, (const char *)value);
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM) {
    size_t shots = *(const size_t *)value;
    job->shots = malloc(sizeof(size_t));
    memcpy(job->shots, (size_t *)value, sizeof(size_t));
    return QDMI_SUCCESS;
  }

  return QDMI_ERROR_NOTSUPPORTED;
}

int LRZ_QDMI_device_job_query_property(LRZ_QDMI_Device_Job job,
                                       const QDMI_Device_Job_Property prop,
                                       const size_t size, void *value,
                                       size_t *size_ret) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int LRZ_QDMI_device_job_submit(LRZ_QDMI_Device_Job job) {
  if (job == NULL || job->circuit == NULL || job->circuit_format == NULL ||
      job->shots == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status != QDMI_JOB_STATUS_CREATED)
    return QDMI_ERROR_BADSTATE;

  char *circuit_format =
      *(job->circuit_format) == QDMI_PROGRAM_FORMAT_QASM2 ? "qasm" : "qasm3";

  cJSON *job_json = cJSON_CreateObject();

  cJSON_AddStringToObject(job_json, "circuit", job->circuit);
  cJSON_AddStringToObject(job_json, "circuit_format", circuit_format);
  cJSON_AddStringToObject(job_json, "resource_name", job->hw);
  cJSON_AddNumberToObject(job_json, "shots", (double)*(job->shots));
  cJSON_AddBoolToObject(job_json, "no_modify", 0);
  cJSON_AddBoolToObject(job_json, "queued", 0);

  CURLcode res;
  CURL *curl = curl_easy_init();
  if (!curl)
    return QDMI_ERROR_FATAL;

  char *url, *auth_header;
  struct curl_slist *headers = NULL;
  struct ResponseStruct response = {0};
  asprintf(&auth_header, "Authorization: Bearer %s", job->session->token);
  asprintf(&url, "%s/job", job->session->base_url);

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cJSON_Print(job_json));

  headers = curl_slist_append(headers, auth_header);
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  res = curl_easy_perform(curl);

  if (res != CURLE_OK)
    return QDMI_ERROR_FATAL;
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  cJSON *json_response = cJSON_Parse(response.response);

  if (!json_response)
    return QDMI_ERROR_FATAL;

  job->uuid = cJSON_GetStringValue(cJSON_GetObjectItem(json_response, "uuid"));
  job->status = QDMI_JOB_STATUS_SUBMITTED;
  LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);
  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_job_cancel(LRZ_QDMI_Device_Job job) {
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;
  QDMI_Job_Status status;
  LRZ_QDMI_device_job_check(job, &status);

  if (status != QDMI_JOB_STATUS_RUNNING &&
      status != QDMI_JOB_STATUS_SUBMITTED && status != QDMI_JOB_STATUS_CREATED)
    return QDMI_ERROR_INVALIDARGUMENT;

  char *base_url =
      job->session->base_url == NULL ? LRZ_DEVICE_HOST : job->session->base_url;

  char *url;
  asprintf(&url, "%s/job/%s", base_url, job->uuid);

  CURL *curl = curl_easy_init();
  if (!curl)
    return QDMI_ERROR_FATAL;

  struct ResponseStruct response = {0};
  struct curl_slist *headers = NULL;
  char *auth_header;
  CURLcode res;
  long response_code;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

  asprintf(&auth_header, "Authorization: Bearer %s", job->session->token);
  headers = curl_slist_append(headers, auth_header);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
    return QDMI_ERROR_BADSTATE;

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  curl_slist_free_all(headers);

  if (response_code == HTTP_FORBIDDEN)
    return QDMI_ERROR_PERMISSIONDENIED;

  if (response_code != HTTP_OK || response.response == NULL)
    return QDMI_ERROR_FATAL;

  if (strcmp(response.response, "OK"))
    return QDMI_ERROR_FATAL;

  LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);

  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_job_check(LRZ_QDMI_Device_Job job,
                              QDMI_Job_Status *status) {
  if (job == NULL || status == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status == QDMI_JOB_STATUS_CREATED) {
    *status = job->status;
    return QDMI_SUCCESS;
  }

  char *base_url =
      job->session->base_url == NULL ? LRZ_DEVICE_HOST : job->session->base_url;

  char *url;
  asprintf(&url, "%s/job/%s", base_url, job->uuid);

  CURL *curl = curl_easy_init();
  if (!curl)
    return QDMI_ERROR_FATAL;

  struct ResponseStruct response = {0};
  struct curl_slist *headers = NULL;
  char *auth_header;
  CURLcode res;
  long response_code;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  asprintf(&auth_header, "Authorization: Bearer %s", job->session->token);
  headers = curl_slist_append(headers, auth_header);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  res = curl_easy_perform(curl);

  if (res != CURLE_OK)
    return QDMI_ERROR_BADSTATE;

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  curl_slist_free_all(headers);

  if (response_code == HTTP_FORBIDDEN)
    return QDMI_ERROR_PERMISSIONDENIED;

  if (response_code != HTTP_OK || response.response == NULL)
    return QDMI_ERROR_FATAL;

  cJSON *response_json = cJSON_Parse(response.response);
  if (!response_json)
    return QDMI_ERROR_FATAL;

  cJSON *response_status = cJSON_GetObjectItem(response_json, "status");
  if (!response_status)
    return QDMI_ERROR_FATAL;

  char *_response = cJSON_GetStringValue(response_status);
  if (!strcmp(_response, "PENDING")) {
    job->status = QDMI_JOB_STATUS_SUBMITTED;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);
  } else if (!strcmp(_response, "WAITING")) {
    job->status = QDMI_JOB_STATUS_QUEUED;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);
  } else if (!strcmp(_response, "FAILED")) {
    job->status = QDMI_JOB_STATUS_FAILED;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  } else if (!strcmp(_response, "COMPLETED")) {
    job->status = QDMI_JOB_STATUS_DONE;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  } else if (!strcmp(_response, "CANCELLED")) {
    job->status = QDMI_JOB_STATUS_CANCELED;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  } else {
    return QDMI_ERROR_FATAL;
  }
  *status = job->status;
  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_job_wait(LRZ_QDMI_Device_Job job, const size_t timeout) {
  struct timeval start, end;
  gettimeofday(&start, NULL);
  QDMI_Job_Status status;
  if (job == NULL || job->status == QDMI_JOB_STATUS_CREATED)
    return QDMI_ERROR_INVALIDARGUMENT;

  while (1) {
    if (LRZ_QDMI_device_job_check(job, &status) == QDMI_SUCCESS) {
      if (status == QDMI_JOB_STATUS_DONE ||
          status == QDMI_JOB_STATUS_CANCELED ||
          status == QDMI_JOB_STATUS_FAILED) {
        return QDMI_SUCCESS;
      }
    }
    gettimeofday(&end, NULL);
    if (timeout != 0 && (size_t)(end.tv_sec - start.tv_sec) > timeout) {
      return QDMI_ERROR_TIMEOUT;
    }
    usleep(500);
  }

  return QDMI_ERROR_FATAL;
}

int fetch_results(LRZ_QDMI_Device_Job job) {

  char *base_url =
      job->session->base_url == NULL ? LRZ_DEVICE_HOST : job->session->base_url;

  char *url;
  asprintf(&url, "%s/job/%s/result", base_url, job->uuid);

  CURL *curl = curl_easy_init();
  if (!curl)
    return QDMI_ERROR_FATAL;

  struct ResponseStruct response = {0};
  struct curl_slist *headers = NULL;
  char *auth_header;
  CURLcode res;
  long response_code;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  asprintf(&auth_header, "Authorization: Bearer %s", job->session->token);
  headers = curl_slist_append(headers, auth_header);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
    return QDMI_ERROR_BADSTATE;

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  curl_slist_free_all(headers);

  if (response_code == HTTP_FORBIDDEN)
    return QDMI_ERROR_PERMISSIONDENIED;

  if (response_code != HTTP_OK || response.response == NULL)
    return QDMI_ERROR_FATAL;
  job->raw_results = response.response;
  char *hist_keys = "";
  cJSON *parsed = cJSON_Parse(response.response);
  cJSON *results = cJSON_GetObjectItem(parsed, "result");
  char *_results = results->valuestring;
  if (*_results == '[') {
    _results[strlen(_results) - 1] = '\0';
    _results++;
  }

  cJSON *parsed_results = cJSON_Parse(_results);
  if (parsed_results == NULL) {
    return QDMI_ERROR_FATAL;
  }

  size_t array_size = (size_t)cJSON_GetArraySize(parsed_results);
  char *f_item = cJSON_GetArrayItem(parsed_results, 0)->string;
  job->prob_dense_size = (1 << strlen(f_item)) * sizeof(double);
  job->hist_size = array_size;
  job->prob_value_size = sizeof(double) * array_size;
  size_t size_in_byte = (size_t)array_size * sizeof(double);

  job->prob_values = malloc(sizeof(double) * array_size);
  job->hist_values = malloc(sizeof(int) * array_size);
  job->prob_dense = malloc(job->prob_dense_size);
  job->n_state = 1 << strlen(f_item);
  size_t shots = *(job->shots);
  for (size_t index = 0; index < array_size; index++) {
    cJSON *value_json = cJSON_GetArrayItem(parsed_results, (int)index);
    int value = (int)cJSON_GetNumberValue(value_json);
    double prob = (double)value / (double)shots;
    char *key = value_json->string;
    long key_int = strtol(key, NULL, 2);

    job->prob_values[index] = prob;
    job->hist_values[index] = value;

    job->prob_dense[key_int] = prob;
    asprintf(&hist_keys, "%s,%s", hist_keys, key);
  }
  job->hist_keys = malloc(strlen(hist_keys) - 1);
  strcpy(job->hist_keys, ++hist_keys);
  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_job_get_results(LRZ_QDMI_Device_Job job,
                                    QDMI_Job_Result result, const size_t size,
                                    void *data, size_t *size_ret) {

  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  QDMI_Job_Status status;
  if ((LRZ_QDMI_device_job_check(job, &status) == QDMI_SUCCESS &&
       status != QDMI_JOB_STATUS_DONE))
    return QDMI_ERROR_INVALIDARGUMENT;

  if (result != QDMI_JOB_RESULT_HIST_KEYS &&
      result != QDMI_JOB_RESULT_HIST_VALUES &&
      result != QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS &&
      result != QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES &&
      result != QDMI_JOB_RESULT_PROBABILITIES_DENSE)
    return QDMI_ERROR_NOTSUPPORTED;

  if (job->raw_results == NULL) {
    int err = fetch_results(job);
    if (err) {
      return err;
    }
  }

  size_t required_size;
  ADD_STRING_PROPERTY(QDMI_JOB_RESULT_HIST_KEYS, job->hist_keys, result, size,
                      data, size_ret)
  ADD_LIST_PROPERTY(QDMI_JOB_RESULT_HIST_VALUES, int, job->hist_values,
                    job->hist_size, result, size, data,
                    size_ret)

  ADD_STRING_PROPERTY(QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS, job->hist_keys,
                      result, size, data, size_ret)
  ADD_LIST_PROPERTY(QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES, int,
                    job->prob_values, job->prob_value_size / sizeof(double),
                    result, size, data, size_ret)

  if (result == QDMI_JOB_RESULT_PROBABILITIES_DENSE) {
    required_size = job->prob_dense_size;
    if (data) {
      if (size < required_size)
        return QDMI_ERROR_INVALIDARGUMENT;
      memcpy(data, job->prob_dense, size);
    }
    if (size_ret)
      *size_ret = required_size;
    return QDMI_SUCCESS;
  }

  return QDMI_ERROR_NOTSUPPORTED;
}

int LRZ_QDMI_device_session_query_device_property(
    LRZ_QDMI_Device_Session session, const QDMI_Device_Property prop,
    const size_t size, void *value, size_t *size_ret) {
  if ((prop >= QDMI_DEVICE_PROPERTY_MAX &&
       prop != QDMI_DEVICE_PROPERTY_CUSTOM1) ||
      (value == NULL && size_ret == NULL)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_NAME, "LRZ", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_VERSION, "0.0.1", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "0.0.1", prop, size,
                      value, size_ret)

  if (prop == QDMI_DEVICE_PROPERTY_CUSTOM1) {
    if (value == NULL) {
      size_t hw_size = 0;
      for (int i = 0; i < NUM_OF_HW; i++) {
        hw_size += (strlen(DEVICE_HARDWARES[i]) + 1);
      }
      *size_ret = hw_size;
      return QDMI_SUCCESS;
    }
    for (int i = 0; i < NUM_OF_HW; i++) {
      if (i != 0)
        strcat(value, ";");
      strcat(value, DEVICE_HARDWARES[i]);
    }
    return QDMI_SUCCESS;
  }

  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_STATUS, QDMI_Device_Status,
                            LRZ_QDMI_read_device_status(), prop, size, value,
                            size_ret)

  if (session == NULL || session->hardware == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_QUBITSNUM, size_t,
                            session->hardware->n_qubit, prop, size, value,
                            size_ret)

  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_SITES, LRZ_QDMI_Site,
                    session->hardware->sites, session->hardware->n_qubit, prop,
                    size, value, size_ret);

  if (session->hardware->n_op == 0) {
    return QDMI_ERROR_NOTSUPPORTED;
  }
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_OPERATIONS, LRZ_QDMI_Operation,
                    session->hardware->operations, session->hardware->n_op,
                    prop, size, value, size_ret);

  return QDMI_ERROR_NOTSUPPORTED;
}

int LRZ_QDMI_device_session_query_site_property(LRZ_QDMI_Device_Session session,
                                                LRZ_QDMI_Site site,
                                                const QDMI_Site_Property prop,
                                                const size_t size, void *value,
                                                size_t *size_ret) {
  if (session == NULL || site == NULL || (value != NULL && size == 0) ||
      prop >= QDMI_SITE_PROPERTY_MAX) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_INDEX, size_t, site->index, prop,
                            size, value, size_ret)
  return QDMI_ERROR_NOTSUPPORTED;
}

int LRZ_QDMI_device_session_query_operation_property(
    LRZ_QDMI_Device_Session session, LRZ_QDMI_Operation operation,
    const size_t num_sites, const LRZ_QDMI_Site *sites, const size_t num_params,
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

  ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME, operation->name, prop, size,
                      value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_QUBITSNUM, size_t,
                            operation->n_qubit, prop, size, value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t,
                            operation->n_param, prop, size, value, size_ret)

  if (!strcmp(OPERATIONS[3]->name, operation->name)) {
    ADD_LIST_PROPERTY(QDMI_OPERATION_PROPERTY_SITES, LRZ_QDMI_Site,
                      session->hardware->coupling_map,
                      session->hardware->coupling_map_size, prop, size, value,
                      size_ret);

  } else {
    ADD_LIST_PROPERTY(QDMI_OPERATION_PROPERTY_SITES, LRZ_QDMI_Site,
                      session->hardware->sites, session->hardware->n_qubit,
                      prop, size, value, size_ret);
  }

  return QDMI_ERROR_NOTSUPPORTED;
}
