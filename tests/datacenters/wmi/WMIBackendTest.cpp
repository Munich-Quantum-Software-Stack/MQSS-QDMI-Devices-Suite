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

#include "wmi_qdmi/device.h"
#include <gtest/gtest.h>

#define WMI_HOST_URL "https://badwwmi-cloudapi.wmi.badw.de"

#define CHECK_DEVICE_STATUS(device_status, expected_value)                     \
  {                                                                            \
    ASSERT_EQ(WMI_QDMI_device_session_query_device_property(                   \
                  session, QDMI_DEVICE_PROPERTY_STATUS, sizeof(size_t),        \
                  device_status, nullptr),                                     \
              QDMI_SUCCESS);                                                   \
    ASSERT_TRUE((*device_status == expected_value));                           \
  }

#define CHECK_JOB_STATUS(job_status, expected_value)                           \
  {                                                                            \
    ASSERT_EQ(WMI_QDMI_device_job_check(job, job_status), QDMI_SUCCESS);       \
    ASSERT_TRUE(*job_status == expected_value);                                \
  }

#define CREATE_JOB(job, n_shot, format, program, sizebuffer)                               \
  {                                                                            \
    ASSERT_EQ(WMI_QDMI_device_session_create_device_job(session, &job),        \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(                                                                 \
        WMI_QDMI_device_job_set_parameter(                                     \
            job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(n_shot), &n_shot), \
        QDMI_SUCCESS);                                                         \
    ASSERT_EQ(WMI_QDMI_device_job_set_parameter(                               \
                  job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,                \
                  sizeof(format), &format),                                    \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(                                                                 \
        WMI_QDMI_device_job_set_parameter(                                     \
            job, QDMI_DEVICE_JOB_PARAMETER_PROGRAM, sizebuffer, program), \
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
  static WMI_QDMI_Device_Session session;
  static char *hostname;

  static void SetUpTestSuite() {
    int err;
    //hostname = std::getenv(WMI_HOST_URL);
    //if (!hostname) {
    //  std::cout << "Please provide a hostname by using environment variable "
    //               "WMI_HOST_URL."
    //            << std::endl;
    //  exit(1);
    //}

    EXIT_ON_FAIL(WMI_QDMI_device_initialize(),
                 "Failed to initialize the device")

    EXIT_ON_FAIL(WMI_QDMI_device_session_alloc(&session),
                 "Failed to allocate a session")

    //EXIT_ON_FAIL(WMI_QDMI_device_session_set_parameter(
    //                 session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
    //                 strlen(hostname) * sizeof(char), hostname),
    //             "Failed to set baseurl for the session")

    EXIT_ON_FAIL(
        WMI_QDMI_device_session_init(session),
        "Failed to initialize a session. Potential errors: Wrong or missing "
        "authentication information, or missing Python packages or device "
        "status is offline, or in "
        "maintenance. To provide credentials, take a look in " __FILE__
            << (__LINE__ - 4));
  }

  static void TearDownTestSuite() {
    WMI_QDMI_device_session_free(session);
    WMI_QDMI_device_finalize();
  }
};

WMI_QDMI_Device_Session QDMIImplementationTest::session = nullptr;
char *QDMIImplementationTest::hostname = nullptr;

TEST_F(QDMIImplementationTest, SessionSetParameterImplemented) {
  ASSERT_EQ(WMI_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(QDMIImplementationTest, SessionSetParameterAfterAllocated) {
  char dummy_hostname[] = "wmi.lrz.de";
  ASSERT_EQ(WMI_QDMI_device_session_set_parameter(
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
  WMI_QDMI_Device_Job job = nullptr;
  ASSERT_NE(WMI_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTIMPLEMENTED);
  WMI_QDMI_device_job_free(job);
}
TEST_F(QDMIImplementationTest, ControlSetParameterImplemented) {
  WMI_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(WMI_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(WMI_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  WMI_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSetShotParameterImplemented) {
  WMI_QDMI_Device_Job job = nullptr;
  int nShot = 1024;
  ASSERT_EQ(WMI_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(WMI_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(nShot), &nShot),
            QDMI_SUCCESS);
  WMI_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSetProgramFormatParameterImplemented) {
  WMI_QDMI_Device_Job job = nullptr;
  int nShot = 1024;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;

  ASSERT_EQ(WMI_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(WMI_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT, sizeof(qirFormat),
                &qirFormat),
            QDMI_SUCCESS);
  ASSERT_EQ(WMI_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,
                sizeof(qasmFormat), &qasmFormat),
            QDMI_ERROR_NOTSUPPORTED);
  WMI_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSubmitJobImplemented) {

  WMI_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(WMI_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(WMI_QDMI_device_job_submit(job), QDMI_ERROR_NOTIMPLEMENTED);
  ASSERT_NE(WMI_QDMI_device_job_wait(job, 0), QDMI_ERROR_NOTIMPLEMENTED);
  WMI_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlSubmitAndWaitJob) {
  WMI_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;

  char *CWD_path = getenv("CWD");
  char *circuit_path = NULL;
  asprintf(&circuit_path, "%s/config/circuit_excited.bc", CWD_path);
  char *buffer = 0;
  long length;
  FILE *f = fopen(circuit_path, "rb");
  fseek(f, 0, SEEK_END);
  size_t sizebuffer = (size_t)ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *program= (char*)malloc(sizebuffer);
  fread(program, 1, sizebuffer, f);
  fclose(f);

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));

  QDMI_Device_Status *device_status =
      (QDMI_Device_Status *)malloc(sizeof(QDMI_Device_Status));

  CREATE_JOB(job, nShot, qirFormat, program, sizebuffer);

  CHECK_DEVICE_STATUS(device_status, QDMI_DEVICE_STATUS_IDLE);
  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_CREATED);

  ASSERT_EQ(WMI_QDMI_device_job_submit(job), QDMI_SUCCESS);

  // Wait until running
  while (*job_status == QDMI_JOB_STATUS_SUBMITTED || *job_status == QDMI_JOB_STATUS_RUNNING )

    WMI_QDMI_device_job_check(job, job_status);


  ASSERT_EQ(WMI_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);

  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_DONE);

  WMI_QDMI_device_job_free(job);
  free(program);         
  free(job_status); 
  free(device_status);
}

TEST_F(QDMIImplementationTest, ControlCheckImplemented) {
  WMI_QDMI_Device_Job job = nullptr;
  QDMI_Job_Status status = QDMI_JOB_STATUS_RUNNING;

  ASSERT_EQ(WMI_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(WMI_QDMI_device_job_check(job, &status), QDMI_ERROR_NOTIMPLEMENTED);
  WMI_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlWaitImplemented) {
  WMI_QDMI_Device_Job job = nullptr;

  ASSERT_EQ(WMI_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(WMI_QDMI_device_job_wait(job, 0), QDMI_ERROR_NOTIMPLEMENTED);
  WMI_QDMI_device_job_free(job);
}

TEST_F(QDMIImplementationTest, ControlGetDataImplemented) {
  WMI_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(WMI_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(WMI_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_MAX, 0,
                                            nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  WMI_QDMI_device_job_free(job);
}


TEST_F(QDMIImplementationTest, ControlGetDataHistogramKeys) {
  WMI_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;

  char *CWD_path = getenv("CWD");
  char *circuit_path = NULL;
  asprintf(&circuit_path, "%s/config/circuit_hadamard_decomposed.bc", CWD_path);
  char *buffer = 0;
  long length;
  FILE *f = fopen(circuit_path, "rb");
  fseek(f, 0, SEEK_END);
  size_t sizebuffer = (size_t)ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *program= (char*)malloc(sizebuffer);
  fread(program, 1, sizebuffer, f);
  fclose(f);

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t histogram_size;
  char *histogram_keys;
  CREATE_JOB(job, nShot, qirFormat, program, sizebuffer);
  ASSERT_EQ(WMI_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(WMI_QDMI_device_job_wait(job, 100), QDMI_SUCCESS);

  ASSERT_EQ(WMI_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS, 0,
                                            nullptr, &histogram_size),
            QDMI_SUCCESS);

  histogram_keys = (char *)malloc(histogram_size);

  ASSERT_EQ(WMI_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS,
                                            histogram_size, histogram_keys,
                                            nullptr),
            QDMI_SUCCESS);

  WMI_QDMI_device_job_free(job);
  free(program);         
  free(job_status);   
}


TEST_F(QDMIImplementationTest, ControlGetDataHistogramValue) {
  WMI_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;

  char *CWD_path = getenv("CWD");
  char *circuit_path = NULL;
  asprintf(&circuit_path, "%s/config/circuit_hadamard_decomposed.bc", CWD_path);
  char *buffer = 0;
  long length;
  FILE *f = fopen(circuit_path, "rb");
  fseek(f, 0, SEEK_END);
  size_t sizebuffer = (size_t)ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *program= (char*)malloc(sizebuffer);
  fread(program, 1, sizebuffer, f);
  fclose(f);

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t histogram_values_size;
  int *histogram_values;
  CREATE_JOB(job, nShot, qirFormat, program, sizebuffer);
  ASSERT_EQ(WMI_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(WMI_QDMI_device_job_wait(job, 100), QDMI_SUCCESS);

  ASSERT_EQ(WMI_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES, 0,
                                            nullptr, &histogram_values_size),
            QDMI_SUCCESS);
  histogram_values = (int *)malloc(histogram_values_size);

  ASSERT_EQ(WMI_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES,
                                            histogram_values_size,
                                            histogram_values, nullptr),
            QDMI_SUCCESS);

  WMI_QDMI_device_job_free(job);
  free(program);         
  free(job_status); 
}


TEST_F(QDMIImplementationTest, ControlGetDataProbabilityKeys) {
  WMI_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_keys_size;
  char *probability_keys;

  char *CWD_path = getenv("CWD");
  char *circuit_path = NULL;
  asprintf(&circuit_path, "%s/config/circuit_hadamard_decomposed.bc", CWD_path);
  char *buffer = 0;
  long length;
  FILE *f = fopen(circuit_path, "rb");
  fseek(f, 0, SEEK_END);
  size_t sizebuffer = (size_t)ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *program= (char*)malloc(sizebuffer);
  fread(program, 1, sizebuffer, f);
  fclose(f);

  CREATE_JOB(job, nShot, qirFormat, program, sizebuffer);

  ASSERT_EQ(WMI_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(WMI_QDMI_device_job_wait(job, 100), QDMI_SUCCESS);

  ASSERT_EQ(WMI_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS, 0, nullptr,
                &probability_keys_size),
            QDMI_SUCCESS);
  probability_keys = (char *)malloc(probability_keys_size);

  ASSERT_EQ(WMI_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS,
                probability_keys_size, probability_keys, nullptr),
            QDMI_SUCCESS);

  WMI_QDMI_device_job_free(job);
  free(program);         
  free(job_status); 
}

TEST_F(QDMIImplementationTest, ControlGetDataProbabilityValues) {
  
  WMI_QDMI_Device_Job job = nullptr;
  size_t nShot = 1024;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_values_size;
  double *probability_values;

  char *CWD_path = getenv("CWD");
  char *circuit_path = NULL;
  asprintf(&circuit_path, "%s/config/circuit_hadamard_decomposed.bc", CWD_path);
  char *buffer = 0;
  long length;
  FILE *f = fopen(circuit_path, "rb");
  fseek(f, 0, SEEK_END);
  size_t sizebuffer = (size_t)ftell(f);
  fseek(f, 0, SEEK_SET);
  
  char *program= (char*)malloc(sizebuffer);
  fread(program, 1, sizebuffer, f);
  fclose(f);

  CREATE_JOB(job, nShot, qirFormat, program, sizebuffer);

  ASSERT_EQ(WMI_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(WMI_QDMI_device_job_wait(job, 100), QDMI_SUCCESS);

  ASSERT_EQ(WMI_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES, 0, nullptr,
                &probability_values_size),
            QDMI_SUCCESS);
  probability_values = (double *)malloc(probability_values_size);

  ASSERT_EQ(WMI_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES,
                probability_values_size, probability_values, nullptr),
            QDMI_SUCCESS);

  double sum = 0.0;
  size_t num_probs = probability_values_size / sizeof(double);
  for (size_t i = 0; i < num_probs; ++i) {
    sum += probability_values[i];
  }
  ASSERT_NEAR(sum,1.0, 1e-12);

  WMI_QDMI_device_job_free(job);
  free(program);         
  free(job_status); 
}

TEST_F(QDMIImplementationTest, QueryDevicePropertyImplemented) {

  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_F(QDMIImplementationTest, QuerySitePropertyImplemented) {

  ASSERT_EQ(WMI_QDMI_device_session_query_site_property(
                nullptr, nullptr, QDMI_SITE_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}
TEST_F(QDMIImplementationTest, QuerySitePropertyNotSupported) {

  size_t size = 0;
  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  std::vector<WMI_QDMI_Site> sites(size / sizeof(WMI_QDMI_Site));
  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, size,
                static_cast<void *>(sites.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";

  ASSERT_EQ(WMI_QDMI_device_session_query_site_property(
                session, sites.at(0), QDMI_SITE_PROPERTY_T1, sizeof(uint64_t),
                nullptr, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
}

TEST_F(QDMIImplementationTest, QueryOperationPropertyNotSupported) {
 
  size_t size = 0;
  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_OPERATIONS, 0, nullptr, &size),
            QDMI_SUCCESS);

  std::vector<WMI_QDMI_Operation> operations(size / sizeof(WMI_QDMI_Operation));
  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_OPERATIONS, size,
                static_cast<void *>(operations.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";

  ASSERT_EQ(WMI_QDMI_device_session_query_operation_property(
                session, operations.at(0), 0, nullptr, 0, nullptr,
                QDMI_OPERATION_PROPERTY_FIDELITY, 0, nullptr, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
}

TEST_F(QDMIImplementationTest, QueryDeviceNameImplemented) {
  size_t size = 0;

  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a name";
  std::string value(size - 1, '\0');
  ASSERT_EQ(
      WMI_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_NAME, size, value.data(), nullptr),
      QDMI_SUCCESS)
      << "Devices must provide a name";
  ASSERT_FALSE(value.empty()) << "Devices must provide a name";
}

TEST_F(QDMIImplementationTest, QueryDeviceVersionImplemented) {
  size_t size = 0;

  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_VERSION, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(
      WMI_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_VERSION, size, value.data(), nullptr),
      QDMI_SUCCESS)
      << "Devices must provide a version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a version";
}

TEST_F(QDMIImplementationTest, QueryDeviceLibraryVersionImplemented) {
  size_t size = 0;

  ASSERT_EQ(
      WMI_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a library version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, size,
                value.data(), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a library version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a library version";
}

TEST_F(QDMIImplementationTest, QuerySiteIDImplemented) {
  size_t size = 0;
  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  std::vector<WMI_QDMI_Site> sites(size / sizeof(WMI_QDMI_Site));
  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, size,
                static_cast<void *>(sites.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  size_t id = 0;
  for (auto *site : sites) {
    ASSERT_EQ(
        WMI_QDMI_device_session_query_site_property(
            session, site, QDMI_SITE_PROPERTY_INDEX, sizeof(size_t), &id, nullptr),
        QDMI_SUCCESS)
        << "Devices must provide a site id";
  }
}

TEST_F(QDMIImplementationTest, QubitNum) {
  size_t num_qubits = 0;
  ASSERT_EQ(WMI_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_QUBITSNUM, sizeof(size_t),
                &num_qubits, nullptr),
            QDMI_SUCCESS);
}
