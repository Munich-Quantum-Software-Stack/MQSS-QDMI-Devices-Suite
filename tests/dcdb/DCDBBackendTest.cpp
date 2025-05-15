#include "dcdb_qdmi/device.h"
#include "dcdb_qdmi/types.h"
#include "qdmi/constants.h"
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <vector>

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
      std::cout << msg << std::endl;                                           \
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
  ASSERT_EQ(DCDB_QDMI_device_job_wait(job), QDMI_ERROR_NOTSUPPORTED);
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
  ASSERT_EQ(DCDB_QDMI_device_job_wait(job), QDMI_ERROR_NOTSUPPORTED);
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
  ASSERT_EQ(
      DCDB_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_STATUS, sizeof(QDMI_Device_Status), static_cast<void *>(&device_status), nullptr),
      QDMI_SUCCESS);
      
  ASSERT_EQ(device_status, QDMI_DEVICE_STATUS_IDLE);
}


TEST_F(QDMIImplementationTest, QueryEnvironmentPropertySupported) {

  size_t size = 0;
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, 0, nullptr,
                &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";
  std::vector<DCDB_QDMI_Environment> envs(size / sizeof(DCDB_QDMI_Environment));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";

  for (auto env : envs) {
    size_t id_size;
    ASSERT_EQ(
        DCDB_QDMI_device_session_query_environment_property(
            session, env, QDMI_ENVIRONMENT_PROPERTY_ID, 0, nullptr, &id_size),
        QDMI_SUCCESS);

    std::string id(id_size - 1, '\0');
    ASSERT_EQ(DCDB_QDMI_device_session_query_environment_property(
                  session, env, QDMI_ENVIRONMENT_PROPERTY_ID, id_size,
                  static_cast<void *>(id.data()), nullptr),
              QDMI_SUCCESS);
  }
}

TEST_F(QDMIImplementationTest, QueryEnvironmentQueryPropertiesID) {

  size_t size = 0;
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, 0, nullptr,
                &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";
  std::vector<DCDB_QDMI_Environment> envs(size / sizeof(DCDB_QDMI_Environment));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";

  for (auto env : envs) {
    size_t id_size;
    ASSERT_EQ(
        DCDB_QDMI_device_session_query_environment_property(
            session, env, QDMI_ENVIRONMENT_PROPERTY_ID, 0, nullptr, &id_size),
        QDMI_SUCCESS);

    std::string id(id_size - 1, '\0');
    ASSERT_EQ(DCDB_QDMI_device_session_query_environment_property(
                  session, env, QDMI_ENVIRONMENT_PROPERTY_ID, id_size,
                  static_cast<void *>(id.data()), nullptr),
              QDMI_SUCCESS);
  }
}

TEST_F(QDMIImplementationTest, QueryEnvironmentQueryPropertiesUnit) {

  size_t size = 0;
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, 0, nullptr,
                &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";
  std::vector<DCDB_QDMI_Environment> envs(size / sizeof(DCDB_QDMI_Environment));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";

  for (auto env : envs) {
    size_t unit_size;
    ASSERT_EQ(DCDB_QDMI_device_session_query_environment_property(
                  session, env, QDMI_ENVIRONMENT_PROPERTY_UNIT, 0, nullptr,
                  &unit_size),
              QDMI_SUCCESS);

    std::string unit(unit_size - 1, '\0');
    ASSERT_EQ(DCDB_QDMI_device_session_query_environment_property(
                  session, env, QDMI_ENVIRONMENT_PROPERTY_UNIT, unit_size,
                  static_cast<void *>(unit.data()), nullptr),
              QDMI_SUCCESS);
  }
}

TEST_F(QDMIImplementationTest, QueryEnvironmentQueryPropertiesSamplingRate) {

  size_t size = 0;
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, 0, nullptr,
                &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";
  std::vector<DCDB_QDMI_Environment> envs(size / sizeof(DCDB_QDMI_Environment));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";

  for (auto env : envs) {
    size_t sampling_rate;
    ASSERT_EQ(DCDB_QDMI_device_session_query_environment_property(
                  session, env, QDMI_ENVIRONMENT_PROPERTY_SAMPLING_RATE,
                  sizeof(int), (void *)&sampling_rate, nullptr),
              QDMI_SUCCESS);
  }
}


TEST_F(QDMIImplementationTest, QueyJobAndCancel) {
  size_t size = 0;
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, 0, nullptr,
                &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";
  std::vector<DCDB_QDMI_Environment> envs(size / sizeof(DCDB_QDMI_Environment));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";

  DCDB_QDMI_Environment env = envs.at(0);

  DCDB_QDMI_Device_Environment_Query query;
  DCDB_QDMI_device_session_create_environment_query(session, &query);

  uint64_t start_ts = 1745644603;
  uint64_t end_ts = 1746817403;

  QDMI_Environment_Query_Status status;
  size_t result_size;

  ASSERT_EQ(DCDB_QDMI_device_environment_query_set_parameter(
                query, QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_START_TIME,
                sizeof(uint64_t), &start_ts),
            QDMI_SUCCESS);
  ASSERT_EQ(DCDB_QDMI_device_environment_query_set_parameter(
                query, QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_END_TIME,
                sizeof(uint64_t), &end_ts),
            QDMI_SUCCESS);
  ASSERT_EQ(DCDB_QDMI_device_environment_query_set_parameter(
                query,
                QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_ENVIRONMENT,
                sizeof(DCDB_QDMI_Environment), &env),
            QDMI_SUCCESS);

  ASSERT_EQ(DCDB_QDMI_device_environment_query_submit(query), QDMI_SUCCESS);

  ASSERT_EQ(DCDB_QDMI_device_environment_query_cancel(query), QDMI_SUCCESS);

  DCDB_QDMI_device_environment_query_check_status(query, &status);

  ASSERT_EQ(status, QDMI_ENVIRONMENT_QUERY_STATUS_CANCELED);

  DCDB_QDMI_device_environment_query_free(query);
}

TEST_F(QDMIImplementationTest, QueyJobAndGetResult) {
  size_t size = 0;
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, 0, nullptr,
                &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";
  std::vector<DCDB_QDMI_Environment> envs(size / sizeof(DCDB_QDMI_Environment));
  ASSERT_EQ(DCDB_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_ENVIRONMENTVARIABLES, size,
                static_cast<void *>(envs.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of environment variables";

  DCDB_QDMI_Environment env = envs.at(0);

  DCDB_QDMI_Device_Environment_Query query;
  DCDB_QDMI_device_session_create_environment_query(session, &query);

  uint64_t start_ts = 1745644603;
  uint64_t end_ts = 1745817403;

  QDMI_Environment_Query_Status status;
  size_t result_size;

  ASSERT_EQ(DCDB_QDMI_device_environment_query_set_parameter(
                query, QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_START_TIME,
                sizeof(uint64_t), &start_ts),
            QDMI_SUCCESS);
  ASSERT_EQ(DCDB_QDMI_device_environment_query_set_parameter(
                query, QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_END_TIME,
                sizeof(uint64_t), &end_ts),
            QDMI_SUCCESS);
  ASSERT_EQ(DCDB_QDMI_device_environment_query_set_parameter(
                query,
                QDMI_DEVICE_ENVIRONMENT_QUERY_PARAMETER_ENVIRONMENT,
                sizeof(DCDB_QDMI_Environment), &env),
            QDMI_SUCCESS);

  ASSERT_EQ(DCDB_QDMI_device_environment_query_submit(query), QDMI_SUCCESS);

  ASSERT_EQ(DCDB_QDMI_device_environment_query_wait(query), QDMI_SUCCESS);

  DCDB_QDMI_device_environment_query_check_status(query, &status);
  ASSERT_EQ(status, QDMI_ENVIRONMENT_QUERY_STATUS_DONE);

  ASSERT_EQ(DCDB_QDMI_device_environment_query_get_results(
                query, QDMI_ENVIRONMENT_QUERY_RESULT_TIMESTAMPS, 0, nullptr,
                &result_size),
            QDMI_SUCCESS);

  std::vector<uint64_t> timestamps(result_size / sizeof(uint64_t));
  ASSERT_EQ(DCDB_QDMI_device_environment_query_get_results(
                query, QDMI_ENVIRONMENT_QUERY_RESULT_TIMESTAMPS, result_size,
                static_cast<void *>(timestamps.data()), nullptr),
            QDMI_SUCCESS);



  ASSERT_EQ(DCDB_QDMI_device_environment_query_get_results(
    query, QDMI_ENVIRONMENT_QUERY_RESULT_VALUES, 0, nullptr,
    &result_size),
QDMI_SUCCESS);

std::vector<int64_t> values(result_size / sizeof(int64_t));
ASSERT_EQ(DCDB_QDMI_device_environment_query_get_results(
    query, QDMI_ENVIRONMENT_QUERY_RESULT_VALUES, result_size,
    static_cast<void *>(values.data()), nullptr),
QDMI_SUCCESS);

  DCDB_QDMI_device_environment_query_free(query);
}
