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

#include "dcdb_qdmi/device.h"
#include "qdmi/constants.h"

#include <cassandra.h>
#include <dcdb/connection.h>
#include <iostream>
#include <string>

/** @file
 * @brief The header file for `QDMI_Device_Session`
 */

/**
 * @brief Enum of the session status that can be set internally.
 *
 * @see DCDB_QDMI_device_session_alloc
 * @see DCDB_QDMI_Device_Session_impl_d::connect
 */
enum class DCDB_QDMI_DEVICE_SESSION_STATUS : uint8_t {
  /// The session is allocated but not yet initialized
  ALLOCATED,

  /// The session is successfully initialized
  INITIALIZED
};

/**
 * @brief The implementation of the encapsulated type
 * QDMI_Device_Session on the device-side.
 * @details Implemented to hold all the required configuration and functions to
 connect the DCDB host at LRZ.
 * @note QDMI_Device_Session is encapsulated in the QDMI Device Session
 * Interface to allow the device implement the type as needed.
 */
struct DCDB_QDMI_Device_Session_impl_d {
private:
  /// Hostname of the DCDB server
  std::string hostname = "stor";

  /// Port number of the DCDB server
  uint16_t port = 9042;

  /// Username used for authentication
  std::string username = "";

  /// Password used for authentication
  std::string password = "";

  /// Pointer to the underlying DCDB connection object
  DCDB::Connection *connection = nullptr;

  /// Internal connection method using provided parameters
  int connect(std::string hostname, uint16_t port, std::string username,
              std::string password);

  /// Current status of the session
  DCDB_QDMI_DEVICE_SESSION_STATUS status;

  /// Set a new hostname for the session
  void setHostname(const std::string &new_hostname) {
    hostname = new_hostname;
  };

  /// Set a new port for the session
  void setPort(const uint16_t new_port) { port = new_port; }

public:
  /**
   * @brief Public function to set hostname and port number for the DCDB server
   *
   * @details This function calls privated functions @ref setHostname and @ref
   * setPort. Set values are used to connect to the DCDB server using @ref
   * DCDB_QDMI_Device_Session_impl_d::connect
   *
   * @param[in] baseUrl The base url to be set, i.e, example:8080
   */
  void setHostnameAndPort(const std::string &baseUrl);

  /// Set the username for authentication
  void setUsername(const std::string &new_username) { username = new_username; }

  /// Set the password for authentication
  void setPassword(const std::string &new_password) { password = new_password; }

  /// Get the underlying connection object
  DCDB::Connection *getConnection() { return connection; }

  /**
   * @brief Public function to connect to the DCDB server
   *
   * @details This function calls the privated function 
   * @ref DCDB_QDMI_Device_Session_impl_d::connectt with setted parameters using
   * the @ref DCDB_QDMI_device_session_set_parameter
   *
   * @return <a
   * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
   * if the connection established successfully.
   * @return <a
   * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
   * if
   *  - `connection` is not @c NULL,
   *  -  connection is failed to established.
   *
   */
  int connect() { return connect(hostname, port, username, password); }

  /// Disconnect and clean up the session
  void disconnect() { connection->disconnect(); }

  /// Update the session status
  void setStatus(DCDB_QDMI_DEVICE_SESSION_STATUS _status) { status = _status; }

  /// Retrieve the current session status
  DCDB_QDMI_DEVICE_SESSION_STATUS getStatus() { return status; }

  /// Destructor
  ~DCDB_QDMI_Device_Session_impl_d() { disconnect(); }
};
