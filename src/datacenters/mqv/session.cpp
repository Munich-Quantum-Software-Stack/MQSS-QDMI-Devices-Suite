/*
 * Copyright (c) 2026 Munich Quantum Valley
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/** @file
 * @brief Session and job helpers of the MQV QDMI device (no C entry points).
 */

#include "device.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <exception>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <utility>

namespace mqv::qdmi {

int writeString(const std::string &prop, const std::size_t size, void *value,
                std::size_t *sizeRet) {
  const std::size_t bytes = prop.size() + 1;
  if (value != nullptr) {
    if (size < bytes) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }
    std::memcpy(value, prop.c_str(), bytes);
  }
  if (sizeRet != nullptr) {
    *sizeRet = bytes;
  }
  return QDMI_SUCCESS;
}

std::optional<std::string> readStringParameter(const void *value,
                                               const std::size_t size) {
  const auto *chars = static_cast<const char *>(value);
  const auto *end = static_cast<const char *>(std::memchr(chars, '\0', size));
  if (end == nullptr) {
    return std::nullopt;
  }
  return std::string(chars, end);
}

std::optional<bool> parseBool(const std::string &text) {
  std::string lower = text;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (lower == "true" || lower == "1") {
    return true;
  }
  if (lower == "false" || lower == "0") {
    return false;
  }
  return std::nullopt;
}

namespace {
std::size_t appendBody(char *data, std::size_t size, std::size_t count,
                       void *body) {
  static_cast<std::string *>(body)->append(data, size * count);
  return size * count;
}
} // namespace

std::optional<std::string> httpGet(const std::string &url,
                                   const std::string &token) {
  CURL *curl = curl_easy_init();
  if (curl == nullptr) {
    logError("http", "curl_easy_init failed");
    return std::nullopt;
  }
  std::string body;
  const std::string auth = "Authorization: Bearer " + token;
  curl_slist *headers = curl_slist_append(nullptr, auth.c_str());
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, appendBody);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
  // Same CA bundle as MQSS-Client uses (see ensureCaBundle).
  if (const char *bundle = std::getenv("CURL_CA_BUNDLE"); bundle != nullptr) {
    curl_easy_setopt(curl, CURLOPT_CAINFO, bundle);
  }
  const CURLcode res = curl_easy_perform(curl);
  long code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    logError("http", "GET " + url + ": " + curl_easy_strerror(res));
    return std::nullopt;
  }
  if (code != 200) {
    logError("http", "GET " + url + ": HTTP " + std::to_string(code));
    return std::nullopt;
  }
  return body;
}

std::optional<std::vector<std::pair<std::string, std::size_t>>>
parseResultHistogram(const std::string &body) {
  try {
    const auto parsed = nlohmann::json::parse(body);
    const auto &text = parsed.at("result").get_ref<const std::string &>();
    auto histogram = nlohmann::json::parse(text);
    if (histogram.is_array()) {
      if (histogram.empty()) {
        return std::nullopt;
      }
      histogram = histogram.front();
    }
    if (histogram.contains("counts") && histogram["counts"].is_object()) {
      histogram = histogram["counts"];
    }
    if (!histogram.is_object() || histogram.empty()) {
      return std::nullopt;
    }
    std::vector<std::pair<std::string, std::size_t>> counts;
    for (const auto &[bitstring, count] : histogram.items()) {
      counts.emplace_back(bitstring, count.get<std::size_t>());
    }
    return counts;
  } catch (const std::exception &e) {
    logError("result", e.what());
    return std::nullopt;
  }
}

std::optional<QDMI_Job_Status> mapJobStatus(const std::string &status) {
  if (status == "PENDING") {
    return QDMI_JOB_STATUS_SUBMITTED;
  }
  if (status == "WAITING" || status == "QUEUED") {
    return QDMI_JOB_STATUS_QUEUED;
  }
  if (status == "RUNNING") {
    return QDMI_JOB_STATUS_RUNNING;
  }
  if (status == "COMPLETED") {
    return QDMI_JOB_STATUS_DONE;
  }
  if (status == "CANCELLED" || status == "CANCELED") {
    return QDMI_JOB_STATUS_CANCELED;
  }
  if (status == "FAILED") {
    return QDMI_JOB_STATUS_FAILED;
  }
  return std::nullopt;
}

void logError(const std::string &where, const std::string &what) {
  std::cerr << "[mqv-qdmi-device] " << where << ": " << what << '\n';
}

void ensureCaBundle() {
  if (std::getenv("CURL_CA_BUNDLE") != nullptr) {
    return;
  }
  // RHEL/Rocky, Debian/Ubuntu, macOS/Alpine.
  for (const char *path :
       {"/etc/pki/tls/certs/ca-bundle.crt",
        "/etc/ssl/certs/ca-certificates.crt", "/etc/ssl/cert.pem"}) {
    if (std::FILE *file = std::fopen(path, "r"); file != nullptr) {
      std::fclose(file);
      setenv("CURL_CA_BUNDLE", path, 0);
      return;
    }
  }
}

std::optional<mqss::client::Resource>
fetchResource(const mqss::client::MQSSClient &client, const std::string &name) {
  // MQSS-Client turns a transport failure into the body "1" or "2", which is
  // valid JSON (a number), so building a Resource from it throws.
  try {
    if (auto resource = client.getResourceInfo(name); resource.has_value()) {
      return resource;
    }
  } catch (const std::exception &e) {
    logError("resource", "GET resources/" + name + ": " + e.what());
  }
  try {
    for (auto &candidate : client.getAllResources()) {
      if (candidate.getName() == name) {
        return candidate;
      }
    }
  } catch (const std::exception &e) {
    logError("resource", std::string("GET resources: ") + e.what());
  }
  return std::nullopt;
}

} // namespace mqv::qdmi

using namespace mqv::qdmi;

bool MQV_QDMI_Device_Session_impl_d::applyEnvironmentDefaults() {
  const auto fill = [](std::string &target, const char *name) {
    if (target.empty()) {
      if (const char *env = std::getenv(name); env != nullptr) {
        target = env;
      }
    }
  };
  fill(baseUrl, ENV_BASEURL);
  fill(token, ENV_TOKEN);
  fill(resourceName, ENV_RESOURCE);
  if (!noModify.has_value()) {
    if (const char *env = std::getenv(ENV_NO_MODIFY); env != nullptr) {
      noModify = parseBool(env);
      if (!noModify.has_value()) {
        logError("session", std::string(ENV_NO_MODIFY) + "='" + env +
                                "' is not true/false");
        return false;
      }
    }
  }
  return true;
}

std::string MQV_QDMI_Device_Session_impl_d::displayName() const {
  return resourceName.empty() ? "MQV" : "MQV " + resourceName;
}

int MQV_QDMI_Device_Session_impl_d::ensureResourceLocked() {
  if (resourceLoaded || resourceName.empty()) {
    return QDMI_SUCCESS;
  }
  if (client == nullptr) {
    logError("resource", "no MQP-API base URL configured (set the session "
                         "parameter BASEURL or MQV_QDMI_BASEURL)");
    return QDMI_ERROR_FATAL;
  }
  const auto resource = fetchResource(*client, resourceName);
  if (!resource.has_value()) {
    logError("resource", "MQP-API does not know resource '" + resourceName +
                             "' (or rejected the request)");
    return QDMI_ERROR_FATAL;
  }
  auto res = *resource; // getNativeGateset() is non-const in MQSS-Client.

  const std::size_t qubits = res.getQubitCount();
  std::vector<std::unique_ptr<MQV_QDMI_Site_impl_d>> newSiteStorage;
  std::vector<MQV_QDMI_Site> newSites;
  newSiteStorage.reserve(qubits);
  newSites.reserve(qubits);
  for (std::size_t i = 0; i < qubits; ++i) {
    newSiteStorage.emplace_back(std::make_unique<MQV_QDMI_Site_impl_d>(
        MQV_QDMI_Site_impl_d{.index = i}));
    newSites.push_back(newSiteStorage.back().get());
  }

  std::vector<MQV_QDMI_Site> newCoupling;
  for (const auto &edge : res.getCouplingMap()) {
    if (edge.size() != 2 || edge[0] < 0 || edge[1] < 0 ||
        static_cast<std::size_t>(edge[0]) >= qubits ||
        static_cast<std::size_t>(edge[1]) >= qubits) {
      logError("resource", "ignoring invalid coupling-map entry");
      continue;
    }
    newCoupling.push_back(newSites[static_cast<std::size_t>(edge[0])]);
    newCoupling.push_back(newSites[static_cast<std::size_t>(edge[1])]);
  }
  // MQSS-Client parses only "[a, b]" pairs, so connectivity written as
  // "[(a, b), ...]" (e.g. MAQCS) also comes back empty. Then the pairs of the
  // explicitly placed two-qubit gates are the coupling map.
  if (newCoupling.empty()) {
    std::set<std::pair<unsigned, unsigned>> pairs;
    for (const auto &gate : res.getNativeGateset()) {
      for (const auto &placement : gate.getSupportedQubits()) {
        if (placement.size() == 2 && placement[0] < qubits &&
            placement[1] < qubits && placement[0] != placement[1]) {
          pairs.emplace(placement[0], placement[1]);
        }
      }
    }
    for (const auto &[a, b] : pairs) {
      newCoupling.push_back(newSites[a]);
      newCoupling.push_back(newSites[b]);
    }
  }
  // MQP-API reports connectivity "None" and instructions {None: None} for
  // devices without a coupling restriction (e.g. trapped ions such as AQT20):
  // all-to-all, both directions.
  if (newCoupling.empty()) {
    for (std::size_t i = 0; i < qubits; ++i) {
      for (std::size_t j = 0; j < qubits; ++j) {
        if (i != j) {
          newCoupling.push_back(newSites[i]);
          newCoupling.push_back(newSites[j]);
        }
      }
    }
  }

  std::vector<std::unique_ptr<MQV_QDMI_Operation_impl_d>> newOpStorage;
  std::vector<MQV_QDMI_Operation> newOps;
  bool hasMeasure = false;
  for (const auto &gate : res.getNativeGateset()) {
    auto op = std::make_unique<MQV_QDMI_Operation_impl_d>();
    op->name = gate.getName();
    op->qubitsNum = gate.getQubitNumber();
    op->parametersNum = gate.getParameterNumber();
    for (const auto &placement : gate.getSupportedQubits()) {
      if (placement.size() != op->qubitsNum) {
        continue;
      }
      bool valid = true;
      for (const auto q : placement) {
        valid = valid && q < qubits;
      }
      if (!valid) {
        continue;
      }
      for (const auto q : placement) {
        op->sites.push_back(newSites[q]);
      }
    }
    // An instruction listed as {None: None} may act on any qubits;
    // MQSS-Client returns it without placements.
    if (op->sites.empty() && op->qubitsNum == 1) {
      op->sites = newSites;
    } else if (op->sites.empty() && op->qubitsNum == 2) {
      op->sites = newCoupling;
    }
    hasMeasure = hasMeasure || op->name == "measure";
    newOps.push_back(op.get());
    newOpStorage.push_back(std::move(op));
  }
  // MQT Core's Qiskit backend rejects circuits whose instructions are not
  // device operations, and every circuit measures. Report `measure` on all
  // qubits if the MQP-API resource does not list it.
  if (!hasMeasure && qubits > 0) {
    auto op = std::make_unique<MQV_QDMI_Operation_impl_d>();
    op->name = "measure";
    op->qubitsNum = 1;
    op->parametersNum = 0;
    op->sites = newSites;
    newOps.push_back(op.get());
    newOpStorage.push_back(std::move(op));
  }

  qubitsNum = qubits;
  siteStorage = std::move(newSiteStorage);
  sites = std::move(newSites);
  couplingMap = std::move(newCoupling);
  operationStorage = std::move(newOpStorage);
  operations = std::move(newOps);
  resourceLoaded = true;
  return QDMI_SUCCESS;
}

QDMI_Device_Status MQV_QDMI_Device_Session_impl_d::statusLocked() {
  if (resourceName.empty() || client == nullptr) {
    return QDMI_DEVICE_STATUS_OFFLINE;
  }
  const auto now = std::chrono::steady_clock::now();
  if (cachedStatus.has_value() && now - cachedStatusAt < STATUS_CACHE_TTL) {
    return *cachedStatus;
  }
  QDMI_Device_Status status = QDMI_DEVICE_STATUS_OFFLINE;
  try {
    if (const auto resource = fetchResource(*client, resourceName);
        resource.has_value()) {
      status = resource->isOnline() ? QDMI_DEVICE_STATUS_IDLE
                                    : QDMI_DEVICE_STATUS_MAINTENANCE;
    }
  } catch (const std::exception &e) {
    logError("status", e.what()); // unreachable site: report OFFLINE
  }
  cachedStatus = status;
  cachedStatusAt = now;
  return status;
}

int MQV_QDMI_Device_Job_impl_d::refreshStatusLocked() {
  if (status == QDMI_JOB_STATUS_CREATED || status == QDMI_JOB_STATUS_DONE ||
      status == QDMI_JOB_STATUS_CANCELED || status == QDMI_JOB_STATUS_FAILED) {
    return QDMI_SUCCESS; // nothing to ask the MQP-API
  }
  std::string remote;
  {
    const std::lock_guard sessionLock(session->mutex);
    remote = session->client->getJobStatus(request);
  }
  const auto mapped = mapJobStatus(remote);
  if (!mapped.has_value()) {
    logError("job " + uuid, "unknown MQP-API job status '" + remote + "'");
    return QDMI_ERROR_FATAL;
  }
  status = *mapped;
  return QDMI_SUCCESS;
}

int MQV_QDMI_Device_Job_impl_d::fetchResultsLocked() {
  if (hasResults) {
    return QDMI_SUCCESS;
  }
  // Read GET job/<id>/result ourselves: MQSS-Client's JobResult throws on the
  // {"counts": {...}} shape MQP-API workers return (see parseResultHistogram).
  std::optional<std::string> body;
  {
    const std::lock_guard sessionLock(session->mutex);
    body =
        httpGet(session->baseUrl + request.getPath() + "/" + uuid + "/result",
                session->token);
  }
  if (!body.has_value()) {
    logError("job " + uuid, "MQP-API returned no result");
    return QDMI_ERROR_FATAL;
  }
  const auto histogram = parseResultHistogram(*body);
  if (!histogram.has_value()) {
    logError("job " + uuid, "MQP-API result contains no histogram");
    return QDMI_ERROR_FATAL;
  }
  std::string keys;
  std::vector<std::size_t> values;
  for (const auto &[bitstring, count] : *histogram) {
    if (!keys.empty()) {
      keys += ',';
    }
    keys += bitstring;
    values.push_back(count);
  }
  histogramKeys = std::move(keys);
  histogramValues = std::move(values);
  hasResults = true;
  return QDMI_SUCCESS;
}
