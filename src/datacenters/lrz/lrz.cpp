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

#include <lrz_qdmi/device.h>
#include <mqss/client.h>

#include "job.h"
#include "lrz_qdmi/types.h"
#include "qdmi/constants.h"
#include "session.h"

using namespace mqss::client;

#define ADD_SINGLE_VALUE_PROPERTY(prop_name, prop_type, prop_value, prop,      \
                                  size, value, size_ret)                       \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < sizeof(prop_type)) {                                      \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        *static_cast<prop_type *>(value) = prop_value;                         \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = sizeof(prop_type);                                         \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

#define ADD_STRING_PROPERTY(prop_name, prop_value, prop, size, value,          \
                            size_ret)                                          \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < strlen(prop_value) + 1) {                                 \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        strncpy(static_cast<char *>(value), prop_value, size);                 \
        static_cast<char *>(value)[size - 1] = '\0';                           \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = strlen(prop_value) + 1;                                    \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

#define ADD_LIST_PROPERTY(prop_name, prop_type, prop_values, prop, size,       \
                          value, size_ret)                                     \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < (prop_values).size() * sizeof(prop_type)) {               \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
                                                                               \
        if constexpr (std::is_trivially_copyable_v<prop_type>) {               \
          std::memcpy(value, (prop_values).data(),                             \
                      (prop_values).size() * sizeof(prop_type));               \
        } else {                                                               \
          std::copy((prop_values).begin(), (prop_values).end(),                \
                    static_cast<prop_type *>(value));                          \
        }                                                                      \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = (prop_values).size() * sizeof(prop_type);                  \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  } /// [DOXYGEN MACRO END]

constexpr std::array SUPPORTED_PROGRAM_FORMATS = {QDMI_PROGRAM_FORMAT_QASM2,
                                                  QDMI_PROGRAM_FORMAT_QASM3};

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

int LRZ_QDMI_device_initialize(void) {
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
  *session = new LRZ_QDMI_Device_Session_impl_d();
  if (*session == nullptr)
    return QDMI_ERROR_OUTOFMEM;

  (*session)->setStatus(LRZ_QDMI_DEVICE_SESSION_STATUS::ALLOCATED);

  return QDMI_SUCCESS;
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

  switch (param) {

  case QDMI_DEVICE_SESSION_PARAMETER_BASEURL:
    session->setURL((char *)value);
    break;
  case QDMI_DEVICE_SESSION_PARAMETER_TOKEN:
    session->setToken((char *)value);
    break;
  case QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1:
    session->setResource((char *)value);
    break;
  default:
    return QDMI_ERROR_NOTIMPLEMENTED;
  }

  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_session_init(LRZ_QDMI_Device_Session session) {

  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  session->init();

  return QDMI_SUCCESS;
}

void LRZ_QDMI_device_session_free(LRZ_QDMI_Device_Session session) {
  delete session;
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

  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_STATUS, QDMI_Device_Status,
                            LRZ_QDMI_read_device_status(), prop, size, value,
                            size_ret)

  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "0.0.1", prop, size,
                      value, size_ret)

  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (prop == QDMI_DEVICE_PROPERTY_CUSTOM1) {
    auto hardwares = session->getAllResourceNames();
    if (value == NULL) {
      size_t hw_size = 0;
      for (auto hardware: hardwares) {
        hw_size += (strlen(hardware.c_str()) + 1);
      }
      *size_ret = hw_size;
      return QDMI_SUCCESS;
    }
    for (size_t i = 0; i < hardwares.size(); i++) {
      if (i != 0)
        strcat((char*)value, ";");
      strcat((char*)value, hardwares[i].c_str());
    }
    return QDMI_SUCCESS;
  }

  if (session->getResource().empty()) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_QUBITSNUM, int,
                            session->getQubitCount(), prop, size, value,
                            size_ret)

  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_SITES, LRZ_QDMI_Site,
                    session->getSites(), prop, size, value, size_ret)

  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_OPERATIONS, LRZ_QDMI_Operation,
                    session->getOperations(), prop, size, value, size_ret)

  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_COUPLINGMAP, LRZ_QDMI_Site,
                    session->getCouplingMap(), prop, size, value, size_ret)

  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_SUPPORTEDPROGRAMFORMATS,
                    QDMI_Program_Format, SUPPORTED_PROGRAM_FORMATS, prop, size,
                    value, size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

int LRZ_QDMI_device_session_query_site_property(LRZ_QDMI_Device_Session session,
                                                LRZ_QDMI_Site site,
                                                const QDMI_Site_Property prop,
                                                const size_t size, void *value,
                                                size_t *size_ret) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int LRZ_QDMI_device_session_query_operation_property(
    LRZ_QDMI_Device_Session session, LRZ_QDMI_Operation operation,
    const size_t num_sites, const LRZ_QDMI_Site *sites, const size_t num_params,
    const double *params, const QDMI_Operation_Property prop, const size_t size,
    void *value, size_t *size_ret) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int LRZ_QDMI_device_session_create_device_job(LRZ_QDMI_Device_Session session,
                                              LRZ_QDMI_Device_Job *job) {
  if (session == NULL || job == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  job = new LRZ_QDMI_Device_Job;

  (*job)->session = session;
  (*job)->status = QDMI_JOB_STATUS_CREATED;

  return QDMI_SUCCESS;
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
    const auto format = *static_cast<const QDMI_Program_Format *>(value);
    if (format != QDMI_PROGRAM_FORMAT_QASM2 &&
        format != QDMI_PROGRAM_FORMAT_QASM3)
      return QDMI_ERROR_NOTSUPPORTED;
    job->circuit_format = format;
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAM) {
    const char *_program = static_cast<const char *>(value);
    strncpy(job->circuit.data(), _program, size);
    job->circuit.data()[size - 1] = '\0';
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM) {
    job->shots = *(const unsigned int *)value;
    return QDMI_SUCCESS;
  }

  return QDMI_ERROR_NOTSUPPORTED;
}

void LRZ_QDMI_device_job_free(LRZ_QDMI_Device_Job job) {
  delete[] static_cast<char *>(job->circuit.data());
  delete job;
}

int LRZ_QDMI_device_job_query_property(LRZ_QDMI_Device_Job job,
                                       const QDMI_Device_Job_Property prop,
                                       const size_t size, void *value,
                                       size_t *size_ret) {
  if (job == nullptr || (value != nullptr && size == 0) ||
      (prop >= QDMI_DEVICE_JOB_PROPERTY_MAX &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM1 &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM2 &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM3 &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM4 &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_STRING_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_ID, job->uuid.c_str(), prop,
                      size, value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_PROGRAMFORMAT,
                            QDMI_Program_Format, job->circuit_format, prop,
                            size, value, size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_PROGRAM, job->circuit.c_str(),
                      prop, size, value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_SHOTSNUM, size_t,
                            job->shots, prop, size, value, size_ret)
  return QDMI_ERROR_NOTSUPPORTED;
}

int LRZ_QDMI_device_job_submit(LRZ_QDMI_Device_Job job) {
  if (job == NULL || job->circuit.empty() ||
      job->circuit_format == QDMI_PROGRAM_FORMAT_MAX || job->shots == 0)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status != QDMI_JOB_STATUS_CREATED)
    return QDMI_ERROR_BADSTATE;

  const char *circuit_format =
      (job->circuit_format) == QDMI_PROGRAM_FORMAT_QASM2 ? "qasm" : "qasm3";

  auto job_request = CircuitJobRequest(job->circuit, circuit_format,
                                       job->resource, job->shots, 0, 0);

  job->jobRequest = job_request;

  auto uuid = job->session->submitJob(job_request);

  if (!uuid.has_value())
    return QDMI_ERROR_FATAL;

  job->status = QDMI_JOB_STATUS_SUBMITTED;

  LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);
  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_job_cancel(LRZ_QDMI_Device_Job job) {
  job->session->cancelJob(job->jobRequest);
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

  auto _status = job->session->getJobStatus(job->jobRequest);

  if (_status == "PENDING") {
    job->status = QDMI_JOB_STATUS_SUBMITTED;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);
  } else if (_status == "WAITING") {
    job->status = QDMI_JOB_STATUS_QUEUED;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);
  } else if (_status == "FAILED") {
    job->status = QDMI_JOB_STATUS_FAILED;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  } else if (_status == "COMPLETED") {
    job->status = QDMI_JOB_STATUS_DONE;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  } else if (_status == "CANCELLED") {
    job->status = QDMI_JOB_STATUS_CANCELED;
    LRZ_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  } else {
    return QDMI_ERROR_FATAL;
  }
  *status = job->status;
  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_job_wait(LRZ_QDMI_Device_Job job, const size_t timeout) {

  if (job == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  job->job_result = job->session->getJobResult(job->jobRequest, true, timeout);

  if (job->job_result == nullptr)
    return QDMI_ERROR_FATAL;

  return QDMI_SUCCESS;
}

int LRZ_QDMI_device_job_get_results(LRZ_QDMI_Device_Job job,
                                    QDMI_Job_Result result, const size_t size,
                                    void *data, size_t *size_ret) {

  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->job_result == nullptr) {
    job->job_result = job->session->getJobResult(job->jobRequest);
    if (job->job_result == nullptr)
      return QDMI_ERROR_INVALIDARGUMENT;
  }

  std::vector<std::map<std::string, unsigned int>> results_ =
      job->job_result->getResults();
  std::map<std::string, unsigned int> counts = results_[0];
  for (auto _count : counts) {
    auto bitstream = _count.first;
    auto count = _count.second;
    job->histogram_keys += (bitstream + ',');
    job->histogram_values.push_back(count);
  }
  job->histogram_keys.data()[job->histogram_keys.size() - 1] = '\0';

  ADD_STRING_PROPERTY(QDMI_JOB_RESULT_HIST_KEYS, job->histogram_keys.c_str(),
                      result, size, data, size_ret)
  ADD_LIST_PROPERTY(QDMI_JOB_RESULT_HIST_VALUES, size_t, job->histogram_values,
                    result, size, data, size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}
