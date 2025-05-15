#include "sensors.h"
#include "session.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <future>

#include "qdmi/constants.h"

#define ADD_STRING_PROPERTY(prop_name, prop_value, prop, size, value,          \
                            size_ret)                                          \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != NULL) {                                                   \
        if ((size) < strlen(prop_value) + 1) {                                 \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        strncpy((char *)(value), prop_value, (size)-1);                        \
        ((char *)(value))[(size)-1] = '\0';                                    \
      }                                                                        \
      if ((size_ret) != NULL) {                                                \
        *(size_ret) = strlen(prop_value) + 1;                                  \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

#define ADD_SINGLE_VALUE_PROPERTY(prop_name, prop_type, prop_value, prop,      \
                                  size, value, size_ret)                       \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != NULL) {                                                   \
        if ((size) < sizeof(prop_type)) {                                      \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        *(prop_type *)(value) = prop_value;                                    \
      }                                                                        \
      if ((size_ret) != NULL) {                                                \
        *(size_ret) = sizeof(prop_type);                                       \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

#define ADD_LIST_PROPERTY(prop_name, prop_type, prop_values, prop, size,       \
                          value, size_ret)                                     \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < (prop_values).size() * sizeof(prop_type)) {               \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        memcpy(static_cast<void *>(value),                                     \
               static_cast<const void *>((prop_values).data()),                \
               (prop_values).size() * sizeof(prop_type));                      \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = (prop_values).size() * sizeof(prop_type);                  \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }
struct DCDB_QDMI_Device_Job_impl_d {};

struct DCDB_QDMI_Device_Operation_impl_d {};

struct DCDB_QDMI_Device_Environment_Query_impl_d {
  uint64_t start_time, end_time;
  DCDB_QDMI_Environment environment{};
  size_t result_length{};
  QDMI_Environment_Query_Status status{};

  DCDB_QDMI_Device_Session_impl_t *session;

  std::list<DCDB::SensorDataStoreReading> results;

  std::future<void> async_query;
};

static QDMI_Device_Status *DCDB_QDMI_get_device_status(void) {
  static QDMI_Device_Status device_status = QDMI_DEVICE_STATUS_OFFLINE;
  return &device_status;
}

QDMI_Device_Status DCDB_QDMI_read_device_status(void) {
  return *DCDB_QDMI_get_device_status();
}

void DCDB_QDMI_set_device_status(QDMI_Device_Status status) {
  *DCDB_QDMI_get_device_status() = status;
}

int DCDB_QDMI_device_initialize() {
  DCDB_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  return QDMI_SUCCESS;
}

int DCDB_QDMI_device_finalize() {
  DCDB_QDMI_set_device_status(QDMI_DEVICE_STATUS_OFFLINE);
  return QDMI_SUCCESS;
}

int DCDB_QDMI_device_session_alloc(DCDB_QDMI_Device_Session *session) {
  *session = new DCDB_QDMI_Device_Session_impl_t();
  (*session)->setStatus(DCDB_QDMI_DEVICE_SESSION_STATUS::ALLOCATED);
  return QDMI_SUCCESS;
}

int DCDB_QDMI_device_session_init(DCDB_QDMI_Device_Session session) {
  return session->connect();
}

void DCDB_QDMI_device_session_free(DCDB_QDMI_Device_Session session) {
  delete session;
}

int DCDB_QDMI_device_session_set_parameter(
    DCDB_QDMI_Device_Session session, const QDMI_Device_Session_Parameter param,
    const size_t size, const void *value) {

  if (session == NULL || (value != NULL && size == 0) ||
      (param >= QDMI_DEVICE_SESSION_PARAMETER_MAX &&
       param != QDMI_DEVICE_SESSION_PARAMETER_BASEURL &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (session->getStatus() != DCDB_QDMI_DEVICE_SESSION_STATUS::ALLOCATED) {
    return QDMI_ERROR_BADSTATE;
  }

  if (param == QDMI_DEVICE_SESSION_PARAMETER_BASEURL)
    session->setHostnameAndPort((char *)value);
  else if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1)
    session->setUsername((char *)value);
  else if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2)
    session->setPassword((char *)value);

  return QDMI_SUCCESS;
}

int DCDB_QDMI_device_session_create_device_job(DCDB_QDMI_Device_Session session,
                                               DCDB_QDMI_Device_Job *job) {
  return QDMI_ERROR_NOTSUPPORTED;
}

void DCDB_QDMI_device_job_free(DCDB_QDMI_Device_Job job) {}

int DCDB_QDMI_device_job_set_parameter(DCDB_QDMI_Device_Job job,
                                       const QDMI_Device_Job_Parameter param,
                                       const size_t size, const void *value) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_job_submit(DCDB_QDMI_Device_Job job) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_job_cancel(DCDB_QDMI_Device_Job job) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_job_check(DCDB_QDMI_Device_Job job,
                               QDMI_Job_Status *status) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_job_wait(DCDB_QDMI_Device_Job job) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_job_get_results(DCDB_QDMI_Device_Job job,
                                     QDMI_Job_Result result, const size_t size,
                                     void *data, size_t *size_ret) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_session_query_device_property(
    DCDB_QDMI_Device_Session session, const QDMI_Device_Property prop,
    const size_t size, void *value, size_t *size_ret) {

  if (prop >= QDMI_DEVICE_PROPERTY_MAX || (value == NULL && size_ret == NULL)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_NAME, "DCDB", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_VERSION, "0.0.1", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "0.0.1", prop, size,
                      value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_STATUS, QDMI_Device_Status,
                            DCDB_QDMI_read_device_status(), prop, size, value,
                            size_ret)
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES,
                    DCDB_QDMI_Environment, DCDB_DEVICE_ENVIRONMENTS, prop, size,
                    value, size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_session_query_site_property(
    DCDB_QDMI_Device_Session session, DCDB_QDMI_Site site,
    const QDMI_Site_Property prop, const size_t size, void *value,
    size_t *size_ret) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_session_query_operation_property(
    DCDB_QDMI_Device_Session session, DCDB_QDMI_Operation operation,
    const size_t num_sites, const DCDB_QDMI_Site *sites,
    const size_t num_params, const double *params,
    const QDMI_Operation_Property prop, const size_t size, void *value,
    size_t *size_ret) {
  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_session_query_environment_property(
    DCDB_QDMI_Device_Session session, DCDB_QDMI_Environment environment,
    QDMI_Environment_Property prop, size_t size, void *value,
    size_t *size_ret) {

  if (session == nullptr || environment == nullptr ||
      (value != nullptr && size == 0) ||
      (prop >= QDMI_ENVIRONMENT_PROPERTY_MAX &&
       prop != QDMI_ENVIRONMENT_PROPERTY_CUSTOM1 &&
       prop != QDMI_ENVIRONMENT_PROPERTY_CUSTOM2 &&
       prop != QDMI_ENVIRONMENT_PROPERTY_CUSTOM3 &&
       prop != QDMI_ENVIRONMENT_PROPERTY_CUSTOM4 &&
       prop != QDMI_ENVIRONMENT_PROPERTY_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_STRING_PROPERTY(QDMI_ENVIRONMENT_PROPERTY_ID, environment->id.c_str(),
                      prop, size, value, size_ret)
  ADD_STRING_PROPERTY(QDMI_ENVIRONMENT_PROPERTY_UNIT, environment->unit.c_str(),
                      prop, size, value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_ENVIRONMENT_PROPERTY_SAMPLING_RATE, int,
                            environment->sampling_rate.count(), prop, size,
                            value, size_ret)
  return QDMI_ERROR_NOTSUPPORTED;
}

int DCDB_QDMI_device_session_create_environment_query(
    DCDB_QDMI_Device_Session session,
    DCDB_QDMI_Device_Environment_Query *query) {

  if (session == nullptr || query == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (session->getStatus() != DCDB_QDMI_DEVICE_SESSION_STATUS::INITIALIZED) {
    return QDMI_ERROR_BADSTATE;
  }
  *query = new DCDB_QDMI_Device_Environment_Query_impl_d();
  (*query)->environment = new DCDB_QDMI_Environment_impl_d();
  (*query)->start_time = 0;
  (*query)->end_time = 0;

  (*query)->session = session;
  return QDMI_SUCCESS;
}

int DCDB_QDMI_device_environment_query_set_parameter(
    DCDB_QDMI_Device_Environment_Query query,
    QDMI_Device_Environment_Query_Parameter param, size_t size,
    const void *value) {

  if (query == nullptr || (value != nullptr && size == 0) ||
      (param >= QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_MAX &&
       param != QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_CUSTOM2 &&
       param != QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_CUSTOM3 &&
       param != QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_CUSTOM4 &&
       param != QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  switch (param) {

  case QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_START_TIME: {
    query->start_time = *(uint64_t *)value;
    return QDMI_SUCCESS;
  }
  case QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_END_TIME: {
    query->end_time = *(uint64_t *)value;
    return QDMI_SUCCESS;
  }

  case QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_ENVIRONMENT: {
    const auto *environment_ptr =
        static_cast<const DCDB_QDMI_Environment *>(value);
    query->environment = *environment_ptr;
    return QDMI_SUCCESS;
  }
  default:
    return QDMI_ERROR_NOTSUPPORTED;
  }
}

void submit_query(DCDB_QDMI_Device_Environment_Query query) {
  query->status = QDMI_ENVIRONMENT_QUERY_STATUS_RUNNING;
  auto connection = query->session->getConnection();
  query->results =
      query->environment->query(connection, query->start_time, query->end_time);

  if (query->status != QDMI_ENVIRONMENT_QUERY_STATUS_CANCELED)
    query->status = QDMI_ENVIRONMENT_QUERY_STATUS_DONE;
}

int DCDB_QDMI_device_environment_query_submit(
    DCDB_QDMI_Device_Environment_Query query) {

  if (query == nullptr || query->environment == nullptr ||
      (query->start_time > query->end_time)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  query->status = QDMI_ENVIRONMENT_QUERY_STATUS_SUBMITTED;
  query->async_query = std::async(submit_query, query);

  return QDMI_SUCCESS;
}

int DCDB_QDMI_device_environment_query_get_results(
    DCDB_QDMI_Device_Environment_Query query,
    QDMI_Environment_Query_Result result, size_t size, void *data,
    size_t *size_ret) {

  if (query == nullptr || (data != nullptr && size == 0) ||
      (result >= QDMI_ENVIRONMENT_QUERY_RESULT_MAX &&
       result != QDMI_ENVIRONMENT_QUERY_RESULT_CUSTOM1 &&
       result != QDMI_ENVIRONMENT_QUERY_RESULT_CUSTOM2 &&
       result != QDMI_ENVIRONMENT_QUERY_RESULT_CUSTOM3 &&
       result != QDMI_ENVIRONMENT_QUERY_RESULT_CUSTOM4 &&
       result != QDMI_ENVIRONMENT_QUERY_RESULT_CUSTOM5) ||
      query->status != QDMI_ENVIRONMENT_QUERY_STATUS_DONE) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  size_t req_size = query->results.size();
  switch (result) {
  case QDMI_ENVIRONMENT_QUERY_RESULT_TIMESTAMPS:
    req_size *= sizeof(uint64_t);
    if (data != nullptr) {
      if (size < req_size) {
        return QDMI_ERROR_INVALIDARGUMENT;
      }
      uint64_t *data_ptr = static_cast<uint64_t *>(data);
      for (DCDB::SensorDataStoreReading reading : query->results) {
        *data_ptr++ = reading.timeStamp.getRaw();
      }
    }

    if ((size_ret) != nullptr) {
      *(size_ret) = req_size;
    }
    return QDMI_SUCCESS;
  case QDMI_ENVIRONMENT_QUERY_RESULT_VALUES:

    req_size *= sizeof(float);
    if (data != nullptr) {
      if (size < req_size) {
        return QDMI_ERROR_INVALIDARGUMENT;
      }
      int64_t *data_ptr = static_cast<int64_t *>(data);
      for (DCDB::SensorDataStoreReading reading : query->results)
        *data_ptr++ = reading.value;
    }
    if ((size_ret) != nullptr) {
      *(size_ret) = req_size;
    }
    return QDMI_SUCCESS;

  default:
    return QDMI_ERROR_NOTSUPPORTED;
  }

  return QDMI_SUCCESS;
}

int DCDB_QDMI_device_environment_query_check_status(
    DCDB_QDMI_Device_Environment_Query query,
    QDMI_Environment_Query_Status *status) {
  if (query == nullptr || status == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  *status = query->status;
  return QDMI_SUCCESS;
}

int DCDB_QDMI_device_environment_query_wait(
    DCDB_QDMI_Device_Environment_Query query) {

  if (query == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  query->async_query.wait();

  query->status = QDMI_ENVIRONMENT_QUERY_STATUS_DONE;
  return QDMI_SUCCESS;
}

int DCDB_QDMI_device_environment_query_cancel(
    DCDB_QDMI_Device_Environment_Query query) {

  if (query == nullptr || query->status == QDMI_ENVIRONMENT_QUERY_STATUS_DONE) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  query->status = QDMI_ENVIRONMENT_QUERY_STATUS_CANCELED;

  return QDMI_SUCCESS;
}

void DCDB_QDMI_device_environment_query_free(
    DCDB_QDMI_Device_Environment_Query query) {
  delete query;
}
