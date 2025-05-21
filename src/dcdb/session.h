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

enum class DCDB_QDMI_DEVICE_SESSION_STATUS : uint8_t { ALLOCATED, INITIALIZED };

typedef struct DCDB_QDMI_Device_Session_impl_d {
private:
  std::string hostname = "stor";
  uint16_t port = 9042;
  std::string username = "";
  std::string password = "";
  DCDB::Connection *connection = nullptr;

  int connect(std::string hostname, uint16_t port, std::string username,
              std::string password);

  DCDB_QDMI_DEVICE_SESSION_STATUS status;
  void setHostname(const std::string &new_hostname);
  void setPort(const uint16_t port);

public:
  void setHostnameAndPort(const std::string &baseUrl);
  void setUsername(const std::string username);
  void setPassword(const std::string password);

  DCDB::Connection *getConnection() { return connection; }

  int connect();
  void disconnect();

  void setStatus(DCDB_QDMI_DEVICE_SESSION_STATUS _status) { status = _status; }
  DCDB_QDMI_DEVICE_SESSION_STATUS getStatus() { return status; }
  ~DCDB_QDMI_Device_Session_impl_d();

} DCDB_QDMI_Device_Session_impl_t;
