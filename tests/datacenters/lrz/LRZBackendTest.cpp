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

#include "lrz_qdmi/device.h"
#include "qdmi/constants.h"
#include "gtest/gtest.h"
#include <bits/stdc++.h>
#include <cstddef>
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#define LRZ_HOST_URL "LRZ_HOST_URL"
#define MQP_SECRET_TOKEN "MQP_SECRET_TOKEN"

#define EXIT_ON_FAIL(func, msg)                                                \
  {                                                                            \
    err = func;                                                                \
    if (err != QDMI_SUCCESS) {                                                 \
      std::cout << msg << std::endl;                                           \
      exit(err);                                                               \
    }                                                                          \
  }

#define CHECK_DEVICE_STATUS(device_status, expected_value)                     \
  {                                                                            \
    ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(                   \
                  session, QDMI_DEVICE_PROPERTY_STATUS, sizeof(size_t),        \
                  device_status, nullptr),                                     \
              QDMI_SUCCESS);                                                   \
    ASSERT_TRUE((*device_status == expected_value));                           \
  }
#define CHECK_JOB_STATUS(job_status, expected_value)                           \
  {                                                                            \
    ASSERT_EQ(LRZ_QDMI_device_job_check(job, job_status), QDMI_SUCCESS);       \
    ASSERT_TRUE(*job_status == expected_value);                                \
  }

#define CREATE_JOB(job, n_shot, format, program)                               \
  {                                                                            \
    ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(session, &job),        \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(                                                                 \
        LRZ_QDMI_device_job_set_parameter(                                     \
            job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(n_shot), &n_shot), \
        QDMI_SUCCESS);                                                         \
    ASSERT_EQ(LRZ_QDMI_device_job_set_parameter(                               \
                  job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,                \
                  sizeof(format), &format),                                    \
              QDMI_SUCCESS);                                                   \
    ASSERT_EQ(LRZ_QDMI_device_job_set_parameter(                               \
                  job, QDMI_DEVICE_JOB_PARAMETER_PROGRAM, strlen(program) + 1, \
                  program),                                                    \
              QDMI_SUCCESS);                                                   \
  }

enum HardwareTypes {
  SIMULATOR,
  SUPERCONDUCTING,
  IONTRAP,
  NEUTRALATOMS,
};

typedef struct HardwareSession_d {
  std::string name;
  size_t n_qubit;
  HardwareTypes type;
  std::shared_ptr<LRZ_QDMI_Device_Session> session;
} HardwareSession;

std::vector<HardwareSession> HARDWARE_SESSIONS = {
    {"Q20", 20, SUPERCONDUCTING, nullptr},
    {"QExa20", 20, SUPERCONDUCTING, nullptr},
    {"QLM", 38, SIMULATOR, nullptr},
    {"AQT20", 20, IONTRAP, nullptr},
    {"MUNIQC-Atoms20", 20, NEUTRALATOMS, nullptr},
    {"Q5", 5, SUPERCONDUCTING, nullptr},
    {"WMI3", 3, SUPERCONDUCTING, nullptr},
    {"MAQCS", 12, SIMULATOR, nullptr},
};

std::unordered_map<std::string, HardwareSession *> NAME_SESSIONS = {
    {"Q20", &HARDWARE_SESSIONS[0]},
    {"QExa20", &HARDWARE_SESSIONS[1]},
    {"QLM", &HARDWARE_SESSIONS[2]},
    {"AQT20", &HARDWARE_SESSIONS[3]},
    {"MUNIQC-Atoms20", &HARDWARE_SESSIONS[4]},
    {"Q5", &HARDWARE_SESSIONS[5]},
    {"WMI3", &HARDWARE_SESSIONS[6]},
    {"MAQCS", &HARDWARE_SESSIONS[7]}};

std::vector<HardwareSession *> HARDWARE_SESSION_PTRS = {
    &HARDWARE_SESSIONS[0], &HARDWARE_SESSIONS[1], &HARDWARE_SESSIONS[2],
    &HARDWARE_SESSIONS[3], &HARDWARE_SESSIONS[4], &HARDWARE_SESSIONS[5],
    &HARDWARE_SESSIONS[6], &HARDWARE_SESSIONS[7],
};

class LRZBackendTest : public ::testing::TestWithParam<HardwareSession *> {
public:
  static std::vector<HardwareSession> LRZHardwareSessions;

protected:
  static void SetUpTestSuite() {
    char *hostname = getenv(LRZ_HOST_URL);
    char *token = getenv(MQP_SECRET_TOKEN);
    int err;
    EXIT_ON_FAIL(LRZ_QDMI_device_initialize(),
                 "Failed to initialize the device");
    size_t hardware_size;
    EXIT_ON_FAIL(
        LRZ_QDMI_device_session_query_device_property(
            nullptr, QDMI_DEVICE_PROPERTY_CUSTOM1, 0, nullptr, &hardware_size),
        "Could not fetch the hardwares");
    std::string hardwares(hardware_size - 1, '\0');

    EXIT_ON_FAIL(LRZ_QDMI_device_session_query_device_property(
                     nullptr, QDMI_DEVICE_PROPERTY_CUSTOM1, 0,
                     static_cast<void *>(hardwares.data()), nullptr),
                 "Could not fetch the hardwares");

    std::regex del(";");
    std::sregex_token_iterator hardware_iterator(hardwares.begin(),
                                                 hardwares.end(), del, -1);
    std::sregex_token_iterator hardware_iterator_end;
    while (hardware_iterator != hardware_iterator_end) {

      HardwareSession *hardware_session =
          NAME_SESSIONS[hardware_iterator++->str()];
      LRZ_QDMI_Device_Session device_session;
      LRZ_QDMI_device_session_alloc(&device_session);

      LRZ_QDMI_device_session_set_parameter(
          device_session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
          strlen(hostname) * sizeof(char) + 1, hostname);

      LRZ_QDMI_device_session_set_parameter(
          device_session, QDMI_DEVICE_SESSION_PARAMETER_TOKEN,
          strlen(token) * sizeof(char) + 1, token);

      LRZ_QDMI_device_session_set_parameter(
          device_session, QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1,
          hardware_session->name.size() * sizeof(char) + 1,
          hardware_session->name.data());

      err = LRZ_QDMI_device_session_init(device_session);
      if (err == QDMI_SUCCESS) {
        hardware_session->session =
            std::make_shared<LRZ_QDMI_Device_Session>(device_session);
      }
    }
  }

  static void TearDownTestSuite() {

    for (auto hardware_session_ptr : HARDWARE_SESSION_PTRS) {
      if (hardware_session_ptr->session != nullptr) {
        std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
            hardware_session_ptr->session;
        LRZ_QDMI_Device_Session session = *(session_ptr.get());
        LRZ_QDMI_device_session_free(session);
      }
    }

    LRZ_QDMI_device_finalize();
  }

  void SetUp() override {}
};

INSTANTIATE_TEST_SUITE_P(QDMITestInstantiation, LRZBackendTest,
                         ::testing::ValuesIn(HARDWARE_SESSION_PTRS));

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

TEST_P(LRZBackendTest, SessionSetParameterImplemented) {
  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());
  ASSERT_EQ(LRZ_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}
TEST_P(LRZBackendTest, SessionSetParameterAfterAllocated) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  char dummy_hostname[] = "qlm.lrz.de";
  ASSERT_EQ(LRZ_QDMI_device_session_set_parameter(
                session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                sizeof(dummy_hostname), &dummy_hostname),
            QDMI_ERROR_BADSTATE);
}

TEST_P(LRZBackendTest, ControlCreateJobImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  ASSERT_NE(LRZ_QDMI_device_session_create_device_job(session, &job),
            QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlSetParameterImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlSetShotParameterImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  int nShot = 50;
  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(nShot), &nShot),
            QDMI_SUCCESS);
  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlSetProgramFormatParameterImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  int nShot = 50;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;

  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT, sizeof(qirFormat),
                &qirFormat),
            QDMI_ERROR_NOTSUPPORTED);
  ASSERT_EQ(LRZ_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT,
                sizeof(qasmFormat), &qasmFormat),
            QDMI_SUCCESS);
  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlSubmitJobImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(LRZ_QDMI_device_job_submit(job), QDMI_ERROR_NOTIMPLEMENTED);
  ASSERT_NE(LRZ_QDMI_device_job_wait(job, 0), QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}
TEST_P(LRZBackendTest, ControlSubmitAndCancelJob) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  size_t nShot = 50;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();
  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);

  ASSERT_EQ(LRZ_QDMI_device_job_submit(job), QDMI_SUCCESS);

  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_SUBMITTED);

  ASSERT_EQ(LRZ_QDMI_device_job_cancel(job), QDMI_SUCCESS);

  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_CANCELED);

  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlSubmitAndWaitJob) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  size_t nShot = 50;
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

  ASSERT_EQ(LRZ_QDMI_device_job_submit(job), QDMI_SUCCESS);

  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_SUBMITTED);

  // Wait until running
  while (*job_status == QDMI_JOB_STATUS_SUBMITTED) {
    LRZ_QDMI_device_job_check(job, job_status);
  }

  CHECK_DEVICE_STATUS(device_status, QDMI_DEVICE_STATUS_BUSY);

  ASSERT_EQ(LRZ_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);

  CHECK_DEVICE_STATUS(device_status, QDMI_DEVICE_STATUS_IDLE);
  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_DONE);

  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlCancelImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;

  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  ASSERT_NE(LRZ_QDMI_device_job_cancel(job), QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlCheckImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  QDMI_Job_Status status = QDMI_JOB_STATUS_RUNNING;

  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(LRZ_QDMI_device_job_check(job, &status), QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlWaitImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;

  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(LRZ_QDMI_device_job_wait(job, 0), QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlGetDataImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_MAX, 0,
                                            nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlGetDataHistogramKeys) {
  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  size_t nShot = 50;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();
  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t histogram_size;
  char *histogram_keys;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);

  ASSERT_EQ(LRZ_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS,
                                            sizeof(size_t), nullptr,
                                            &histogram_size),
            QDMI_SUCCESS);

  histogram_keys = (char *)malloc(histogram_size);
  ASSERT_EQ(LRZ_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS,
                                            histogram_size, histogram_keys,
                                            nullptr),
            QDMI_SUCCESS);

  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlGetDataHistogramValue) {

  HardwareSession *hardware_session = GetParam();

  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  size_t nShot = 50;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t histogram_values_size;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(LRZ_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);

  ASSERT_EQ(LRZ_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES, 0,
                                            nullptr, &histogram_values_size),
            QDMI_SUCCESS);
  std::vector<int> histogram_values(histogram_values_size / sizeof(int));

  ASSERT_EQ(LRZ_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_HIST_VALUES, histogram_values_size,
                static_cast<void *>(histogram_values.data()), nullptr),
            QDMI_SUCCESS);

  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, ControlGetDataProbabilityKeys) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  size_t nShot = 50;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_keys_size;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(LRZ_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);

  ASSERT_EQ(LRZ_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS, 0, nullptr,
                &probability_keys_size),
            QDMI_SUCCESS);
  std::string probability_keys(probability_keys_size, '\0');

  ASSERT_EQ(LRZ_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS,
                probability_keys_size,
                static_cast<void *>(probability_keys.data()), nullptr),
            QDMI_SUCCESS);

  LRZ_QDMI_device_job_free(job);
}
TEST_P(LRZBackendTest, ControlGetDataProbabilityValues) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  size_t nShot = 50;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_values_size;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(LRZ_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);

  ASSERT_EQ(LRZ_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES, 0, nullptr,
                &probability_values_size),
            QDMI_SUCCESS);
  std::vector<double> probability_values(probability_values_size /
                                         sizeof(double));

  ASSERT_EQ(LRZ_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES,
                probability_values_size,
                static_cast<void *>(probability_values.data()), nullptr),
            QDMI_SUCCESS);

  LRZ_QDMI_device_job_free(job);
}
TEST_P(LRZBackendTest, ControlGetDataProbabilityDense) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  LRZ_QDMI_Device_Job job = nullptr;
  size_t nShot = 50;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;
  std::string test_circuit = Get_test_circuit();
  const char *c_t_c = test_circuit.c_str();

  QDMI_Job_Status *job_status =
      (QDMI_Job_Status *)malloc(sizeof(QDMI_Job_Status));
  size_t probability_dense_size;
  CREATE_JOB(job, nShot, qasmFormat, c_t_c);
  ASSERT_EQ(LRZ_QDMI_device_job_submit(job), QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_wait(job, 0), QDMI_SUCCESS);

  ASSERT_EQ(
      LRZ_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_PROBABILITIES_DENSE,
                                      0, nullptr, &probability_dense_size),
      QDMI_SUCCESS);

  std::vector<double> probability_dense(probability_dense_size /
                                        sizeof(double));

  ASSERT_EQ(LRZ_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_PROBABILITIES_DENSE,
                probability_dense_size,
                static_cast<void *>(probability_dense.data()), nullptr),
            QDMI_SUCCESS);

  LRZ_QDMI_device_job_free(job);
}

TEST_P(LRZBackendTest, QueryDevicePropertyImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}

TEST_P(LRZBackendTest, QuerySitePropertyImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  ASSERT_EQ(LRZ_QDMI_device_session_query_site_property(
                nullptr, nullptr, QDMI_SITE_PROPERTY_MAX, 0, nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
}
TEST_P(LRZBackendTest, QuerySitePropertyNotSupported) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  size_t size = 0;
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  std::vector<LRZ_QDMI_Site> sites(size / sizeof(LRZ_QDMI_Site));
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, size,
                static_cast<void *>(sites.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";

  ASSERT_EQ(LRZ_QDMI_device_session_query_site_property(
                session, sites.at(0), QDMI_SITE_PROPERTY_T1, sizeof(uint64_t),
                nullptr, nullptr),
            QDMI_ERROR_NOTSUPPORTED);
}

TEST_P(LRZBackendTest, QueryOperationProperties) {
  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr ||
      hardware_session->type == SIMULATOR)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  auto expected = hardware_session->type == SIMULATOR ? QDMI_ERROR_NOTSUPPORTED
                                                      : QDMI_SUCCESS;
  size_t size;
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_OPERATIONS, 0, nullptr, &size),
            expected)
      << "Devices must provide a list of sites";

  std::vector<LRZ_QDMI_Operation> operations(size / sizeof(LRZ_QDMI_Operation));
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_OPERATIONS, size,
                static_cast<void *>(operations.data()), nullptr),
            expected)
      << "Devices must provide a list of sites";

  for (const auto &op : operations) {
    size_t name_length = 0;
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_NAME, 0, nullptr, &name_length),
              expected);
    std::string name(name_length - 1, '\0');
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_NAME, name_length, name.data(),
                  nullptr),
              expected);

    size_t qubit_num = 0;
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_QUBITSNUM, sizeof(size_t), &qubit_num,
                  nullptr),
              expected);

    size_t sites_size = 0;
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_SITES, 0, nullptr, &sites_size),
              expected);
    std::vector<LRZ_QDMI_Site> sites(sites_size);
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_SITES, sites_size, sites.data(),
                  nullptr),
              expected);
  }
}

TEST_P(LRZBackendTest, QueryDeviceNameImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  size_t size = 0;

  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a name";
  std::string value(size - 1, '\0');
  ASSERT_EQ(
      LRZ_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_NAME, size, value.data(), nullptr),
      QDMI_SUCCESS)
      << "Devices must provide a name";
  ASSERT_FALSE(value.empty()) << "Devices must provide a name";
}

TEST_P(LRZBackendTest, QueryDeviceVersionImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  size_t size = 0;

  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_VERSION, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(
      LRZ_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_VERSION, size, value.data(), nullptr),
      QDMI_SUCCESS)
      << "Devices must provide a version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a version";
}

TEST_P(LRZBackendTest, QueryDeviceLibraryVersionImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  size_t size = 0;

  ASSERT_EQ(
      LRZ_QDMI_device_session_query_device_property(
          session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a library version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, size,
                value.data(), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a library version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a library version";
}

TEST_P(LRZBackendTest, QuerySiteIDImplemented) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());

  size_t size = 0;
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  std::vector<LRZ_QDMI_Site> sites(size / sizeof(LRZ_QDMI_Site));
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, size,
                static_cast<void *>(sites.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";
  size_t id = 0;
  for (auto *site : sites) {
    ASSERT_EQ(LRZ_QDMI_device_session_query_site_property(
                  session, site, QDMI_SITE_PROPERTY_INDEX, sizeof(size_t), &id,
                  nullptr),
              QDMI_SUCCESS)
        << "Devices must provide a site id";
  }
}

TEST_P(LRZBackendTest, QubitNum) {

  HardwareSession *hardware_session = GetParam();
  if (hardware_session->session == nullptr)
    GTEST_SKIP();
  std::shared_ptr<LRZ_QDMI_Device_Session> session_ptr =
      hardware_session->session;
  LRZ_QDMI_Device_Session session = *(session_ptr.get());
  size_t num_qubits = 0;
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_QUBITSNUM, sizeof(size_t),
                &num_qubits, nullptr),
            QDMI_SUCCESS);
}
