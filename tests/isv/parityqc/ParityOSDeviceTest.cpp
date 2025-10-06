// ParityQC © 2025. See LICENSE.txt in /src/isv/parityqc/ for details.

#include "parityos_qdmi/device.h"
#include "qdmi/constants.h"
#include <gtest/gtest.h>

class QDMIImplementationTest : public ::testing::Test {
private:
protected:
  static ParityOS_QDMI_Device_Session session;

  static void SetUpTestSuite() {
    const char *token = "foo";

    // This function *must* be called first (and exactly once):
    ASSERT_EQ(ParityOS_QDMI_device_initialize(), QDMI_SUCCESS)
        << "Failed to initialize device";

    // Any function using `session` must come after this function:
    ASSERT_EQ(ParityOS_QDMI_device_session_alloc(&session), QDMI_SUCCESS)
        << "Failed to allocate a session";

    // Use this function to supply all required parameters needed for
    // session_init.
    ASSERT_EQ(ParityOS_QDMI_device_session_set_parameter(
                  session, QDMI_DEVICE_SESSION_PARAMETER_TOKEN,
                  strlen(token) * sizeof(char), token),
              QDMI_SUCCESS)
        << "Failed to set authentication token";

    // This function has to be called before using the `session` with the device
    // query or device job interface.
    ASSERT_EQ(ParityOS_QDMI_device_session_init(session), QDMI_SUCCESS)
        << "Failed to initialize session";
  }

  static void TearDownTestSuite() {
    // Any function using `session` must come before this function:
    ParityOS_QDMI_device_session_free(session);
    // This function *must* be called last (and exactly once):
    ASSERT_EQ(ParityOS_QDMI_device_finalize(), QDMI_SUCCESS)
        << "Failed to finalize device.";
  }
};

ParityOS_QDMI_Device_Session QDMIImplementationTest::session = nullptr;

/// TODO: Below tests are mostly from the template folder of the QDMI repo.
/// Once our device is implemented make sure everything is properly tested.

TEST_F(QDMIImplementationTest, SessionSetParameterImplemented) {
  ASSERT_EQ(ParityOS_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(QDMIImplementationTest, JobCreateImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ParityOS_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, JobSetParameterImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  ASSERT_EQ(ParityOS_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);

  /// TODO: more checks (possibly in dedicated TEST)

  ParityOS_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, JobQueryPropertyImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  ASSERT_EQ(ParityOS_QDMI_device_job_query_property(
                job, QDMI_DEVICE_JOB_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);

  /// TODO: more checks (possibly in dedicated TEST)

  ParityOS_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, JobSubmitImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  /// Without program we cannot submit the job:
  ASSERT_EQ(ParityOS_QDMI_device_job_submit(job), QDMI_ERROR_INVALIDARGUMENT);

  ParityOS_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, JobCancelImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  ASSERT_EQ(ParityOS_QDMI_device_job_cancel(job), QDMI_SUCCESS);

  ParityOS_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, JobCheckImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  QDMI_Job_Status status = QDMI_JOB_STATUS_RUNNING;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  ASSERT_EQ(ParityOS_QDMI_device_job_check(job, &status), QDMI_SUCCESS);

  ParityOS_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, JobWaitImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  /// TODO: Right now not implemented
  ASSERT_EQ(ParityOS_QDMI_device_job_wait(job, 0), QDMI_ERROR_FATAL);

  ParityOS_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, JobGetResultsImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  ASSERT_EQ(ParityOS_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_MAX, 0,
                                                 nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);

  /// TODO: more checks (possibly in dedicated TEST)

  ParityOS_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, QueryDevicePropertyImplemented) {
  ASSERT_EQ(ParityOS_QDMI_device_session_query_device_property(
                nullptr, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);

  /// TODO: more checks (possibly in dedicated TEST)
}

TEST_F(QDMIImplementationTest, QuerySitePropertyImplemented) {
  ASSERT_EQ(ParityOS_QDMI_device_session_query_site_property(
                nullptr, nullptr, QDMI_SITE_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(QDMIImplementationTest, QueryOperationPropertyImplemented) {
  ASSERT_EQ(ParityOS_QDMI_device_session_query_operation_property(
                nullptr, nullptr, 0, nullptr, 0, nullptr,
                QDMI_OPERATION_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

struct DevicePropertyTestParam {
  QDMI_Device_Property property;
  const char *error_message;
};

class QDMIPropertyTest
    : public QDMIImplementationTest,
      public ::testing::WithParamInterface<DevicePropertyTestParam> {};

/// NOTE: This is written for string properties.
TEST_P(QDMIPropertyTest, QueryDevicePropertyImplemented) {
  const auto &param = GetParam();
  size_t size = 0;

  /// First we figure out the size of the value:
  ASSERT_EQ(ParityOS_QDMI_device_session_query_device_property(
                session, param.property, 0, nullptr, &size),
            QDMI_SUCCESS)
      << param.error_message;

  /// Here we allocate enough memory to hold the value. The size is reported
  /// including the null-terminator but this does not count into the length of a
  /// std::string! Hence we have to subtract 1.
  const char arbitrary_character = '\0';
  std::string value(size - 1, arbitrary_character);

  /// Now we actually retrieve the value:
  ASSERT_EQ(ParityOS_QDMI_device_session_query_device_property(
                session, param.property, size, value.data(), nullptr),
            QDMI_SUCCESS)
      << param.error_message;
  ASSERT_FALSE(value.empty()) << param.error_message;
}

INSTANTIATE_TEST_SUITE_P(
    QDMIProperties, QDMIPropertyTest,

    ::testing::Values(
        // clang-format off
  DevicePropertyTestParam{QDMI_DEVICE_PROPERTY_NAME,"devices must provide a name"},
  DevicePropertyTestParam{QDMI_DEVICE_PROPERTY_VERSION,"devices must provide a version"},
  DevicePropertyTestParam{QDMI_DEVICE_PROPERTY_LIBRARYVERSION,"devices must provide a library version"}
        // clang-format on
        ));

TEST_F(QDMIPropertyTest, QueryDeviceStatusPropertyImplemented) {
  QDMI_Device_Status status = QDMI_DEVICE_STATUS_MAX;
  size_t size = sizeof(QDMI_Device_Status);

  ASSERT_EQ(ParityOS_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_STATUS, size, &status, nullptr),
            QDMI_SUCCESS)
      << "devices must provide a status";

  ASSERT_EQ(status, QDMI_DEVICE_STATUS_IDLE) << "unexpected status";
}
