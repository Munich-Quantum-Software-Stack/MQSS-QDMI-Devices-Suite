#define _GNU_SOURCE
#include <string.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <Python.h>
#include <cJSON.h>
#include <curl/curl.h>

#include <qdmi/device.h>
#include <stdbool.h>
#include <string.h>

#define PRINT(a) printf("[AQT Backend]............%s\n", a)
#define AQT_TOKEN "AQT_TOKEN"
#define AQT_URL "AQT_URL"
#define SCRIPT_LOCATION "SCRIPT_LOCATION" 
#define SCRIPT_NAME "SCRIPT_NAME"
#define FUNCTION_NAME "FUNCTION_NAME"
#define WORKSPACE_ID "lrz"
#define RESOURCE_ID "marmot"

struct ResponseStruct {
  char *response;
  size_t size;
};


int QDMI_query_get_sites_dev(size_t num_entries, QDMI_Site *sites,
                             size_t *num_sites) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int QDMI_query_get_operations_dev(size_t num_entries,
                                  QDMI_Operation *operations,
                                  size_t *num_operations) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int QDMI_query_device_property_dev(QDMI_Device_Property prop, size_t size,
                                   void *value, size_t *size_ret) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int QDMI_query_site_property_dev(QDMI_Site site, QDMI_Site_Property prop,
                                 size_t size, void *value, size_t *size_ret) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int QDMI_query_operation_property_dev(QDMI_Operation operation,
                                      size_t num_sites, const QDMI_Site *sites,
                                      QDMI_Operation_Property prop, size_t size,
                                      void *value, size_t *size_ret) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}
////////////////////////// Control
int QDMI_control_create_job_dev(QDMI_Program_Format format, size_t size,
                                const void *prog, QDMI_Job *job) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int QDMI_control_set_parameter_dev(QDMI_Job job, QDMI_Job_Parameter param,
                                   size_t size, const void *value) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int QDMI_control_submit_job_dev(QDMI_Job job) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int QDMI_control_cancel_dev(QDMI_Job job) { return QDMI_ERROR_NOTIMPLEMENTED; }

int QDMI_control_check_dev(QDMI_Job job, QDMI_Job_Status *status) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int QDMI_control_wait_dev(QDMI_Job job) { return QDMI_ERROR_NOTIMPLEMENTED; }

int QDMI_control_get_data_dev(QDMI_Job job, QDMI_Job_Result result, size_t size,
                              void *data, size_t *size_ret) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}

void QDMI_control_free_job_dev(QDMI_Job job) {
  // return QDMI_ERROR_NOTIMPLEMENTED;
}

int QDMI_control_initialize_dev(void) { return QDMI_ERROR_NOTIMPLEMENTED; }

int QDMI_control_finalize_dev(void) { return QDMI_ERROR_NOTIMPLEMENTED; }













/*
const char *getUrl() { return getenv(AQT_URL); }

const char *getToken() { return getenv(AQT_TOKEN); }

const char *getTokenHeader() {
  const char *token = getToken();
  char *auth_format = "Authorization: Bearer %s";
  char *auth;
  int err = asprintf(&auth, auth_format, token);
  if (err == -1)
    return NULL;
  return auth;
}

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

const char *handlePythonError() {
  PyErr_Print();
  Py_Finalize();
  return NULL;
}

const char *getAQTJobFromPython(const char *token, const char *qir_mod,
                                size_t mod_size, int shots) {

  const char *result;
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

  PyObject *pArgs = PyTuple_Pack(3, PyUnicode_FromString(token),
                                 PyBytes_FromStringAndSize(qir_mod, mod_size),
                                 PyLong_FromLong(shots));
  if (pArgs == NULL)
    return handlePythonError();

  PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
  if (pValue == NULL)
    return handlePythonError();

  result = PyUnicode_AsUTF8(pValue);

  Py_Finalize();

  return result;
}

int QDMI_control_submit(QDMI_Device dev, QDMI_Fragment *frag, int numshots,
                        QInfo info, QDMI_Job *job) {

  PRINT("QDMI_control_submit");

  void *bitcode = (*frag)->qirmod;
  size_t size = (*frag)->sizebuffer;
  const char *request_json =
      getAQTJobFromPython(getToken(), (char *)bitcode, size, numshots);

  char *url;
  char *url_fmt = "%s/submit/%s/%s";
  int err = asprintf(&url, url_fmt, getUrl(), WORKSPACE_ID, RESOURCE_ID);
  if (err == -1)
    QDMI_ERROR_OUTOFMEM;

  struct curl_slist *headers = NULL;
  struct ResponseStruct response = {0};

  CURL *curl = curl_easy_init();

  if (!curl)
    return QDMI_ERROR_OUTOFMEM;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  headers = curl_slist_append(headers, getTokenHeader());
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json);

  CURLcode result = curl_easy_perform(curl);
  if (result != CURLE_OK)
    return QDMI_ERROR_BACKEND;

  int return_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &return_code);
  if (return_code != 200)
    return QDMI_ERROR_BACKEND;

  cJSON *response_json = cJSON_Parse(response.response);
  if (response_json == NULL)
    return QDMI_ERROR_BACKEND;

  cJSON *json_sub_response = cJSON_GetObjectItem(response_json, "job");
  if (json_sub_response == NULL)
    return QDMI_ERROR_BACKEND;

  cJSON *json_sub_response_id =
      cJSON_GetObjectItem(json_sub_response, "job_id");
  if (json_sub_response_id == NULL)
    return QDMI_ERROR_BACKEND;

  const char *job_id = cJSON_GetStringValue(json_sub_response_id);

  (*job) = malloc(sizeof(QDMI_Job_impl_t *));
  (*job)->task_id = (char *)malloc(strlen(job_id));
  strcpy((*job)->task_id, job_id);

  return QDMI_SUCCESS;
}

int QDMI_control_pack_qasm2(QDMI_Device dev, char *qasmstr,
                            QDMI_Fragment *frag) {

  return QDMI_ERROR_NOTIMPL;
}

int QDMI_control_test(QDMI_Device dev, QDMI_Job *job, int *flag,
                      QDMI_Status *status) {

  return QDMI_ERROR_NOTIMPL;
}

int QDMI_control_cancel(QDMI_Device dev, QDMI_Job *job, QInfo info) {

  return QDMI_ERROR_NOTIMPL;
}

int QDMI_control_pause(QDMI_Device dev, QDMI_Job *job, QInfo info) {

  return QDMI_ERROR_NOTIMPL;
}
int QDMI_control_wait(QDMI_Device dev, QDMI_Job *job, QDMI_Status *status) {
  PRINT("QDMI_control_wait");
  
  char *url_fmt = "%s/result/%s";
  int number_of_try = 0;
  char *response_status;

  while (true) {
    char *url;
    int err = asprintf(&url, url_fmt, getUrl(), (*job)->task_id);
    if (err == -1)
      QDMI_ERROR_OUTOFMEM;

    struct curl_slist *headers = NULL;
    struct ResponseStruct response = {0};

    CURL *curl = curl_easy_init();

    if (!curl)
      return QDMI_ERROR_OUTOFMEM;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

    headers = curl_slist_append(headers, getTokenHeader());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK)
      return QDMI_ERROR_BACKEND;

    int return_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &return_code);
    if (return_code != 200)
      return QDMI_ERROR_BACKEND;

    cJSON *json_response = cJSON_Parse(response.response);
    cJSON *json_sub_response = cJSON_GetObjectItem(json_response, "response");
    cJSON *json_status = cJSON_GetObjectItem(json_sub_response, "status");
    response_status = cJSON_GetStringValue(json_status);
    if (!strcmp(response_status, "finished")) {
      *status = QDMI_COMPLETE;
      return QDMI_SUCCESS;
    }

    if (!strcmp(response_status, "queued"))
      *status = QDMI_WAITING;
    else if (*status == QDMI_WAITING) {
      *status = QDMI_EXECUTING;
      number_of_try = -1;
    }
    number_of_try++;
    sleep(1 << number_of_try);
  }

  return QDMI_SUCCESS;
}

int QDMI_control_pack_qir(QDMI_Device dev, void *qirmod, QDMI_Fragment *frag) {
  PRINT("QDMI_control_pack_qir");
  (*frag) = (QDMI_Fragment_t *)malloc(sizeof(QDMI_Fragment_t));
  (*frag)->qirmod = qirmod;
  (*frag)->sizebuffer =
      1992; // WARNING: NEEDS TO BE UPDATED. IT IS JUST FOR THE TEST CASE!
  return QDMI_SUCCESS;
}

int QDMI_device_status(QDMI_Device dev, QInfo info, int *status) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_backend_init(QInfo info) {
  PRINT("QDMI_backend_init");

  char *url_fmt = "%s/resources/%s";
  char *url;
  int err = asprintf(&url, url_fmt, getUrl(), RESOURCE_ID);

  if (err == -1)
    QDMI_ERROR_OUTOFMEM;

  CURL *curl = curl_easy_init();

  if (!curl)
    return QDMI_ERROR_OUTOFMEM;

  struct curl_slist *headers = NULL;
  struct ResponseStruct response = {0};

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  headers = curl_slist_append(headers, getTokenHeader());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode result = curl_easy_perform(curl);
  if (result != CURLE_OK)
    return QDMI_ERROR_BACKEND;

  int return_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &return_code);

  if (return_code != 200)
    return QDMI_ERROR_BACKEND;

  cJSON *response_json = cJSON_Parse(response.response);

  cJSON *json_status = cJSON_GetObjectItem(response_json, "status");
  char *status = cJSON_GetStringValue(json_status);
  free(status);
  free(response_json);
  free(response.response);
  return QDMI_SUCCESS;
}

int QDMI_query_device_property_exists(QDMI_Device dev,
                                      QDMI_Device_property prop, int *scope) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_device_property_type(QDMI_Device dev,
                                    QDMI_Device_property prop) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_device_property_c(QDMI_Device dev, QDMI_Device_property prop,
                                 char **value) {

  if (prop == BACKEND_NAME) {
    *value = "AQT";
  }

  return QDMI_SUCCESS;
}

int QDMI_query_device_property_i(QDMI_Device dev, QDMI_Device_property prop,
                                 int *value) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_device_property_f(QDMI_Device dev, QDMI_Device_property prop,
                                 float *value) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_device_property_d(QDMI_Device dev, QDMI_Device_property prop,
                                 double *value) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_gateset_num(QDMI_Device dev, int *num_gates) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_all_gates(QDMI_Device dev, QDMI_Gate *gates) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_byname(QDMI_Device dev, char *name, QDMI_Gate *gate) {

  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_gate_name(QDMI_Device dev, QDMI_Gate gate, char *name,
                         int *len) {

  return QDMI_ERROR_NOTIMPL;
}
int QDMI_query_gate_size(QDMI_Device dev, QDMI_Gate gate, int *gatesize) {

  return QDMI_ERROR_NOTIMPL;
}

int QDMI_control_readout_size(QDMI_Device dev, QDMI_Status *status,
                              QDMI_Job job, int *numbits) {
  PRINT("QDMI_control_readout_size");

  char *url_fmt = "%s/result/%s";
  char *task_id = job->task_id;
  char *url;
  int err = asprintf(&url, url_fmt, getUrl(), task_id);

  struct curl_slist *headers = NULL;
  struct ResponseStruct response = {0};

  CURL *curl = curl_easy_init();

  if (!curl)
    return QDMI_ERROR_OUTOFMEM;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  headers = curl_slist_append(headers, getTokenHeader());
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode result = curl_easy_perform(curl);
  if (result != CURLE_OK)
    return QDMI_ERROR_BACKEND;

  int return_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &return_code);
  if (return_code != 200)
    return QDMI_ERROR_BACKEND;

  cJSON *json_response = cJSON_Parse(response.response);
  if (json_response == NULL)
    return QDMI_ERROR_BACKEND;

  cJSON *sub_response = cJSON_GetObjectItem(json_response, "response");
  if (sub_response == NULL)
    return QDMI_ERROR_BACKEND;

  cJSON *json_result = cJSON_GetObjectItem(sub_response, "result");
  if (json_result == NULL)
    return QDMI_ERROR_BACKEND;

  cJSON *json_sub_result = cJSON_GetObjectItem(json_result, "0");
  if (json_sub_result == NULL)
    return QDMI_ERROR_BACKEND;

  cJSON *single_result_json = cJSON_GetArrayItem(json_sub_result, 0);
  if (single_result_json == NULL)
    return QDMI_ERROR_BACKEND;

  *numbits = cJSON_GetArraySize(single_result_json);

  return QDMI_SUCCESS;
}

int QDMI_control_readout_raw_num(QDMI_Device dev, QDMI_Status *status,
                                 QDMI_Job job, int *num) {
  PRINT("QDMI_control_readout_raw_num");                  

  char *url_fmt = "%s/result/%s";
  char *task_id = job->task_id;
  char *url;
  int err = asprintf(&url, url_fmt, getUrl(), task_id);

  struct curl_slist *headers = NULL;
  struct ResponseStruct response = {0};

  CURL *curl = curl_easy_init();

  if (!curl)
    return QDMI_ERROR_OUTOFMEM;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  headers = curl_slist_append(headers, getTokenHeader());
  headers = curl_slist_append(headers, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode result = curl_easy_perform(curl);
  if (result != CURLE_OK)
    return QDMI_ERROR_BACKEND;

  int return_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &return_code);
  if (return_code != 200)
    return QDMI_ERROR_BACKEND;

  cJSON *json_response = cJSON_Parse(response.response);
  if (json_response == NULL)
    return QDMI_ERROR_BACKEND;

  cJSON *sub_response = cJSON_GetObjectItem(json_response, "response");
  if (sub_response == NULL)
    return QDMI_ERROR_BACKEND;

  cJSON *json_result = cJSON_GetObjectItem(sub_response, "result");
  if (json_result == NULL)
    return QDMI_ERROR_BACKEND;

  cJSON *json_sub_result = cJSON_GetObjectItem(json_result, "0");
  if (json_sub_result == NULL)
    return QDMI_ERROR_BACKEND;

  int number_of_shots = cJSON_GetArraySize(json_sub_result);

  cJSON *single_result_json = cJSON_GetArrayItem(json_sub_result, 0);
  if (single_result_json == NULL)
    return QDMI_ERROR_BACKEND;
  int nunber_of_qubit = cJSON_GetArraySize(single_result_json);

  for (int index = 0; index < number_of_shots; index++) {
    cJSON *single_result_json = cJSON_GetArrayItem(json_sub_result, index);
    if (single_result_json == NULL)
      return QDMI_ERROR_BACKEND;

    int qubit = 0;
    for (int qubit_index = 0; qubit_index < nunber_of_qubit; qubit_index++) {
      cJSON *digit_json = cJSON_GetArrayItem(single_result_json, qubit_index);
      int digit = cJSON_GetNumberValue(digit_json);
      if (digit)
        qubit += 1 << (nunber_of_qubit - (qubit_index + 1));
    }

    num[qubit] += 1;
  }

  return QDMI_SUCCESS;
}

int QDMI_control_readout_raw_sample(QDMI_Device dev, QDMI_Status *status,
                                    int numraw, QInfo info, long *hist) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_all_qubits(QDMI_Device dev, QDMI_Qubit *qubits) {

  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_qubits_num(QDMI_Device dev, int *num_qubits) {

  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_qubit_property_exists(QDMI_Device dev, QDMI_Qubit qubit,
                                     QDMI_Qubit_property prop, int *scope) {
  return QDMI_ERROR_NOTIMPL;
}
int QDMI_query_qubit_property_type(QDMI_Device dev, QDMI_Qubit qubit,
                                   QDMI_Qubit_property prop) {
  return QDMI_ERROR_NOTIMPL;
}
int QDMI_query_qubit_property_c(QDMI_Device dev, QDMI_Qubit qubit,
                                QDMI_Qubit_property prop, char *value) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_qubit_property_i(QDMI_Device dev, QDMI_Qubit qubit,
                                QDMI_Qubit_property prop, int *value) {

  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_qubit_property_f(QDMI_Device dev, QDMI_Qubit qubit,
                                QDMI_Qubit_property prop, float *value) {

  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_qubit_property_d(QDMI_Device dev, QDMI_Qubit qubit,
                                QDMI_Qubit_property prop, double *value) {
  return QDMI_ERROR_NOTIMPL;
}
int QDMI_control_extract_state(QDMI_Device dev, QDMI_Status status,
                               int *state) {
  return QDMI_ERROR_NOTIMPL;
}
int QDMI_control_readout_hist_size(QDMI_Device dev, QDMI_Status *status,
                                   QDMI_Job job, int *size) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_control_readout_hist_top(QDMI_Device dev, QDMI_Status *status,
                                  QDMI_Job job, int numhist, QInfo info,
                                  long *hist) {

  return QDMI_ERROR_NOTIMPL;
}
int QDMI_query_gate_unitary(QDMI_Device dev, QDMI_Gate gate,
                            QDMI_Unitary *unitary) {
  return QDMI_ERROR_NOTIMPL;
}
int QDMI_query_gate_property_exists(QDMI_Device dev, QDMI_Gate gate,
                                    QDMI_Gate_property prop, int *scope) {
  return QDMI_ERROR_NOTIMPL;
}
int QDMI_query_gate_property_type(QDMI_Device dev, QDMI_Gate gate,
                                  QDMI_Gate_property prop) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_gate_property_i(QDMI_Device dev, QDMI_Gate_property prop,
                               QDMI_Gate gate, int *coor, int *value) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_query_gate_property_f(QDMI_Device dev, QDMI_Gate_property prop,
                               QDMI_Gate gate, int *coor, float *value) {
  return QDMI_ERROR_NOTIMPL;
}
int QDMI_query_gate_property_d(QDMI_Device dev, QDMI_Gate_property prop,
                               QDMI_Gate gate, int *coor, double *value) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_device_quality_check(QDMI_Device dev, double *result) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_device_quality_limit(QDMI_Device dev, double *result) {
  return QDMI_ERROR_NOTIMPL;
}

int QDMI_device_quality_calibrate(QDMI_Device dev) {
  return QDMI_ERROR_NOTIMPL;
}

*/