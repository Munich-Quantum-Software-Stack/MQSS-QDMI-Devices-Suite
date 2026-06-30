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
#pragma once
#include "lrz_qdmi/constants.h"
#include "lrz_qdmi/types.h"
#include "mqss/job.h"
#include <iostream>
#include <mqss/client.h>
#include <optional>
#include <string>
#include <vector>

/** @file
 * @brief The header file for `LRZ_QDMI_Device_Session`
 */

/**
 * @brief Enum of the session status that can be set internally.
 *
 * @see LRZ_QDMI_device_session_alloc
 */
enum class LRZ_QDMI_DEVICE_SESSION_STATUS {
  /// The session is allocated but not yet initialized
  ALLOCATED,

  /// The session is successfully initialized
  INITIALIZED,

  /// The session is successfully initialized
  DEFAULT
};

struct LRZ_QDMI_Site_impl_d {
  size_t index;

public:
  LRZ_QDMI_Site_impl_d(size_t idx) : index(idx){};
};

struct LRZ_QDMI_Operation_impl_d {

public:
  std::string name;
  size_t qubitNumber;
  size_t parameterNumber;
  bool zoned = false;
  std::vector<LRZ_QDMI_Site> supportedSites;
};

/**
 * @brief The implementation of the encapsulated type
 * QDMI_Device_Session on the device-side.
 * @details Implemented to hold all the required configuration and functions to
 connect to the MQP.
 * @note QDMI_Device_Session is encapsulated in the QDMI Device Session
 * Interface to allow the device implement the type as needed.
 */
struct LRZ_QDMI_Device_Session_impl_d {
private:
  /// URL of the MQP
  std::string url = "https://portal.quantum.lrz.de:4000/v1/";

  /// Authentication token
  std::string token;

  /// Resource to be query or submit job to
  std::optional<mqss::client::Resource> resource;

  /// Resource name
  std::string resourceName;

  /// MQSS Client object. For more information, please see @return
  /// <ahref="https://munich-quantum-software-stack.github.io/MQSS-Client/>here</a>
  mqss::client::MQSSClient client;

  /// The status of the session
  LRZ_QDMI_DEVICE_SESSION_STATUS status;

  std::optional<std::vector<LRZ_QDMI_Site>> sites;

  std::optional<std::vector<LRZ_QDMI_Operation_impl_d>> operations;

  std::optional<std::vector<LRZ_QDMI_Site>> couplingMap;

  void setSites() {
    if (sites.has_value())
      sites->clear();

    for (size_t index = 0; index < (*resource).getQubitCount(); index++) {
      (*sites).push_back(new LRZ_QDMI_Site_impl_d(index));
    }
  }

  std::vector<LRZ_QDMI_Site>
  setSupportedSites(std::vector<std::vector<unsigned int>> supportedQubits) {
    std::vector<LRZ_QDMI_Site> supportedSites;
    for (auto qubitPair : supportedQubits) {
      for (auto qubit : qubitPair) {
        supportedSites.emplace_back((*sites).at(qubit));
      }
    }
    return supportedSites;
  }

  void setOperations() {
    if (operations.has_value())
      operations->clear();
    auto x = (*resource).getNativeGateset();
    for (auto gate : (*resource).getNativeGateset()) {
      LRZ_QDMI_Operation operation = new LRZ_QDMI_Operation_impl_d();
      operation->name = gate.getName();
      operation->qubitNumber = gate.getQubitNumber();
      operation->parameterNumber = gate.getParameterNumber();
      operation->zoned = false;
      operation->supportedSites =
          setSupportedSites(gate.getSupportedQubits());
      operations->emplace_back(*operation);
    }
  }

  void setCouplingMap() {
    if (couplingMap.has_value())
      couplingMap->clear();
    for (auto qubitPair : (*resource).getCouplingMap()) {
      for (auto qubit : qubitPair) {
        (*couplingMap).emplace_back((*sites).at(qubit));
      }
    }
  }

public:
  /**
   * @brief Public function to set URL for the MQP.
   *
   * @details Set values are used to connect to the MQP using @ref
   * LRZ_QDMI_Device_Session_impl_d::init
   *
   * @param[in] baseUrl The base url to be set, i.e,
   * https://portal.quantum.lrz.de:4000/v1/
   */

  void setURL(const std::string &newUrl) { url = newUrl; }

  /**
   * @brief Set the token for authentication
   * @param[in] new_token The token to be set.
   */
  void setToken(const std::string &newToken) { token = newToken; }

  /**
   * @brief Set the session's status
   * @param[in] new_status The status to be set.
   */
  void setStatus(LRZ_QDMI_DEVICE_SESSION_STATUS newStatus) {
    status = newStatus;
  }

  /**
   * @brief Set the session's resource
   * @param[in] new_resource The resource to be set.
   */
  void setResource(const std::string &newResourceName) {
    resourceName = newResourceName;
    resource = client.getResourceInfo(resourceName);
    setSites();
    setOperations();
    setCouplingMap();
  }

  /**
   * @brief Returns the session's resource
   */
  const std::string getResource() { return resourceName; }

  /**
   * @brief Get function for the current session status
   *
   * @returns The current session status
   */
  LRZ_QDMI_DEVICE_SESSION_STATUS getStatus() { return status; }

  /**
   * @brief Public function to init to the MQSSClient session.
   *
   * @details This function creates a sessions of MQSSClient with given
   * parameters using the
   * @ref LRZ_QDMI_device_session_set_parameter.
   *
   */
  void init() {
    client = mqss::client::MQSSClient(token, url, false);
  }

  std::vector<std::string> getAllResourceNames() {
    std::vector<std::string> resouceNames;
    std::vector<mqss::client::Resource> resources = client.getAllResources();
    for (mqss::client::Resource resourceInfo : resources) {
      resouceNames.push_back(resourceInfo.getName());
    }
    return resouceNames;
  }

  int getQubitCount() {
    if (!resource.has_value()) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }

    return (int)(*resource).getQubitCount();
  }

  std::vector<LRZ_QDMI_Site> getSites() {
    if (!resource.has_value()) {
      return {};
    }

    return *sites;
  }

  auto getOperations() { return *operations; }

  auto getCouplingMap() { return *couplingMap; }

  std::optional<std::string> submitJob(mqss::client::CircuitJobRequest) {}

  auto cancelJob(mqss::client::CircuitJobRequest) {}

  std::string getJobStatus(mqss::client::CircuitJobRequest) {}

  std::unique_ptr<mqss::client::JobResult>
  getJobResult(mqss::client::CircuitJobRequest, bool wait = false,
               size_t timeout = 0) {}
};
