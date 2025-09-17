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

#include "session.h"
#include "dcdb/sensor.h"
#include "dcdb_qdmi/device.h"
#include "qdmi/constants.h"
#include <cassandra.h>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <string>

#include <dcdb/sensor.h>

void DCDB_QDMI_Device_Session_impl_d::setHostnameAndPort(
    const std::string &baseUrl) {
  size_t index = 0;
  for (char _char : baseUrl) {
    if (_char == ':')
      break;
    index++;
  }
  std::string url = baseUrl.substr(0, index);
  setHostname(url);

  if (index != baseUrl.size()) {
    std::string port = baseUrl.substr(index + 1, baseUrl.size());
    setPort(stoi(port));
  }
}

int DCDB_QDMI_Device_Session_impl_d::connect(std::string hostname,
                                             uint16_t port,
                                             std::string username,
                                             std::string password) {

  if (connection != nullptr) {
    return QDMI_ERROR_FATAL;
  }
  connection = new DCDB::Connection(hostname, port, username, password);

  if (!connection->connect()) {
    return QDMI_ERROR_FATAL;
  }

  setStatus(DCDB_QDMI_DEVICE_SESSION_STATUS::INITIALIZED);

  return QDMI_SUCCESS;
}
