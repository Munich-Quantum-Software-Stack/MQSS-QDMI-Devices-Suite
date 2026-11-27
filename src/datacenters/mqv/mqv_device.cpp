/*
 * Copyright (c) 2026 Munich Quantum Valley
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/** @file
 * @brief QDMI 1.4 Device Interface of the MQV QDMI device.
 * @details Translates QDMI calls into MQP-API requests through MQSS-Client.
 *
 * Session parameters:
 *  - QDMI_DEVICE_SESSION_PARAMETER_BASEURL: MQP-API base URL, e.g.
 *    `https://<mqp-api-host>/v1/` (fallback: env `MQV_QDMI_BASEURL`).
 *  - QDMI_DEVICE_SESSION_PARAMETER_TOKEN: MQP-API token, sent as Bearer token
 *    (fallback: env `MQV_QDMI_TOKEN`).
 *  - QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1: MQP-API resource name
 *    (fallback: env `MQV_QDMI_RESOURCE`).
 *  - QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2: MQP-API job field `no_modify`,
 *    "true" or "false" (fallback: env `MQV_QDMI_NO_MODIFY`; default: true).
 *
 * Session initialization does not contact the MQP-API. The resource is loaded
 * on first use; the device status is probed live (cached for a few seconds).
 * Jobs carry exactly one OpenQASM 2 or 3 program; results are histograms.
 * No C++ exception crosses the C interface.
 */

#include "device.hpp"

#include "mqv_qdmi/device.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <exception>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <vector>

using namespace mqv::qdmi;

namespace {

constexpr std::array SUPPORTED_PROGRAM_FORMATS = {QDMI_PROGRAM_FORMAT_QASM2,
                                                  QDMI_PROGRAM_FORMAT_QASM3};

bool isSupportedFormat(const QDMI_Program_Format format) {
  return format == QDMI_PROGRAM_FORMAT_QASM2 ||
         format == QDMI_PROGRAM_FORMAT_QASM3;
}

/// Run @p body and translate any escaping exception into a QDMI error code.
template <class Body> int guarded(const char *where, Body &&body) noexcept {
  try {
    return body();
  } catch (const std::bad_alloc &) {
    return QDMI_ERROR_OUTOFMEM;
  } catch (const std::exception &e) {
    logError(where, e.what());
    return QDMI_ERROR_FATAL;
  } catch (...) {
    logError(where, "unknown exception");
    return QDMI_ERROR_FATAL;
  }
}

bool isTerminal(const QDMI_Job_Status status) {
  return status == QDMI_JOB_STATUS_DONE || status == QDMI_JOB_STATUS_CANCELED ||
         status == QDMI_JOB_STATUS_FAILED;
}

} // namespace

// ---------------------------------------------------------------------------
// Device lifecycle (the library has no global state)
// ---------------------------------------------------------------------------

int MQV_QDMI_device_initialize() { return QDMI_SUCCESS; }

int MQV_QDMI_device_finalize() { return QDMI_SUCCESS; }

// ---------------------------------------------------------------------------
// Sessions
// ---------------------------------------------------------------------------

int MQV_QDMI_device_session_alloc(MQV_QDMI_Device_Session *session) {
  if (session == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("session_alloc", [&]() -> int {
    *session = new MQV_QDMI_Device_Session_impl_d();
    return QDMI_SUCCESS;
  });
}

int MQV_QDMI_device_session_set_parameter(MQV_QDMI_Device_Session session,
                                          QDMI_Device_Session_Parameter param,
                                          const size_t size,
                                          const void *value) {
  if (session == nullptr || (value != nullptr && size == 0) ||
      (param >= QDMI_DEVICE_SESSION_PARAMETER_MAX && !isCustom(param))) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  const bool supported = param == QDMI_DEVICE_SESSION_PARAMETER_BASEURL ||
                         param == QDMI_DEVICE_SESSION_PARAMETER_TOKEN ||
                         param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1 ||
                         param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2;
  if (!supported) {
    return QDMI_ERROR_NOTSUPPORTED;
  }
  if (value == nullptr) {
    return QDMI_SUCCESS; // support probe
  }
  return guarded("session_set_parameter", [&]() -> int {
    const std::lock_guard lock(session->mutex);
    if (session->state != SessionState::Allocated) {
      return QDMI_ERROR_BADSTATE;
    }
    const auto text = readStringParameter(value, size);
    if (!text.has_value()) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }
    switch (param) {
    case QDMI_DEVICE_SESSION_PARAMETER_BASEURL:
      session->baseUrl = *text;
      break;
    case QDMI_DEVICE_SESSION_PARAMETER_TOKEN:
      session->token = *text;
      break;
    case QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2: {
      const auto flag = parseBool(*text);
      if (!flag.has_value()) {
        return QDMI_ERROR_INVALIDARGUMENT;
      }
      session->noModify = flag;
      break;
    }
    default: // QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1: resource name
      session->resourceName = *text;
      break;
    }
    return QDMI_SUCCESS;
  });
}

int MQV_QDMI_device_session_init(MQV_QDMI_Device_Session session) {
  if (session == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("session_init", [&]() -> int {
    const std::lock_guard lock(session->mutex);
    if (session->state != SessionState::Allocated) {
      return QDMI_ERROR_BADSTATE;
    }
    if (!session->applyEnvironmentDefaults()) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }
    if (!session->baseUrl.empty()) {
      ensureCaBundle();
      // REST mode (is_hpc = false); the constructor does not contact the API.
      session->client = std::make_unique<mqss::client::MQSSClient>(
          session->token, session->baseUrl, false);
    }
    session->state = SessionState::Initialized;
    return QDMI_SUCCESS;
  });
}

void MQV_QDMI_device_session_free(MQV_QDMI_Device_Session session) {
  delete session;
}

int MQV_QDMI_device_session_query_device_property(
    MQV_QDMI_Device_Session session, const QDMI_Device_Property prop,
    const size_t size, void *value, size_t *size_ret) {
  if (session == nullptr ||
      (prop >= QDMI_DEVICE_PROPERTY_MAX && !isCustom(prop))) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("query_device_property", [&]() -> int {
    const std::lock_guard lock(session->mutex);
    if (session->state != SessionState::Initialized) {
      return QDMI_ERROR_BADSTATE;
    }
    switch (prop) {
    case QDMI_DEVICE_PROPERTY_NAME:
      return writeString(session->displayName(), size, value, size_ret);
    case QDMI_DEVICE_PROPERTY_VERSION:
      return writeString(MQV_QDMI_DEVICE_VERSION, size, value, size_ret);
    case QDMI_DEVICE_PROPERTY_LIBRARYVERSION:
      return writeString(QDMI_VERSION, size, value, size_ret);
    case QDMI_DEVICE_PROPERTY_ID:
      return writeString(MQV_QDMI_DEVICE_ID, size, value, size_ret);
    case QDMI_DEVICE_PROPERTY_SUPPORTEDPROGRAMFORMATS:
      return writeList(
          std::vector<QDMI_Program_Format>(SUPPORTED_PROGRAM_FORMATS.begin(),
                                           SUPPORTED_PROGRAM_FORMATS.end()),
          size, value, size_ret);
    case QDMI_DEVICE_PROPERTY_STATUS:
      return writeValue<QDMI_Device_Status>(session->statusLocked(), size,
                                            value, size_ret);
    case QDMI_DEVICE_PROPERTY_QUBITSNUM:
    case QDMI_DEVICE_PROPERTY_SITES:
    case QDMI_DEVICE_PROPERTY_OPERATIONS:
    case QDMI_DEVICE_PROPERTY_COUPLINGMAP: {
      if (const int ret = session->ensureResourceLocked();
          ret != QDMI_SUCCESS) {
        return ret;
      }
      if (prop == QDMI_DEVICE_PROPERTY_QUBITSNUM) {
        return writeValue<size_t>(session->qubitsNum, size, value, size_ret);
      }
      if (prop == QDMI_DEVICE_PROPERTY_SITES) {
        return writeList(session->sites, size, value, size_ret);
      }
      if (prop == QDMI_DEVICE_PROPERTY_OPERATIONS) {
        return writeList(session->operations, size, value, size_ret);
      }
      return writeList(session->couplingMap, size, value, size_ret);
    }
    case QDMI_DEVICE_PROPERTY_QUEUELENGTH: {
      if (session->resourceName.empty() || session->client == nullptr) {
        return QDMI_ERROR_NOTSUPPORTED;
      }
      const int pending =
          session->client->getNumberPendingJobs(session->resourceName);
      if (pending < 0) {
        return QDMI_ERROR_FATAL;
      }
      return writeValue<size_t>(static_cast<size_t>(pending), size, value,
                                size_ret);
    }
    default:
      return QDMI_ERROR_NOTSUPPORTED;
    }
  });
}

int MQV_QDMI_device_session_query_site_property(MQV_QDMI_Device_Session session,
                                                MQV_QDMI_Site site,
                                                const QDMI_Site_Property prop,
                                                const size_t size, void *value,
                                                size_t *size_ret) {
  if (session == nullptr || site == nullptr ||
      (prop >= QDMI_SITE_PROPERTY_MAX && !isCustom(prop))) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (session->state != SessionState::Initialized) {
    return QDMI_ERROR_BADSTATE;
  }
  if (prop == QDMI_SITE_PROPERTY_INDEX) {
    return writeValue<size_t>(site->index, size, value, size_ret);
  }
  return QDMI_ERROR_NOTSUPPORTED;
}

int MQV_QDMI_device_session_query_operation_property(
    MQV_QDMI_Device_Session session, MQV_QDMI_Operation operation,
    const size_t num_sites, const MQV_QDMI_Site *sites, const size_t num_params,
    const double *params, const QDMI_Operation_Property prop, const size_t size,
    void *value, size_t *size_ret) {
  if (session == nullptr || operation == nullptr ||
      (sites != nullptr && num_sites == 0) ||
      (params != nullptr && num_params == 0) ||
      (prop >= QDMI_OPERATION_PROPERTY_MAX && !isCustom(prop))) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (session->state != SessionState::Initialized) {
    return QDMI_ERROR_BADSTATE;
  }
  return guarded("query_operation_property", [&]() -> int {
    switch (prop) {
    case QDMI_OPERATION_PROPERTY_NAME:
      return writeString(operation->name, size, value, size_ret);
    case QDMI_OPERATION_PROPERTY_QUBITSNUM:
      return writeValue<size_t>(operation->qubitsNum, size, value, size_ret);
    case QDMI_OPERATION_PROPERTY_PARAMETERSNUM:
      return writeValue<size_t>(operation->parametersNum, size, value,
                                size_ret);
    case QDMI_OPERATION_PROPERTY_SITES:
      return writeList(operation->sites, size, value, size_ret);
    default:
      return QDMI_ERROR_NOTSUPPORTED;
    }
  });
}

// ---------------------------------------------------------------------------
// Jobs
// ---------------------------------------------------------------------------

int MQV_QDMI_device_session_create_device_job(MQV_QDMI_Device_Session session,
                                              MQV_QDMI_Device_Job *job) {
  if (session == nullptr || job == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (session->state != SessionState::Initialized) {
    return QDMI_ERROR_BADSTATE;
  }
  return guarded("create_device_job", [&]() -> int {
    auto *created = new MQV_QDMI_Device_Job_impl_d();
    created->session = session;
    *job = created;
    return QDMI_SUCCESS;
  });
}

int MQV_QDMI_device_session_retrieve_device_job_by_id(
    MQV_QDMI_Device_Session session, const char *job_id,
    MQV_QDMI_Device_Job *job) {
  if (session == nullptr || job_id == nullptr || job == nullptr ||
      job_id[0] == '\0') {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (session->state != SessionState::Initialized) {
    return QDMI_ERROR_BADSTATE;
  }
  // The MQP-API scopes jobs per token and does not return the original
  // program, so the program list cannot be reconstructed.
  return QDMI_ERROR_NOTSUPPORTED;
}

int MQV_QDMI_device_job_set_parameter(MQV_QDMI_Device_Job job,
                                      const QDMI_Device_Job_Parameter param,
                                      const size_t size, const void *value) {
  if (job == nullptr || (value != nullptr && size == 0) ||
      (param >= QDMI_DEVICE_JOB_PARAMETER_MAX && !isCustom(param))) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (param != QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM) {
    // Includes the reserved values of the removed PROGRAMFORMAT and PROGRAM.
    return QDMI_ERROR_NOTSUPPORTED;
  }
  if (value == nullptr) {
    return QDMI_SUCCESS; // support probe
  }
  if (size != sizeof(size_t)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  const std::lock_guard lock(job->mutex);
  if (job->status != QDMI_JOB_STATUS_CREATED) {
    return QDMI_ERROR_BADSTATE;
  }
  const size_t shots = *static_cast<const size_t *>(value);
  if (shots == 0) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  job->shots = shots;
  return QDMI_SUCCESS;
}

int MQV_QDMI_device_job_set_programs(MQV_QDMI_Device_Job job,
                                     const QDMI_Program_Format format,
                                     const size_t count, const size_t *sizes,
                                     const void *const *programs) {
  if (job == nullptr || count == 0 ||
      (format >= QDMI_PROGRAM_FORMAT_MAX && !isCustom(format))) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (!isSupportedFormat(format) || count != 1) {
    return QDMI_ERROR_NOTSUPPORTED; // one OpenQASM program per job
  }
  if (programs == nullptr) {
    return QDMI_SUCCESS; // support probe
  }
  if (sizes == nullptr || programs[0] == nullptr || sizes[0] == 0) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  const auto *text = static_cast<const char *>(programs[0]);
  const size_t bytes = sizes[0];
  // A text program contains exactly one NUL, as its final byte.
  if (text[bytes - 1] != '\0' ||
      std::memchr(text, '\0', bytes - 1) != nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("job_set_programs", [&]() -> int {
    std::string copy(text, bytes - 1);
    const std::lock_guard lock(job->mutex);
    if (job->status != QDMI_JOB_STATUS_CREATED) {
      return QDMI_ERROR_BADSTATE;
    }
    job->program = std::move(copy);
    job->format = format;
    job->hasProgram = true;
    return QDMI_SUCCESS;
  });
}

int MQV_QDMI_device_job_get_program(MQV_QDMI_Device_Job job,
                                    const size_t program_index,
                                    const size_t size, void *data,
                                    size_t *size_ret) {
  if (job == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("job_get_program", [&]() -> int {
    const std::lock_guard lock(job->mutex);
    if (!job->hasProgram) {
      return QDMI_ERROR_BADSTATE;
    }
    if (program_index != 0) {
      return QDMI_ERROR_OUTOFRANGE;
    }
    return writeString(job->program, size, data, size_ret);
  });
}

int MQV_QDMI_device_job_get_program_status(MQV_QDMI_Device_Job job,
                                           const size_t program_index,
                                           QDMI_Job_Status *status) {
  if (job == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  const std::lock_guard lock(job->mutex);
  if (!job->hasProgram) {
    return QDMI_ERROR_BADSTATE;
  }
  if (program_index != 0) {
    return QDMI_ERROR_OUTOFRANGE;
  }
  if (status == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  *status = job->status; // a single program shares the job's status
  return QDMI_SUCCESS;
}

void MQV_QDMI_device_job_free(MQV_QDMI_Device_Job job) { delete job; }

int MQV_QDMI_device_job_query_property(MQV_QDMI_Device_Job job,
                                       const QDMI_Device_Job_Property prop,
                                       const size_t size, void *value,
                                       size_t *size_ret) {
  if (job == nullptr ||
      (prop >= QDMI_DEVICE_JOB_PROPERTY_MAX && !isCustom(prop))) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("job_query_property", [&]() -> int {
    const std::lock_guard lock(job->mutex);
    switch (prop) {
    case QDMI_DEVICE_JOB_PROPERTY_ID:
      if (job->uuid.empty()) {
        return QDMI_ERROR_BADSTATE; // assigned by the MQP-API on submit
      }
      return writeString(job->uuid, size, value, size_ret);
    case QDMI_DEVICE_JOB_PROPERTY_SHOTSNUM:
      return writeValue<size_t>(job->shots, size, value, size_ret);
    case QDMI_DEVICE_JOB_PROPERTY_PROGRAMFORMAT:
      if (!job->hasProgram) {
        return QDMI_ERROR_BADSTATE;
      }
      return writeValue<QDMI_Program_Format>(job->format, size, value,
                                             size_ret);
    case QDMI_DEVICE_JOB_PROPERTY_PROGRAMSNUM:
      if (!job->hasProgram) {
        return QDMI_ERROR_BADSTATE;
      }
      return writeValue<size_t>(1, size, value, size_ret);
    default:
      return QDMI_ERROR_NOTSUPPORTED;
    }
  });
}

int MQV_QDMI_device_job_submit(MQV_QDMI_Device_Job job) {
  if (job == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("job_submit", [&]() -> int {
    const std::lock_guard lock(job->mutex);
    if (job->status != QDMI_JOB_STATUS_CREATED || !job->hasProgram) {
      return QDMI_ERROR_BADSTATE;
    }
    auto *session = job->session;
    const std::lock_guard sessionLock(session->mutex);
    if (session->client == nullptr || session->resourceName.empty()) {
      logError("job_submit",
               "no MQP-API base URL or resource configured for this session");
      return QDMI_ERROR_BADSTATE;
    }
    job->request = mqss::client::CircuitJobRequest(
        job->program,
        job->format == QDMI_PROGRAM_FORMAT_QASM2 ? "qasm" : "qasm3",
        session->resourceName, static_cast<unsigned int>(job->shots),
        session->noModify.value_or(DEFAULT_NO_MODIFY), /*queued=*/false);
    const auto uuid = session->client->submitJob(job->request);
    if (!uuid.has_value() || uuid->empty()) {
      logError("job_submit", "MQP-API did not accept the job");
      return QDMI_ERROR_FATAL;
    }
    job->request.setUuid(*uuid);
    job->uuid = *uuid;
    job->status = QDMI_JOB_STATUS_SUBMITTED;
    return QDMI_SUCCESS;
  });
}

int MQV_QDMI_device_job_cancel(MQV_QDMI_Device_Job job) {
  if (job == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("job_cancel", [&]() -> int {
    const std::lock_guard lock(job->mutex);
    if (job->status == QDMI_JOB_STATUS_DONE) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }
    if (isTerminal(job->status)) {
      return QDMI_SUCCESS; // already canceled or failed
    }
    if (job->status != QDMI_JOB_STATUS_CREATED) {
      const std::lock_guard sessionLock(job->session->mutex);
      job->session->client->cancelJob(job->request);
    }
    job->status = QDMI_JOB_STATUS_CANCELED;
    return QDMI_SUCCESS;
  });
}

int MQV_QDMI_device_job_check(MQV_QDMI_Device_Job job,
                              QDMI_Job_Status *status) {
  if (job == nullptr || status == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("job_check", [&]() -> int {
    const std::lock_guard lock(job->mutex);
    if (const int ret = job->refreshStatusLocked(); ret != QDMI_SUCCESS) {
      return ret;
    }
    *status = job->status;
    return QDMI_SUCCESS;
  });
}

int MQV_QDMI_device_job_wait(MQV_QDMI_Device_Job job, const size_t timeout) {
  if (job == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  return guarded("job_wait", [&]() -> int {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
    while (true) {
      {
        const std::lock_guard lock(job->mutex);
        if (job->status == QDMI_JOB_STATUS_CREATED) {
          return QDMI_ERROR_INVALIDARGUMENT; // never submitted: would not end
        }
        if (const int ret = job->refreshStatusLocked(); ret != QDMI_SUCCESS) {
          return ret;
        }
        if (isTerminal(job->status)) {
          return QDMI_SUCCESS;
        }
      }
      if (timeout != 0 && std::chrono::steady_clock::now() >= deadline) {
        return QDMI_ERROR_TIMEOUT;
      }
      std::this_thread::sleep_for(WAIT_POLL_INTERVAL);
    }
  });
}

int MQV_QDMI_device_job_get_results(MQV_QDMI_Device_Job job,
                                    const size_t program_index,
                                    const QDMI_Job_Result result,
                                    const size_t size, void *data,
                                    size_t *size_ret) {
  if (job == nullptr || (result >= QDMI_JOB_RESULT_MAX && !isCustom(result))) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (result != QDMI_JOB_RESULT_HIST_KEYS &&
      result != QDMI_JOB_RESULT_HIST_VALUES) {
    return QDMI_ERROR_NOTSUPPORTED;
  }
  return guarded("job_get_results", [&]() -> int {
    const std::lock_guard lock(job->mutex);
    if (!job->hasProgram) {
      return QDMI_ERROR_BADSTATE;
    }
    if (program_index != 0) {
      return QDMI_ERROR_OUTOFRANGE;
    }
    if (job->status != QDMI_JOB_STATUS_DONE) {
      return QDMI_ERROR_BADSTATE;
    }
    if (const int ret = job->fetchResultsLocked(); ret != QDMI_SUCCESS) {
      return ret;
    }
    if (result == QDMI_JOB_RESULT_HIST_KEYS) {
      return writeString(job->histogramKeys, size, data, size_ret);
    }
    return writeList(job->histogramValues, size, data, size_ret);
  });
}
