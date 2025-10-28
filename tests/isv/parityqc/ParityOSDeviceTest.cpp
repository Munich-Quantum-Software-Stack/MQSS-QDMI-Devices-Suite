// ParityQC © 2025. See LICENSE.txt in /src/isv/parityqc/ for details.

#include "parityos_qdmi/device.h"
#include "qdmi/constants.h"
#include <cstdlib>
#include <gtest/gtest.h>

/**
 * In order to run the test you should run a local installation of parityapi
 * with specific user `testuser`. Certain environment variables should be set.
 * Run the tests to see which ones.
 */

//===----------------------------------------------------------------------===//
// Base: Setup and tear down device globally for all test cases.
//===----------------------------------------------------------------------===//

class ParityOSDeviceTest : public ::testing::Test {
private:
  static void assert_parityos_pass_is_set_or_exit(const char *username) {
    auto pass_var = "PARITYOS_PASS";

    auto pass = std::getenv(pass_var);
    if (!pass) {
      std::cerr << "\nERROR: Please export `" << pass_var
                << "` into your environment. It should contain the parityos "
                   "password of the user `"
                << username << "` which is used for testing." << std::endl;
      exit(1);
    }
  }

  /// Base URL of the Parity API (you should use a local version).
  static const char *get_base_url_or_exit() {
    auto baseurl_var = "PARITYQC_PARITYOS_BASEURL";

    auto baseurl = std::getenv(baseurl_var);
    if (!baseurl) {
      std::cerr << "\nERROR: Please export `" << baseurl_var
                << "` into your environment." << std::endl;
      exit(1);
    }

    return baseurl;
  }

protected:
  static ParityOS_QDMI_Device_Session session;

  static void SetUpTestSuite() {
    const char *base_url = get_base_url_or_exit();
    const char *username = "testuser";

    assert_parityos_pass_is_set_or_exit(username);

    // This function *must* be called first (and exactly once):
    ASSERT_EQ(ParityOS_QDMI_device_initialize(), QDMI_SUCCESS)
        << "Failed to initialize device";

    // Any function using `session` must come after this function:
    ASSERT_EQ(ParityOS_QDMI_device_session_alloc(&session), QDMI_SUCCESS)
        << "Failed to allocate a session";

    // Use this function to supply all required parameters needed for
    // session_init.
    ASSERT_EQ(ParityOS_QDMI_device_session_set_parameter(
                  session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                  strlen(base_url) * sizeof(char), base_url),
              QDMI_SUCCESS)
        << "Failed to set base url";

    ASSERT_EQ(ParityOS_QDMI_device_session_set_parameter(
                  session, QDMI_DEVICE_SESSION_PARAMETER_USERNAME,
                  strlen(username) * sizeof(char), username),
              QDMI_SUCCESS)
        << "Failed to set username";

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

ParityOS_QDMI_Device_Session ParityOSDeviceTest::session = nullptr;

//===----------------------------------------------------------------------===//
// Misc tests
//===----------------------------------------------------------------------===//

/// TODO: Below tests are mostly from the template folder of the QDMI repo.
/// Once our device is implemented make sure everything is properly tested.

TEST_F(ParityOSDeviceTest, SessionSetParameterImplemented) {
  ASSERT_EQ(ParityOS_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(ParityOSDeviceTest, JobCreateImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ParityOS_QDMI_device_job_free(job);
}

TEST_F(ParityOSDeviceTest, JobSetParameterImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  ASSERT_EQ(ParityOS_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);

  /// TODO: more checks (possibly in dedicated TEST)

  ParityOS_QDMI_device_job_free(job);
}

TEST_F(ParityOSDeviceTest, JobQueryPropertyImplemented) {
  ParityOS_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  ASSERT_EQ(ParityOS_QDMI_device_job_query_property(
                job, QDMI_DEVICE_JOB_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);

  /// TODO: more checks (maybe in dedicated TEST)

  ParityOS_QDMI_device_job_free(job);
}

//===----------------------------------------------------------------------===//
// Job submission and post processing
//===----------------------------------------------------------------------===//

class JobTest : public ParityOSDeviceTest {
protected:
  static const std::string program;
  static const std::string result;

  ParityOS_QDMI_Device_Job job = nullptr;

  void SetUp() override {
    ASSERT_EQ(ParityOS_QDMI_device_session_create_device_job(session, &job),
              QDMI_SUCCESS);

    const auto format = QDMI_PROGRAM_FORMAT_CUSTOM1;
    ASSERT_EQ(ParityOS_QDMI_device_job_set_parameter(
                  job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT, sizeof(format),
                  &format),
              QDMI_SUCCESS);

    ASSERT_EQ(ParityOS_QDMI_device_job_set_parameter(
                  job, QDMI_DEVICE_JOB_PARAMETER_PROGRAM, program.size(),
                  program.data()),
              QDMI_SUCCESS);

    ASSERT_EQ(ParityOS_QDMI_device_job_submit(job), QDMI_SUCCESS);
  }

  void TearDown() override { ParityOS_QDMI_device_job_free(job); }
};

const std::string JobTest::program = "{ \"content\": \"hello\" }";
const std::string JobTest::result = "is english";

TEST_F(JobTest, Cancel) {
  ASSERT_EQ(ParityOS_QDMI_device_job_cancel(nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  ASSERT_EQ(ParityOS_QDMI_device_job_cancel(job), QDMI_ERROR_NOTIMPLEMENTED);
}

TEST_F(JobTest, Check) {
  QDMI_Job_Status status = QDMI_JOB_STATUS_RUNNING;
  ASSERT_EQ(ParityOS_QDMI_device_job_check(nullptr, &status),
            QDMI_ERROR_INVALIDARGUMENT);
  ASSERT_EQ(ParityOS_QDMI_device_job_check(job, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  ASSERT_EQ(ParityOS_QDMI_device_job_check(job, &status),
            QDMI_ERROR_NOTIMPLEMENTED);
}

TEST_F(JobTest, Wait) {
  /// Tests also that it works a second time:
  ASSERT_EQ(ParityOS_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);
  ASSERT_EQ(ParityOS_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);
}

TEST_F(JobTest, GetResults) {
  /// You first have to wait so that the job transitions into the DONE status.
  ASSERT_EQ(ParityOS_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);

  ASSERT_EQ(ParityOS_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_MAX, 0,
                                                 nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);

  size_t size_ret;
  ASSERT_EQ(ParityOS_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_CUSTOM1,
                                                 0, nullptr, &size_ret),
            QDMI_SUCCESS);
  ASSERT_EQ(size_ret, result.size() + 1); // including \0
  std::string data(size_ret - 1, '\0');
  ASSERT_EQ(ParityOS_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_CUSTOM1, size_ret, data.data(), nullptr),
            QDMI_SUCCESS);
  ASSERT_EQ(data, result);
}

//===----------------------------------------------------------------------===//
// Query tests
//===----------------------------------------------------------------------===//

TEST_F(ParityOSDeviceTest, QueryDevicePropertyImplemented) {
  ASSERT_EQ(ParityOS_QDMI_device_session_query_device_property(
                nullptr, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);

  /// TODO: more checks (possibly in dedicated TEST)
}

TEST_F(ParityOSDeviceTest, QuerySitePropertyImplemented) {
  ASSERT_EQ(ParityOS_QDMI_device_session_query_site_property(
                nullptr, nullptr, QDMI_SITE_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(ParityOSDeviceTest, QueryOperationPropertyImplemented) {
  ASSERT_EQ(ParityOS_QDMI_device_session_query_operation_property(
                nullptr, nullptr, 0, nullptr, 0, nullptr,
                QDMI_OPERATION_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

struct DevicePropertyTestParam {
  QDMI_Device_Property property;
  const char *error_message;
};

class PropertyTest
    : public ParityOSDeviceTest,
      public ::testing::WithParamInterface<DevicePropertyTestParam> {};

/// NOTE: This is written for string properties.
TEST_P(PropertyTest, QueryDevicePropertyImplemented) {
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
    QDMIProperties, PropertyTest,

    ::testing::Values(
        // clang-format off
  DevicePropertyTestParam{QDMI_DEVICE_PROPERTY_NAME,"devices must provide a name"},
  DevicePropertyTestParam{QDMI_DEVICE_PROPERTY_VERSION,"devices must provide a version"},
  DevicePropertyTestParam{QDMI_DEVICE_PROPERTY_LIBRARYVERSION,"devices must provide a library version"}
        // clang-format on
        ));
