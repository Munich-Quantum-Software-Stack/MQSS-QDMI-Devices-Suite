#include "qlm_qdmi/device.h"
#include <gtest/gtest.h>

#define CHECK_DEVICE_STATUS(device_status, expected_value)                     \
  {                                                                            \
    ASSERT_EQ(QLM_QDMI_device_session_query_device_property(                   \
                  session, QDMI_DEVICE_PROPERTY_STATUS, sizeof(size_t),        \
                  device_status, nullptr),                                     \
              QDMI_SUCCESS);                                                   \
    ASSERT_TRUE((*device_status == expected_value));                           \
  }

#define CHECK_JOB_STATUS(job_status, expected_value)                           \
  {                                                                            \
    ASSERT_EQ(QLM_QDMI_device_job_check(job, job_status), QDMI_SUCCESS);       \
    ASSERT_TRUE(*job_status == expected_value);                                \
  }

#define CREATE_JOB(job, n_shot, format, program)                               \
  {                                                                            \
    ASSERT_EQ(QLM_QDMI_device_session_create_device_job(session, &job),        \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(                                                                 \
        QLM_QDMI_device_job_set_parameter(                                     \
            job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(n_shot), &n_shot), \
        QDMI_SUCCESS);                                                         \
    ASSERT_EQ(QLM_QDMI_device_job_set_parameter(                               \
                  job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,                \
                  sizeof(format), &format),                                    \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(                                                                 \
        QLM_QDMI_device_job_set_parameter(                                     \
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
  static QLM_QDMI_Device_Session session;
  static char hostname[];

  static void SetUpTestSuite() {
    int err;

    EXIT_ON_FAIL(QLM_QDMI_device_initialize(),
                 "Failed to initialize the device")

    EXIT_ON_FAIL(QLM_QDMI_device_session_alloc(&session),
                 "Failed to allocate a session")

    EXIT_ON_FAIL(QLM_QDMI_device_session_set_parameter(
                     session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                     strlen(hostname) * sizeof(char), hostname),
                 "Failed to set baseurl for the session")

    EXIT_ON_FAIL(
        QLM_QDMI_device_session_init(session),
        "Failed to initialize a session. Potential errors: Wrong or missing "
        "authentication information, or missing Python packages or device "
        "status is offline, or in "
        "maintenance. To provide credentials, take a look in " __FILE__
            << (__LINE__ - 4));
  }

  static void TearDownTestSuite() {
    QLM_QDMI_device_session_free(session);
    QLM_QDMI_device_finalize();
  }
};

QLM_QDMI_Device_Session QDMIImplementationTest::session = nullptr;
char QDMIImplementationTest::hostname[] = "localhost:20501";

TEST_F(QDMIImplementationTest, SessionSetParameterImplemented) {
  ASSERT_EQ(QLM_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(QDMIImplementationTest, SessionSetParameterAfterAllocated) {
  char dummy_hostname[] = "qlm.lrz.de";
  ASSERT_EQ(QLM_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                sizeof(dummy_hostname), &dummy_hostname),
            QDMI_ERROR_BADSTATE);
}

namespace {
std::string Get_test_circuit() {
  return "OPENQASM 2.0;\n"
         "include \"qelib1.inc\";\n"
         "qreg q[2];\n"
         "creg c[2];\n"
         "h q[0];\n"
         "cx q[0], q[1];\n"
         "measure q -> c;\n";
}
} // namespace

TEST_F(QDMIImplementationTest, ControlCreateJobImplemented) {
  QLM_QDMI_Device_Job job = nullptr;
  ASSERT_NE(QLM_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTIMPLEMENTED);
  QLM_QDMI_device_job_free(job);
}
TEST_F(QDMIImplementationTest, ControlSetParameterImplemented) {
  QLM_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(QLM_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(QLM_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSetShotParameterImplemented) {
  QLM_QDMI_Device_Job job = nullptr;
  int nShot = 1024;
  ASSERT_EQ(QLM_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(QLM_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(nShot), &nShot),
            QDMI_SUCCESS);
  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSetProgramFormatParameterImplemented) {
  QLM_QDMI_Device_Job job = nullptr;
  int nShot = 1024;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;

  ASSERT_EQ(QLM_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(QLM_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT, sizeof(qirFormat),
                &qirFormat),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(QLM_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,
                sizeof(qasmFormat), &qasmFormat),
            QDMI_SUCCESS);
  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSubmitJobImplemented) {

  QLM_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(QLM_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(QLM_QDMI_device_job_submit(job), QDMI_ERROR_NOTIMPLEMENTED);
  ASSERT_NE(QLM_QDMI_device_job_wait(job), QDMI_ERROR_NOTIMPLEMENTED);
  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSubmitAndCancelJob) {

  QLM_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);

  ASSERT_EQ(QLM_QDMI_device_job_submit(job), QDMI_SUCCESS);

  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_SUBMITTED);

  ASSERT_EQ(QLM_QDMI_device_job_cancel(job), QDMI_SUCCESS);

  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_CANCELED);
  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSubmitAndWaitJob) {
  QLM_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));

  QDMI_Device_Status *device_status =
      (QDMI_Device_Status *)malloc(sizeof(QDMI_Device_Status));

  CREATE_JOB(job, nShot, qasmFormat, c_t_c);

  CHECK_DEVICE_STATUS(device_status, QDMI_DEVICE_STATUS_IDLE);
  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_CREATED);

  ASSERT_EQ(QLM_QDMI_device_job_submit(job), QDMI_SUCCESS);

  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_SUBMITTED);

  // Wait until running
  while (*job_status == QDMI_JOB_STATUS_SUBMITTED)
    QLM_QDMI_device_job_check(job, job_status);

  CHECK_DEVICE_STATUS(device_status, QDMI_DEVICE_STATUS_BUSY);

  ASSERT_EQ(QLM_QDMI_device_job_wait(job), QDMI_SUCCESS);

  CHECK_DEVICE_STATUS(device_status, QDMI_DEVICE_STATUS_IDLE);
  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_DONE);

  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlCancelImplemented) {
  QLM_QDMI_Device_Job job = nullptr;

  ASSERT_EQ(QLM_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(QLM_QDMI_device_job_cancel(job), QDMI_ERROR_NOTIMPLEMENTED);
  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlCheckImplemented) {
  QLM_QDMI_Device_Job job = nullptr;
  QDMI_Job_Status status = QDMI_JOB_STATUS_RUNNING;

  ASSERT_EQ(QLM_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(QLM_QDMI_device_job_check(job, &status), QDMI_ERROR_NOTIMPLEMENTED);
  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlWaitImplemented) {
  QLM_QDMI_Device_Job job = nullptr;

  ASSERT_EQ(QLM_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(QLM_QDMI_device_job_wait(job), QDMI_ERROR_NOTIMPLEMENTED);
  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataImplemented) {
  QLM_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(QLM_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(QLM_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_MAX, 0,
                                            nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataHistogramKeys) {
  QLM_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t histogram_size;
  char *histogram_keys;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(QLM_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(QLM_QDMI_device_job_wait(job), QDMI_SUCCESS);

  ASSERT_EQ(QLM_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS, 0,
                                            nullptr, &histogram_size),
            QDMI_SUCCESS);

  histogram_keys = (char *)malloc(histogram_size);

  ASSERT_EQ(QLM_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS,
                                            histogram_size, histogram_keys,
                                            nullptr),
            QDMI_SUCCESS);

  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataHistogramValue) {
  QLM_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t histogram_values_size;
  int *histogram_values;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(QLM_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(QLM_QDMI_device_job_wait(job), QDMI_SUCCESS);

  ASSERT_EQ(QLM_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES, 0,
                                            nullptr, &histogram_values_size),
            QDMI_SUCCESS);
  histogram_values = (int *)malloc(histogram_values_size);

  ASSERT_EQ(QLM_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES,
                                            histogram_values_size,
                                            histogram_values, nullptr),
            QDMI_SUCCESS);

  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataProbabilityKeys) {
  QLM_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_keys_size;
  char *probability_keys;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(QLM_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(QLM_QDMI_device_job_wait(job), QDMI_SUCCESS);

  ASSERT_EQ(QLM_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS, 0, nullptr,
                &probability_keys_size),
            QDMI_SUCCESS);
  probability_keys = (char *)malloc(probability_keys_size);

  ASSERT_EQ(QLM_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS,
                probability_keys_size, probability_keys, nullptr),
            QDMI_SUCCESS);

  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataProbabilityValues) {
  QLM_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_values_size;
  double *probability_values;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(QLM_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(QLM_QDMI_device_job_wait(job), QDMI_SUCCESS);

  ASSERT_EQ(QLM_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES, 0, nullptr,
                &probability_values_size),
            QDMI_SUCCESS);
  probability_values = (double *)malloc(probability_values_size);

  ASSERT_EQ(QLM_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES,
                probability_values_size, probability_values, nullptr),
            QDMI_SUCCESS);

  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataProbabilityDense) {
  QLM_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_dense_size;
  double *probability_dense;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(QLM_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(QLM_QDMI_device_job_wait(job), QDMI_SUCCESS);

  ASSERT_EQ(
      QLM_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_PROBABILITIES_DENSE,
                                      0, nullptr, &probability_dense_size),
      QDMI_SUCCESS);

  probability_dense = (double *)malloc(probability_dense_size);

  ASSERT_EQ(QLM_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_DENSE,
                probability_dense_size, probability_dense, nullptr),
            QDMI_SUCCESS);

  QLM_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, QueryDevicePropertyImplemented) {

  ASSERT_EQ(QLM_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(QDMIImplementationTest, QuerySitePropertyImplemented) {

  ASSERT_EQ(QLM_QDMI_device_session_query_site_property(
                nullptr, nullptr, QDMI_SITE_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}
TEST_F(QDMIImplementationTest, QuerySitePropertyNotSupported) {

  size_t size = 0;
  ASSERT_EQ(QLM_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  std::vector<QLM_QDMI_Site> sites(size / sizeof(QLM_QDMI_Site));
  ASSERT_EQ(QLM_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, size,
                static_cast<void *>(sites.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";

  ASSERT_EQ(QLM_QDMI_device_session_query_site_property(
                session, sites.at(0), QDMI_SITE_PROPERTY_T1, sizeof(uint64_t),
                nullptr, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
}

TEST_F(QDMIImplementationTest, QueryOperationPropertyNotSupported) {
  // Since it is a emulator
  ASSERT_EQ(QLM_QDMI_device_session_query_operation_property(
                session, nullptr, 0, nullptr, 0, nullptr,
                QDMI_OPERATION_PROPERTY_FIDELITY, 0, nullptr, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
}

TEST_F(QDMIImplementationTest, QueryDeviceNameImplemented) {
  size_t size = 0;

  ASSERT_EQ(QLM_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a name";
  std::string value(size - 1, '\0');
  ASSERT_EQ(
      QLM_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_NAME, size, value.data(), nullptr),
      QDMI_SUCCESS)
      << "Devices must provide a name";
  ASSERT_FALSE(value.empty()) << "Devices must provide a name";
}

TEST_F(QDMIImplementationTest, QueryDeviceVersionImplemented) {
  size_t size = 0;

  ASSERT_EQ(QLM_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_VERSION, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(
      QLM_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_VERSION, size, value.data(), nullptr),
      QDMI_SUCCESS)
      << "Devices must provide a version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a version";
}

TEST_F(QDMIImplementationTest, QueryDeviceLibraryVersionImplemented) {
  size_t size = 0;

  ASSERT_EQ(
      QLM_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a library version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(QLM_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, size,
                value.data(), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a library version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a library version";
}

TEST_F(QDMIImplementationTest, QuerySiteIDImplemented) {
  size_t size = 0;
  ASSERT_EQ(QLM_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  std::vector<QLM_QDMI_Site> sites(size / sizeof(QLM_QDMI_Site));
  ASSERT_EQ(QLM_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, size,
                static_cast<void *>(sites.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  size_t id = 0;
  for (auto *site : sites) {
    ASSERT_EQ(
        QLM_QDMI_device_session_query_site_property(
            session, site, QDMI_SITE_PROPERTY_ID, sizeof(size_t), &id, nullptr),
        QDMI_SUCCESS)
        << "Devices must provide a site id";
  }
}

TEST_F(QDMIImplementationTest, QubitNum) {
  size_t num_qubits = 0;
  ASSERT_EQ(QLM_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_QUBITSNUM, sizeof(size_t),
                &num_qubits, nullptr),
            QDMI_SUCCESS);
}
