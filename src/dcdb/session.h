
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