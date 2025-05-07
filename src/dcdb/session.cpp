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

int DCDB_QDMI_Device_Session_impl_d::connect() {
  return connect(hostname, port, username, password);
}
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

void DCDB_QDMI_Device_Session_impl_d::setHostname(
    const std::string &new_hostname) {
  hostname = new_hostname;
}

void DCDB_QDMI_Device_Session_impl_d::setPort(uint16_t new_port) {
  port = new_port;
}

void DCDB_QDMI_Device_Session_impl_d::setUsername(std::string _username) {
  username = _username;
}

void DCDB_QDMI_Device_Session_impl_d::setPassword(std::string _password) {
  password = _password;
}
DCDB_QDMI_Device_Session_impl_d::~DCDB_QDMI_Device_Session_impl_d() {
  disconnect();
}

void DCDB_QDMI_Device_Session_impl_d::disconnect() { connection->disconnect(); }

int DCDB_QDMI_Device_Session_impl_d::connect(std::string hostname,
                                             uint16_t port,
                                             std::string username,
                                             std::string password) {

  if (connection != nullptr) {
    return 0;
  }
  connection = new DCDB::Connection(hostname, port, username, password);

  if (!connection->connect()) {
    return QDMI_ERROR_FATAL;
  }

  setStatus(DCDB_QDMI_DEVICE_SESSION_STATUS::INITIALIZED);

  return QDMI_SUCCESS;
}
