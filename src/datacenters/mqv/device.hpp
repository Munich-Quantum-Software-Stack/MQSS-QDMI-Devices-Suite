/*
 * Copyright (c) 2026 Munich Quantum Valley
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/** @file
 * @brief Internal data structures of the MQV QDMI device.
 * @details The device connects to an MQSS MQP-API (REST) through MQSS-Client.
 * All per-endpoint state lives in the device session, so that one library can
 * serve several configured devices (e.g. several MQP-API sites or resources)
 * at the same time. The library itself holds no mutable global state.
 *
 * Based on the LRZ device in this repository (src/datacenters/lrz, as of
 * 0ce0dc0), ported to QDMI 1.4, with changes for the MQP-API deployed at MQV.
 */

#pragma once

#include "mqv_qdmi/constants.h"
#include "mqv_qdmi/device.h" // handle types (MQV_QDMI_Device_Session, ...)
#include "mqv_qdmi/types.h"

#include <chrono>
#include <cstddef>
#include <cstring>
#include <memory>
#include <mqss/client.h>
#include <mqss/job.h>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mqv::qdmi {

/// Environment variables used when the corresponding session parameter is not
/// set. They keep tokens out of configuration files and manifests.
inline constexpr const char *ENV_BASEURL = "MQV_QDMI_BASEURL";
inline constexpr const char *ENV_TOKEN = "MQV_QDMI_TOKEN";
inline constexpr const char *ENV_RESOURCE = "MQV_QDMI_RESOURCE";
inline constexpr const char *ENV_NO_MODIFY = "MQV_QDMI_NO_MODIFY";

/// The MQP-API job field no_modify when neither CUSTOM2 nor ENV_NO_MODIFY is
/// set. True: a QDMI client (e.g. MQT Core's Qiskit backend) sends programs
/// already in the device's native gates, and MQSS currently accepts OpenQASM
/// only with no_modify=true.
inline constexpr bool DEFAULT_NO_MODIFY = true;

/// Shot count used when a job does not set QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM.
inline constexpr std::size_t DEFAULT_SHOTS = 1024;

/// How long a probed device status is reused before the MQP-API is asked again.
inline constexpr std::chrono::seconds STATUS_CACHE_TTL{5};

/// Interval between status polls while waiting for a job.
inline constexpr std::chrono::milliseconds WAIT_POLL_INTERVAL{500};

/// Whether @p value is one of the five CUSTOM enum values QDMI reserves.
constexpr bool isCustom(const long long value) {
  return value >= 999999995LL && value <= 999999999LL;
}

/// Write a single value of type T (property query semantics of QDMI).
template <class T>
int writeValue(const T &prop, const std::size_t size, void *value,
               std::size_t *sizeRet) {
  if (value != nullptr) {
    if (size < sizeof(T)) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }
    *static_cast<T *>(value) = prop;
  }
  if (sizeRet != nullptr) {
    *sizeRet = sizeof(T);
  }
  return QDMI_SUCCESS;
}

/// Write a NUL-terminated string (property query semantics of QDMI).
int writeString(const std::string &prop, std::size_t size, void *value,
                std::size_t *sizeRet);

/// Write a contiguous list of T (property query semantics of QDMI).
template <class T>
int writeList(const std::vector<T> &prop, const std::size_t size, void *value,
              std::size_t *sizeRet) {
  const std::size_t bytes = prop.size() * sizeof(T);
  if (value != nullptr) {
    if (size < bytes) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }
    if (bytes > 0) {
      std::memcpy(value, prop.data(), bytes);
    }
  }
  if (sizeRet != nullptr) {
    *sizeRet = bytes;
  }
  return QDMI_SUCCESS;
}

/// Read a NUL-terminated string parameter of at most @p size bytes.
/// Returns std::nullopt if no terminating NUL lies within @p size.
std::optional<std::string> readStringParameter(const void *value,
                                               std::size_t size);

/// Parse a boolean parameter: "true"/"false" or "1"/"0" (case-insensitive).
std::optional<bool> parseBool(const std::string &text);

/// GET @p url with a Bearer @p token through libcurl. Returns the body for
/// HTTP 200, std::nullopt otherwise (logged). Never throws.
std::optional<std::string> httpGet(const std::string &url,
                                   const std::string &token);

/// Parse the body of GET job/<id>/result into the first histogram. The
/// "result" field is a JSON string holding {"00": n, ...}, a list of those,
/// or either wrapped as {"counts": {...}} (as MQP-API workers return it).
/// MQSS-Client's JobResult parses only the first two. Never throws.
std::optional<std::vector<std::pair<std::string, std::size_t>>>
parseResultHistogram(const std::string &body);

/// Map an MQP-API job status string to a QDMI job status.
std::optional<QDMI_Job_Status> mapJobStatus(const std::string &status);

/// Log a diagnostic message to stderr with the device prefix.
void logError(const std::string &where, const std::string &what);

/// Point CURL_CA_BUNDLE at the system CA bundle if it is unset.
/// MQSS-Client passes getenv("CURL_CA_BUNDLE") to CURLOPT_CAINFO on every
/// request; unset, that is NULL, which disables libcurl's default bundle and
/// makes every HTTPS request fail.
void ensureCaBundle();

/// Fetch one MQP-API resource by name. Falls back to the resource list when
/// GET resources/<name> fails: some deployments deny it to tokens that may
/// list all resources. Never throws; std::nullopt if neither works.
std::optional<mqss::client::Resource>
fetchResource(const mqss::client::MQSSClient &client, const std::string &name);

enum class SessionState { Allocated, Initialized };

} // namespace mqv::qdmi

/// A qubit of the configured MQP-API resource.
struct MQV_QDMI_Site_impl_d {
  std::size_t index;
};

/// A native gate of the configured MQP-API resource.
struct MQV_QDMI_Operation_impl_d {
  std::string name;
  std::size_t qubitsNum = 0;
  std::size_t parametersNum = 0;
  /// Supported placements, flattened: qubitsNum consecutive sites each.
  std::vector<MQV_QDMI_Site> sites;
};

struct MQV_QDMI_Device_Session_impl_d {
  mqv::qdmi::SessionState state = mqv::qdmi::SessionState::Allocated;

  // Session parameters (QDMI_DEVICE_SESSION_PARAMETER_BASEURL, _TOKEN,
  // _CUSTOM1 = MQP-API resource name, _CUSTOM2 = no_modify), with environment
  // fallbacks at init.
  std::string baseUrl;
  std::string token;
  std::string resourceName;
  std::optional<bool> noModify;

  /// Serializes lazy loading, status caching and client use.
  std::mutex mutex;

  /// Created at init; constructing it does not contact the MQP-API.
  std::unique_ptr<mqss::client::MQSSClient> client;

  /// Resource description, loaded lazily on first use and then kept, so that
  /// site and operation handles stay valid for the lifetime of the session.
  bool resourceLoaded = false;
  std::size_t qubitsNum = 0;
  std::vector<std::unique_ptr<MQV_QDMI_Site_impl_d>> siteStorage;
  std::vector<MQV_QDMI_Site> sites;
  std::vector<std::unique_ptr<MQV_QDMI_Operation_impl_d>> operationStorage;
  std::vector<MQV_QDMI_Operation> operations;
  /// Coupling map, flattened: pairs of consecutive sites.
  std::vector<MQV_QDMI_Site> couplingMap;

  std::optional<QDMI_Device_Status> cachedStatus;
  std::chrono::steady_clock::time_point cachedStatusAt;

  /// Fill unset parameters from the environment (ENV_* above). Returns false
  /// if ENV_NO_MODIFY is set but not a boolean.
  bool applyEnvironmentDefaults();

  /// Display name reported as QDMI_DEVICE_PROPERTY_NAME.
  [[nodiscard]] std::string displayName() const;

  /// Load the resource description once. Without a configured resource the
  /// session describes an empty device (0 qubits) and returns QDMI_SUCCESS.
  /// Caller must hold @ref mutex.
  int ensureResourceLocked();

  /// Live device status, cached for STATUS_CACHE_TTL. Caller must hold mutex.
  QDMI_Device_Status statusLocked();
};

struct MQV_QDMI_Device_Job_impl_d {
  MQV_QDMI_Device_Session session = nullptr;
  std::mutex mutex;

  QDMI_Job_Status status = QDMI_JOB_STATUS_CREATED;

  // Program list (this device accepts exactly one program per job).
  bool hasProgram = false;
  QDMI_Program_Format format = QDMI_PROGRAM_FORMAT_QASM2;
  std::string program; ///< Program text without the trailing NUL.

  std::size_t shots = mqv::qdmi::DEFAULT_SHOTS;

  mqss::client::CircuitJobRequest request;
  std::string uuid;

  bool hasResults = false;
  std::string histogramKeys; ///< Comma-separated bitstrings.
  std::vector<std::size_t> histogramValues;

  /// Refresh @ref status from the MQP-API. Caller must hold @ref mutex.
  int refreshStatusLocked();

  /// Fetch and store the histogram. Caller must hold @ref mutex.
  int fetchResultsLocked();
};
