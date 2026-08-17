/*
 * Copyright (c) 2024 - 2026 MQSS Project
 * All rights reserved.
 *
 * Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#pragma once
#include "lrz_qdmi/constants.h"
#include "lrz_qdmi/types.h"
#include "mqss/job.h"

#include <iostream>
#include <memory>
#include <mqss/client.h>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class LRZ_QDMI_DEVICE_SESSION_STATUS {
  /// The session is allocated but not yet initialized
  ALLOCATED,

  /// The session is successfully initialized
  INITIALIZED,

  /// The session is successfully initialized
  DEFAULT
};

/** @file
 * @brief The header file for `LRZ_QDMI_Device_Session`
 */

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
  std::string resourceName; // ok

  /// MQSS Client object. For more information, please see @return
  /// <ahref="https://munich-quantum-software-stack.github.io/MQSS-Client/>here</a>
  mqss::client::MQSSClient client; // ok

  /// The status of the session
  LRZ_QDMI_DEVICE_SESSION_STATUS status;

  std::vector<LRZ_QDMI_Operation> operations = {}; // ok

  std::vector<LRZ_QDMI_Site> sites = {}; // ok

  std::vector<LRZ_QDMI_Site> couplingMap; // ok

  void setSites();

  void setOperations();

  void setCouplingMap();

  std::vector<LRZ_QDMI_Site>
  setSupportedSites(std::vector<std::vector<unsigned int>> supportedQubits);

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

  std::string getURL() { return url; }

  /**
   * @brief Set the token for authentication
   * @param[in] new_token The token to be set.
   */
  void setToken(const std::string &newToken) { token = newToken; }
  std::string getToken() { return token; }

  /**
   * @brief Set the session's status
   * @param[in] new_status The status to be set.
   */
  void setStatus(LRZ_QDMI_DEVICE_SESSION_STATUS newStatus) {
    status = newStatus;
  }

  /**
   * @brief Get function for the current session status
   *
   * @returns The current session status
   */
  LRZ_QDMI_DEVICE_SESSION_STATUS getStatus() { return status; }

  /**
   * @brief Set the session's resource
   * @param[in] new_resource The resource to be set.
   */
  void setResource(const std::string &newResourceName);

  /**
   * @brief Returns the session's resource
   */
  const std::string getResource() { return resourceName; }

  std::vector<LRZ_QDMI_Site> getSites() {
    return !resource.has_value() ? std::vector<LRZ_QDMI_Site>{} : sites;
  }

  std::vector<LRZ_QDMI_Operation> getOperations() {
    return !resource.has_value() ? std::vector<LRZ_QDMI_Operation>{}
                                 : operations;
  }

  std::vector<LRZ_QDMI_Site> getCouplingMap() {
    return !resource.has_value() ? std::vector<LRZ_QDMI_Site>{} : couplingMap;
  }

  /**
   * @brief Public function to init to the MQSSClient session.
   *
   * @details This function creates a sessions of MQSSClient with given
   * parameters using the
   * @ref LRZ_QDMI_device_session_set_parameter.
   *
   */
  void init() { client = mqss::client::MQSSClient(token, url, false); }

  int getQubitCount() {
    return !resource.has_value() ? QDMI_ERROR_INVALIDARGUMENT
                                 : (int)(*resource).getQubitCount();
  }

  std::optional<std::string> submitJob(mqss::client::CircuitJobRequest job) {
    return client.submitJob(job);
  }

  void cancelJob(mqss::client::CircuitJobRequest job) { client.cancelJob(job); }

  std::string getJobStatus(mqss::client::CircuitJobRequest job) {
    return client.getJobStatus(job);
  }

  std::unique_ptr<mqss::client::JobResult>
  getJobResult(mqss::client::CircuitJobRequest job, bool wait = false,
               size_t timeout = 0) {

    return client.getJobResult(job, wait, timeout);
  }

  std::vector<std::string> getAllResourceNames();
};
