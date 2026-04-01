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
#include "dcdb_qdmi/types.h"
#include "qdmi/constants.h"
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <vector>
#include <thread>

#define DCDB_HOST_URL "DCDB_HOST_URL"

#define CHECK_DEVICE_STATUS(device_status, expected_value)                     \
  {                                                                            \
    ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(                  \
                  session, QDMI_DEVICE_PROPERTY_STATUS, sizeof(size_t),        \
                  device_status, nullptr),                                     \
              QDMI_SUCCESS);                                                   \
    ASSERT_TRUE((*device_status == expected_value));                           \
  }

#define CHECK_JOB_STATUS(job_status, expected_value)                           \
  {                                                                            \
    ASSERT_EQ(DCDB_QDMI_device_job_check(job, job_status), QDMI_SUCCESS);      \
    ASSERT_TRUE(*job_status == expected_value);                                \
  }

#define CREATE_JOB(job, n_shot, format, program)                               \
  {                                                                            \
    ASSERT_EQ(DCDB_QDMI_device_session_create_device_job(session, &job),       \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(                                                                 \
        DCDB_QDMI_device_job_set_parameter(                                    \
            job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(n_shot), &n_shot), \
        QDMI_SUCCESS);                                                         \
    ASSERT_EQ(DCDB_QDMI_device_job_set_parameter(                              \
                  job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,                \
                  sizeof(format), &format),                                    \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(                                                                 \
        DCDB_QDMI_device_job_set_parameter(                                    \
            job, QDMI_DEVICE_JOB_PARAMETER_PROGRAM, strlen(program), program), \
        QDMI_SUCCESS);                                                         \
  }

#define EXIT_ON_FAIL(func, msg)                                                \
  {                                                                            \
    err = func;                                                                \
    if (err != QDMI_SUCCESS) {                                                 \
      exit(err);                                                               \
    }                                                                          \
  }

class QDMIImplementationTest : public ::testing::Test {
private:
protected:
  static DCDB_QDMI_Device_Session session;
  static char *hostname;

  static void SetUpTestSuite() {
    int err;
    // hostname = "std::getenv(DCDB_HOST_URL)";

    hostname = "host.docker.internal:9042";

    EXIT_ON_FAIL(DCDB_QDMI_device_initialize(),
                 "Failed to initialize the device")

    EXIT_ON_FAIL(DCDB_QDMI_device_session_alloc(&session),
                 "Failed to allocate a session")

    EXIT_ON_FAIL(DCDB_QDMI_device_session_set_parameter(
                     session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                     strlen(hostname) * sizeof(char), hostname),
                 "Failed to set baseurl for the session")

    EXIT_ON_FAIL(
        DCDB_QDMI_device_session_init(session),
        "Failed to initialize a session. Potential errors: Wrong or missing "
        "authentication information, or missing Python packages or device "
        "status is offline, or in "
        "maintenance. To provide credentials, take a look in " __FILE__
            << (__LINE__ - 4));
  }

  static void TearDownTestSuite() {
    DCDB_QDMI_device_session_free(session);
    DCDB_QDMI_device_finalize();
  }
};
DCDB_QDMI_Device_Session QDMIImplementationTest::session = nullptr;
char *QDMIImplementationTest::hostname = nullptr;

TEST_F(QDMIImplementationTest, SessionSetParameterImplemented) {
  ASSERT_EQ(DCDB_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(QDMIImplementationTest, SessionSetParameterAfterAllocated) {
  char dummy_hostname[] = "dcdb.lrz.de";
  ASSERT_EQ(DCDB_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                sizeof(dummy_hostname), &dummy_hostname),
            QDMI_ERROR_BADSTATE);
}

TEST_F(QDMIImplementationTest, ControlCreateJobImplemented) {
  DCDB_QDMI_Device_Job job = nullptr;
  ASSERT_NE(DCDB_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTIMPLEMENTED);
  DCDB_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSetParameterImplemented) {
  DCDB_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(DCDB_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
  DCDB_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSetShotParameterImplemented) {
  DCDB_QDMI_Device_Job job = nullptr;
  int nShot = 1024;
  ASSERT_EQ(DCDB_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(nShot), &nShot),
            QDMI_ERROR_NOTSUPPORTED);
  DCDB_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSetProgramFormatParameterImplemented) {
  DCDB_QDMI_Device_Job job = nullptr;
  int nShot = 1024;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;

  ASSERT_EQ(DCDB_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT, sizeof(qirFormat),
                &qirFormat),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,
                sizeof(qasmFormat), &qasmFormat),
            QDMI_ERROR_NOTSUPPORTED);
  DCDB_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSubmitJobImplemented) {

  DCDB_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(DCDB_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_submit(job), QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_wait(job, 0), QDMI_ERROR_NOTSUPPORTED);
  DCDB_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlCancelImplemented) {
  DCDB_QDMI_Device_Job job = nullptr;

  ASSERT_EQ(DCDB_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_cancel(job), QDMI_ERROR_NOTSUPPORTED);
  DCDB_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlCheckImplemented) {
  DCDB_QDMI_Device_Job job = nullptr;
  QDMI_Job_Status status = QDMI_JOB_STATUS_RUNNING;

  ASSERT_EQ(DCDB_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_check(job, &status), QDMI_ERROR_NOTSUPPORTED);
  DCDB_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlWaitImplemented) {
  DCDB_QDMI_Device_Job job = nullptr;

  ASSERT_EQ(DCDB_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_wait(job, 0), QDMI_ERROR_NOTSUPPORTED);
  DCDB_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataImplemented) {
  DCDB_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(DCDB_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(DCDB_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_MAX, 0,
                                             nullptr, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
  DCDB_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, QueryDevicePropertyImplemented) {

  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(QDMIImplementationTest, QuerySitePropertyImplemented) {

  ASSERT_EQ(DCDB_QDMI_device_session_query_site_property(
                nullptr, nullptr, QDMI_SITE_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
}

TEST_F(QDMIImplementationTest, QueryOperationPropertyNotSupported) {
  // Since it is a emulator
  ASSERT_EQ(DCDB_QDMI_device_session_query_operation_property(
                session, nullptr, 0, nullptr, 0, nullptr,
                QDMI_OPERATION_PROPERTY_FIDELITY, 0, nullptr, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
}

TEST_F(QDMIImplementationTest, QueryDeviceNameImplemented) {
  size_t size = 0;

  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a name";
  std::string value(size - 1, '\0');
  ASSERT_EQ(
      DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_NAME, size, value.data(), nullptr),
      QDMI_SUCCESS)
      << "Devices must provide a name";
  ASSERT_FALSE(value.empty()) << "Devices must provide a name";
}

TEST_F(QDMIImplementationTest, QueryDeviceVersionImplemented) {
  size_t size = 0;

  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_VERSION, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(
      DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_VERSION, size, value.data(), nullptr),
      QDMI_SUCCESS)
      << "Devices must provide a version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a version";
}

TEST_F(QDMIImplementationTest, QueryDeviceLibraryVersionImplemented) {
  size_t size = 0;

  ASSERT_EQ(
      DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a library version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, size,
                value.data(), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a library version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a library version";
}

TEST_F(QDMIImplementationTest, QueryDeviceStatusImplemented) {
  QDMI_Device_Status device_status;
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_STATUS,
                sizeof(QDMI_Device_Status), static_cast<void *>(&device_status),
                nullptr),
            QDMI_SUCCESS);

  ASSERT_EQ(device_status, QDMI_DEVICE_STATUS_IDLE);
}

TEST_F(QDMIImplementationTest, QueryTelemetryPropertySupported) {

  size_t size = 0;
  ASSERT_EQ(
      DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";
  std::vector<DCDB_QDMI_TelemetrySensor> envs(
      size / sizeof(DCDB_QDMI_TelemetrySensor));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";

  for (auto env : envs) {
    size_t id_size;
    ASSERT_EQ(DCDB_QDMI_device_session_query_telemetrysensor_property(
                  session, env, QDMI_TELEMETRYSENSOR_PROPERTY_ID, 0, nullptr,
                  &id_size),
              QDMI_SUCCESS);

    std::string id(id_size - 1, '\0');
    ASSERT_EQ(DCDB_QDMI_device_session_query_telemetrysensor_property(
                  session, env, QDMI_TELEMETRYSENSOR_PROPERTY_ID, id_size,
                  static_cast<void *>(id.data()), nullptr),
              QDMI_SUCCESS);
  }
}

TEST_F(QDMIImplementationTest, QueryTelemetryQueryPropertiesID) {

  size_t size = 0;
  ASSERT_EQ(
      DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";
  std::vector<DCDB_QDMI_TelemetrySensor> envs(
      size / sizeof(DCDB_QDMI_TelemetrySensor));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";

  for (auto env : envs) {
    size_t id_size;
    ASSERT_EQ(DCDB_QDMI_device_session_query_telemetrysensor_property(
                  session, env, QDMI_TELEMETRYSENSOR_PROPERTY_ID, 0, nullptr,
                  &id_size),
              QDMI_SUCCESS);

    std::string id(id_size - 1, '\0');
    ASSERT_EQ(DCDB_QDMI_device_session_query_telemetrysensor_property(
                  session, env, QDMI_TELEMETRYSENSOR_PROPERTY_ID, id_size,
                  static_cast<void *>(id.data()), nullptr),
              QDMI_SUCCESS);
  }
}

TEST_F(QDMIImplementationTest, QueryTelemetryQueryPropertiesUnit) {

  size_t size = 0;
  ASSERT_EQ(
      DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";
  std::vector<DCDB_QDMI_TelemetrySensor> envs(
      size / sizeof(DCDB_QDMI_TelemetrySensor));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";

  for (auto env : envs) {
    size_t unit_size;
    ASSERT_EQ(DCDB_QDMI_device_session_query_telemetrysensor_property(
                  session, env, QDMI_TELEMETRYSENSOR_PROPERTY_UNIT, 0, nullptr,
                  &unit_size),
              QDMI_SUCCESS);

    std::string unit(unit_size - 1, '\0');
    ASSERT_EQ(DCDB_QDMI_device_session_query_telemetrysensor_property(
                  session, env, QDMI_TELEMETRYSENSOR_PROPERTY_UNIT, unit_size,
                  static_cast<void *>(unit.data()), nullptr),
              QDMI_SUCCESS);
  }
}

TEST_F(QDMIImplementationTest, QueryTelemetryQueryPropertiesSamplingRate) {

  size_t size = 0;
  ASSERT_EQ(
      DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";
  std::vector<DCDB_QDMI_TelemetrySensor> envs(
      size / sizeof(DCDB_QDMI_TelemetrySensor));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";

  for (auto env : envs) {
    size_t sampling_rate;
    ASSERT_EQ(DCDB_QDMI_device_session_query_telemetrysensor_property(
                  session, env, QDMI_TELEMETRYSENSOR_PROPERTY_SAMPLINGRATE,
                  sizeof(int), (void *)&sampling_rate, nullptr),
              QDMI_SUCCESS);
  }
}

TEST_F(QDMIImplementationTest, QueyJobAndCancel) {
  size_t size = 0;
  ASSERT_EQ(
      DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";
  std::vector<DCDB_QDMI_TelemetrySensor> envs(
      size / sizeof(DCDB_QDMI_TelemetrySensor));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";

  DCDB_QDMI_TelemetrySensor env = envs.at(0);

  DCDB_QDMI_Device_TelemetrySensor_Query query;
  DCDB_QDMI_device_session_create_device_telemetrysensor_query(session, &query);

  uint64_t start_ts = 1745644603;
  uint64_t end_ts = 1746817403;

  QDMI_TelemetrySensor_Query_Status status;
  size_t result_size;

  ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_set_parameter(
                query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_STARTTIME,
                sizeof(uint64_t), &start_ts),
            QDMI_SUCCESS);
  ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_set_parameter(
                query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_ENDTIME,
                sizeof(uint64_t), &end_ts),
            QDMI_SUCCESS);
  ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_set_parameter(
                query,
                QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_TELEMETRYSENSOR,
                sizeof(DCDB_QDMI_TelemetrySensor), &env),
            QDMI_SUCCESS);

  ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_submit(query), QDMI_SUCCESS);

  ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_cancel(query), QDMI_SUCCESS);

  DCDB_QDMI_device_telemetrysensor_query_check_status(query, &status);

  ASSERT_EQ(status, QDMI_TELEMETRYSENSOR_QUERY_STATUS_CANCELED);

  DCDB_QDMI_device_telemetrysensor_query_free(query);
}

TEST_F(QDMIImplementationTest, QueyJobAndGetResult) {
  size_t size = 0;
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 0, nullptr, &size), QDMI_SUCCESS)
          << "Devices must provide a list of telemetry variables";
  
  std::vector<DCDB_QDMI_TelemetrySensor> envs(size / sizeof(DCDB_QDMI_TelemetrySensor));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, size,
          static_cast<void *>(envs.data()), nullptr), QDMI_SUCCESS)
      << "Devices must provide a list of telemetry variables";

  // DCDB_QDMI_TelemetrySensor env = envs.at(5);
  // DCDB_QDMI_Device_TelemetrySensor_Query query;
  // DCDB_QDMI_device_session_create_device_telemetrysensor_query(session, &query);

  // uint64_t start_ts = 1745644603;
  // uint64_t end_ts = 1745817403;

  // --- Dynamic timestamps: from 2 hours ago to now ---
  auto time_now   = std::chrono::system_clock::now();
  auto time_end    = time_now - std::chrono::hours(2);
  auto time_start  = time_end - std::chrono::minutes(5);

  uint64_t start_ts = static_cast<uint64_t>(std::chrono::system_clock::to_time_t(time_start));
  uint64_t end_ts = static_cast<uint64_t>(std::chrono::system_clock::to_time_t(time_end));

  // Print for verification
  std::time_t t_start = static_cast<std::time_t>(start_ts);
  std::time_t t_end   = static_cast<std::time_t>(end_ts);
  std::string str_start = std::ctime(&t_start);
  std::string str_end   = std::ctime(&t_end);
  if (!str_start.empty() && str_start.back() == '\n') str_start.pop_back();
  if (!str_end.empty()   && str_end.back()   == '\n') str_end.pop_back();
  std::cout << "Query Time Window:\n"
            << " + Start: " << str_start << " (" << start_ts << ")\n"
            << " + End:   " << str_end   << " (" << end_ts   << ")\n";

  // --- Loop over sensor IDs 0 to 4 ---
  for (int sensor_id = 0; sensor_id <= 4; ++sensor_id) {
    std::cout << "\n==========================================\n"
              << "Querying sensor ID: " << sensor_id
              << "\n==========================================\n";

    if (sensor_id >= static_cast<int>(envs.size())) {
        std::cerr << "Sensor ID " << sensor_id
                  << " out of range (envs.size()=" << envs.size() << "), skipping.\n";
        continue;
    }

    DCDB_QDMI_TelemetrySensor env = envs.at(sensor_id);

    // Fresh query handle for each sensor
    DCDB_QDMI_Device_TelemetrySensor_Query query;
    ASSERT_EQ(DCDB_QDMI_device_session_create_device_telemetrysensor_query(
            session, &query), QDMI_SUCCESS);

    QDMI_TelemetrySensor_Query_Status status;
    size_t result_size = 0;

    // Set parameters
    ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_set_parameter(
            query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_STARTTIME,
            sizeof(uint64_t), &start_ts), QDMI_SUCCESS);
    ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_set_parameter(
            query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_ENDTIME,
            sizeof(uint64_t), &end_ts), QDMI_SUCCESS);
    ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_set_parameter(
            query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_TELEMETRYSENSOR,
            sizeof(DCDB_QDMI_TelemetrySensor), &env), QDMI_SUCCESS);

    // Submit and wait
    ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_submit(query), QDMI_SUCCESS);
    ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_wait(query, 0), QDMI_SUCCESS);

    // Check status
    DCDB_QDMI_device_telemetrysensor_query_check_status(query, &status);
    ASSERT_EQ(status, QDMI_TELEMETRYSENSOR_QUERY_STATUS_DONE)
        << "Query failed for sensor_id=" << sensor_id;

    // Get timestamps
    ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_get_results(
            query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_TIMESTAMPS,
            0, nullptr, &result_size), QDMI_SUCCESS);

    std::vector<uint64_t> timestamps(result_size / sizeof(uint64_t));
    ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_get_results(
            query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_TIMESTAMPS,
            result_size, static_cast<void*>(timestamps.data()), nullptr), QDMI_SUCCESS);

    // Get values
    ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_get_results(
            query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_VALUES,
            0, nullptr, &result_size), QDMI_SUCCESS);

    std::vector<int64_t> values(result_size / sizeof(int64_t));
    ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_get_results(
            query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_VALUES,
            result_size, static_cast<void*>(values.data()), nullptr), QDMI_SUCCESS);

    // Free query handle before next iteration
    DCDB_QDMI_device_telemetrysensor_query_free(query);

    // Print results
    std::cout << "Timestamps (" << timestamps.size() << "): ";
    for (auto ts : timestamps)
        std::cout << ts << " ";
    std::cout << "\nValues (" << values.size() << "): ";
    for (auto v : values)
        std::cout << v << " ";
    std::cout << "\n";
  }
}

TEST_F(QDMIImplementationTest, QueryDataLoop) {
    size_t size = 0;
    ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
            session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 0, nullptr, &size), QDMI_SUCCESS)
            << "Devices must provide a list of telemetry variables";

    std::vector<DCDB_QDMI_TelemetrySensor> envs(size / sizeof(DCDB_QDMI_TelemetrySensor));
    ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
            session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, size,
            static_cast<void*>(envs.data()), nullptr), QDMI_SUCCESS)
        << "Devices must provide a list of telemetry variables";

    // --- Config ---
    const int sensor_id        = 0;   // sensor to query
    const int total_iterations = 5;   // number of loop iterations
    const int window_minutes   = 2;   // query window size in minutes
    const int sleep_minutes    = 2;   // sleep between iterations
    const int base_hours_ago   = 2;   // base offset from now

    ASSERT_LT(sensor_id, static_cast<int>(envs.size()))
        << "Sensor ID " << sensor_id << " out of range";

    DCDB_QDMI_TelemetrySensor env = envs.at(sensor_id);

    // --- Loop ---
    for (int iter = 0; iter < total_iterations; ++iter) {

        std::cout << "\n==========================================\n"
                  << "Iteration " << (iter + 1) << " / " << total_iterations
                  << "\n==========================================\n";

        // Each iteration shifts the window back by (iter * window_minutes)
        // so windows don't overlap:
        //   iter 0: [now-2h02m, now-2h00m]
        //   iter 1: [now-2h04m, now-2h02m]
        //   iter 2: [now-2h06m, now-2h04m]  etc.
        auto time_now   = std::chrono::system_clock::now();
        auto time_end   = time_now
                        - std::chrono::hours(base_hours_ago)
                        - std::chrono::minutes(iter * window_minutes);
        auto time_start = time_end - std::chrono::minutes(window_minutes);

        uint64_t start_ts = static_cast<uint64_t>(
            std::chrono::system_clock::to_time_t(time_start));
        uint64_t end_ts = static_cast<uint64_t>(
            std::chrono::system_clock::to_time_t(time_end));

        // Print time window
        std::time_t t_start = static_cast<std::time_t>(start_ts);
        std::time_t t_end   = static_cast<std::time_t>(end_ts);
        std::string str_start = std::ctime(&t_start);
        std::string str_end   = std::ctime(&t_end);
        if (!str_start.empty() && str_start.back() == '\n') str_start.pop_back();
        if (!str_end.empty()   && str_end.back()   == '\n') str_end.pop_back();
        std::cout << "Query Time Window:\n"
                  << " + Start: " << str_start << " (" << start_ts << ")\n"
                  << " + End:   " << str_end   << " (" << end_ts   << ")\n";

        // Fresh query handle for each iteration
        DCDB_QDMI_Device_TelemetrySensor_Query query;
        ASSERT_EQ(DCDB_QDMI_device_session_create_device_telemetrysensor_query(
                session, &query), QDMI_SUCCESS);

        // Set parameters
        ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_set_parameter(
                query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_STARTTIME,
                sizeof(uint64_t), &start_ts), QDMI_SUCCESS);
        ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_set_parameter(
                query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_ENDTIME,
                sizeof(uint64_t), &end_ts), QDMI_SUCCESS);
        ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_set_parameter(
                query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_TELEMETRYSENSOR,
                sizeof(DCDB_QDMI_TelemetrySensor), &env), QDMI_SUCCESS);

        // Submit and wait
        ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_submit(query), QDMI_SUCCESS);
        ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_wait(query, 0), QDMI_SUCCESS);

        // Check status
        QDMI_TelemetrySensor_Query_Status status;
        DCDB_QDMI_device_telemetrysensor_query_check_status(query, &status);
        EXPECT_EQ(status, QDMI_TELEMETRYSENSOR_QUERY_STATUS_DONE)
            << "Query failed at iteration " << iter;

        if (status == QDMI_TELEMETRYSENSOR_QUERY_STATUS_DONE) {

            // Get timestamps
            size_t result_size = 0;
            ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_get_results(
                    query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_TIMESTAMPS,
                    0, nullptr, &result_size), QDMI_SUCCESS);
            std::vector<uint64_t> timestamps(result_size / sizeof(uint64_t));
            ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_get_results(
                    query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_TIMESTAMPS,
                    result_size, static_cast<void*>(timestamps.data()), nullptr), QDMI_SUCCESS);

            // Get values
            ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_get_results(
                    query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_VALUES,
                    0, nullptr, &result_size), QDMI_SUCCESS);
            std::vector<int64_t> values(result_size / sizeof(int64_t));
            ASSERT_EQ(DCDB_QDMI_device_telemetrysensor_query_get_results(
                    query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_VALUES,
                    result_size, static_cast<void*>(values.data()), nullptr), QDMI_SUCCESS);

            // Print results
            std::cout << "Timestamps (" << timestamps.size() << "): ";
            for (auto ts : timestamps) std::cout << ts << " ";
            std::cout << "\nValues (" << values.size() << "): ";
            for (auto v  : values)    std::cout << v  << " ";
            std::cout << "\n";
        }

        // Free query handle
        DCDB_QDMI_device_telemetrysensor_query_free(query);

        // Sleep between iterations (skip after last one)
        if (iter < total_iterations - 1) {
            std::cout << "Sleeping " << sleep_minutes << " minutes...\n";
            std::this_thread::sleep_for(std::chrono::minutes(sleep_minutes));
        }
    }
}
