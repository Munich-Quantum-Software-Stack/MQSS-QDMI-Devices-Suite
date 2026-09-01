/*
 * Copyright (c) 2024 - 2026 MQSS Project
 * All rights reserved.
 *
 * Licensed under the Apache License v2.0 with LLVM Exceptions (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "lrz_qdmi/constants.h"
#include "lrz_qdmi/device.h"
#include "lrz_qdmi/types.h"
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
                  device_session, QDMI_DEVICE_PROPERTY_STATUS, sizeof(size_t), \
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
    ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(device_session, &job), \
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

class LRZBackendTest : public ::testing::Test {
public:
protected:
  static LRZ_QDMI_Device_Session device_session;
  static void SetUpTestSuite() {

    char *hostname = getenv(LRZ_HOST_URL);
    char *token = getenv(MQP_SECRET_TOKEN);
    int err;
    EXIT_ON_FAIL(LRZ_QDMI_device_initialize(),
                 "Failed to initialize the device");

    err = LRZ_QDMI_device_session_alloc(&device_session);

    err = LRZ_QDMI_device_session_set_parameter(
        device_session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
        strlen(hostname) * sizeof(char) + 1, hostname);

    err = LRZ_QDMI_device_session_set_parameter(
        device_session, QDMI_DEVICE_SESSION_PARAMETER_TOKEN,
        strlen(token) * sizeof(char) + 1, token);

    LRZ_QDMI_device_session_init(device_session);
    ASSERT_EQ(err, QDMI_SUCCESS);
  }

  static void TearDownTestSuite() {
    LRZ_QDMI_device_session_free(device_session);

    LRZ_QDMI_device_finalize();
  }

  void SetUp() override {}
};

LRZ_QDMI_Device_Session LRZBackendTest::device_session = nullptr;

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

TEST_F(LRZBackendTest, QueryHardwares) {
  size_t hardware_size;
  int err = LRZ_QDMI_device_session_query_device_property(
      device_session, QDMI_DEVICE_PROPERTY_CUSTOM1, 0, nullptr, &hardware_size);
  ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to Query Hardware size";
  std::string hardwares(hardware_size - 1, '\0');

  err = LRZ_QDMI_device_session_query_device_property(
      device_session, QDMI_DEVICE_PROPERTY_CUSTOM1, hardware_size,
      static_cast<void *>(hardwares.data()), nullptr);
  ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to Query Hardwares";
}

TEST_F(LRZBackendTest, SetHardware) {
  size_t hardware_size;
  int err = LRZ_QDMI_device_session_query_device_property(
      device_session, QDMI_DEVICE_PROPERTY_CUSTOM1, 0, nullptr, &hardware_size);
  ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to Query Hardware size";
  std::string hardwares_str(hardware_size - 1, '\0');

  err = LRZ_QDMI_device_session_query_device_property(
      device_session, QDMI_DEVICE_PROPERTY_CUSTOM1, hardware_size,
      static_cast<void *>(hardwares_str.data()), nullptr);
  ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to Query Hardwares";

  std::vector<std::string> hardwares;
  size_t start = 0;
  size_t end;
  while ((end = hardwares_str.find(';', start)) != std::string::npos) {
    hardwares.push_back(hardwares_str.substr(start, end - start));
    start = end + 1;
  }
  hardwares.push_back(hardwares_str.substr(start));

  const char *hardwareToSet = "EQE1";

  err = LRZ_QDMI_device_session_set_parameter(
      device_session, QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1,
      strlen(hardwareToSet) * sizeof(char) + 1, hardwareToSet);
  ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to set hardwares";
}

TEST_F(LRZBackendTest, QueryHardwareQubit) {
  int qubitNum;
  int err = LRZ_QDMI_device_session_query_device_property(
      device_session, QDMI_DEVICE_PROPERTY_QUBITSNUM, sizeof(qubitNum),
      &qubitNum, nullptr);
  ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to Query Qubit Number";
}

TEST_F(LRZBackendTest, QueryHardwareSupportedProgramFormat) {
  size_t size;
  int err = LRZ_QDMI_device_session_query_device_property(
      device_session, QDMI_DEVICE_PROPERTY_SUPPORTEDPROGRAMFORMATS, 0, NULL,
      &size);
  ASSERT_EQ(err, QDMI_SUCCESS)
      << "Failed to Query Supported Program Format Size";

  std::vector<QDMI_Program_Format> supportedProgramFormat(
      size / sizeof(QDMI_Program_Format));
  err = LRZ_QDMI_device_session_query_device_property(
      device_session, QDMI_DEVICE_PROPERTY_SUPPORTEDPROGRAMFORMATS, size,
      static_cast<void *>(supportedProgramFormat.data()), nullptr);
  ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to Query Supported Program Formats";
}
TEST_F(LRZBackendTest, QueryHardwareSites) {
  size_t size;
  int err = LRZ_QDMI_device_session_query_device_property(
      device_session, QDMI_DEVICE_PROPERTY_SITES, 0, NULL, &size);
  ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to Query Site's Size";

  std::vector<LRZ_QDMI_Site> sites(size / sizeof(LRZ_QDMI_Site));
  err = LRZ_QDMI_device_session_query_device_property(
      device_session, QDMI_DEVICE_PROPERTY_SITES, size,
      static_cast<void *>(sites.data()), nullptr);
  ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to Query Sites";

  for (auto site : sites) {
    size_t index;
    err = LRZ_QDMI_device_session_query_site_property(
        device_session, site, QDMI_SITE_PROPERTY_INDEX, sizeof(size_t), &index,
        nullptr);
    ASSERT_EQ(err, QDMI_SUCCESS) << "Failed to Query Site's Index";
    ASSERT_GE(index, 0);

    ASSERT_EQ(LRZ_QDMI_device_session_query_site_property(
                  device_session, sites.at(0), QDMI_SITE_PROPERTY_T1,
                  sizeof(uint64_t), nullptr, nullptr),
              QDMI_ERROR_NOTSUPPORTED);
  }
}

TEST_F(LRZBackendTest, QueryDeviceLibraryVersionImplemented) {

  size_t size = 0;

  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                device_session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, 0, nullptr,
                &size),
            QDMI_SUCCESS)
      << "Devices must provide a library version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                device_session, QDMI_DEVICE_PROPERTY_LIBRARYVERSION, size,
                value.data(), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a library version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a library version";
}

TEST_F(LRZBackendTest, QueryDeviceVersionImplemented) {

  size_t size = 0;

  ASSERT_EQ(
      LRZ_QDMI_device_session_query_device_property(
          device_session, QDMI_DEVICE_PROPERTY_VERSION, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a version";
  std::string value(size - 1, '\0');
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                device_session, QDMI_DEVICE_PROPERTY_VERSION, size,
                value.data(), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a version";
  ASSERT_FALSE(value.empty()) << "Devices must provide a version";
}

TEST_F(LRZBackendTest, QueryDeviceNameImplemented) {

  size_t size = 0;

  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                device_session, QDMI_DEVICE_PROPERTY_NAME, 0, nullptr, &size),
            QDMI_SUCCESS)
      << "Devices must provide a name";
  std::string value(size - 1, '\0');
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                device_session, QDMI_DEVICE_PROPERTY_NAME, size, value.data(),
                nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a name";
  ASSERT_FALSE(value.empty()) << "Devices must provide a name";
}

TEST_F(LRZBackendTest, QueryOperationProperties) {
  size_t size;
  ASSERT_EQ(
      LRZ_QDMI_device_session_query_device_property(
          device_session, QDMI_DEVICE_PROPERTY_OPERATIONS, 0, nullptr, &size),
      QDMI_SUCCESS)
      << "Devices must provide a list of sites";

  std::vector<LRZ_QDMI_Operation> operations(size / sizeof(LRZ_QDMI_Operation));
  ASSERT_EQ(LRZ_QDMI_device_session_query_device_property(
                device_session, QDMI_DEVICE_PROPERTY_OPERATIONS, size,
                static_cast<void *>(operations.data()), nullptr),
            QDMI_SUCCESS)
      << "Devices must provide a list of sites";

  for (const auto &op : operations) {
    size_t name_length = 0;
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  device_session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_NAME, 0, nullptr, &name_length),
              QDMI_SUCCESS);
    std::string name(name_length - 1, '\0');
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  device_session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_NAME, name_length, name.data(),
                  nullptr),
              QDMI_SUCCESS);

    size_t qubit_num = 0;
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  device_session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_QUBITSNUM, sizeof(size_t), &qubit_num,
                  nullptr),
              QDMI_SUCCESS);

    size_t sites_size = 0;
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  device_session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_SITES, 0, nullptr, &sites_size),
              QDMI_SUCCESS);
    std::vector<LRZ_QDMI_Site> sites(sites_size);
    ASSERT_EQ(LRZ_QDMI_device_session_query_operation_property(
                  device_session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_SITES, sites_size, sites.data(),
                  nullptr),
              QDMI_SUCCESS);
  }
}

TEST_F(LRZBackendTest, ControlCreateJobImplemented) {

  LRZ_QDMI_Device_Job job = nullptr;
  ASSERT_NE(LRZ_QDMI_device_session_create_device_job(device_session, &job),
            QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}

TEST_F(LRZBackendTest, ControlSetParameterImplemented) {

  LRZ_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(device_session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_MAX, 0, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  LRZ_QDMI_device_job_free(job);
}

TEST_F(LRZBackendTest, ControlSetShotParameterImplemented) {

  LRZ_QDMI_Device_Job job = nullptr;
  int nShot = 50;
  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(device_session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(nShot), &nShot),
            QDMI_SUCCESS);
  LRZ_QDMI_device_job_free(job);
}

TEST_F(LRZBackendTest, ControlSetProgramFormatParameterImplemented) {

  LRZ_QDMI_Device_Job job = nullptr;
  int nShot = 50;
  const QDMI_Program_Format qirFormat = QDMI_PROGRAM_FORMAT_QIRBASESTRING;
  const QDMI_Program_Format qasmFormat = QDMI_PROGRAM_FORMAT_QASM2;

  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(device_session, &job),
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

TEST_F(LRZBackendTest, ControlSubmitJobImplemented) {

  LRZ_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(device_session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(LRZ_QDMI_device_job_submit(job), QDMI_ERROR_NOTIMPLEMENTED);
  ASSERT_NE(LRZ_QDMI_device_job_wait(job, 0), QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}

TEST_F(LRZBackendTest, ControlSubmitAndWaitJob) {

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

  ASSERT_EQ(LRZ_QDMI_device_job_wait(job, 600), QDMI_SUCCESS);

  CHECK_DEVICE_STATUS(device_status, QDMI_DEVICE_STATUS_IDLE);
  CHECK_JOB_STATUS(job_status, QDMI_JOB_STATUS_DONE);

  LRZ_QDMI_device_job_free(job);
}

TEST_F(LRZBackendTest, ControlCancelImplemented) {

  LRZ_QDMI_Device_Job job = nullptr;

  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(device_session, &job),
            QDMI_SUCCESS);

  ASSERT_NE(LRZ_QDMI_device_job_cancel(job), QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}

TEST_F(LRZBackendTest, ControlCheckImplemented) {

  LRZ_QDMI_Device_Job job = nullptr;
  QDMI_Job_Status status = QDMI_JOB_STATUS_RUNNING;

  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(device_session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(LRZ_QDMI_device_job_check(job, &status), QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}

TEST_F(LRZBackendTest, ControlWaitImplemented) {

  LRZ_QDMI_Device_Job job = nullptr;

  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(device_session, &job),
            QDMI_SUCCESS);
  ASSERT_NE(LRZ_QDMI_device_job_wait(job, 600), QDMI_ERROR_NOTIMPLEMENTED);
  LRZ_QDMI_device_job_free(job);
}
TEST_F(LRZBackendTest, ControlGetDataImplemented) {

  LRZ_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(LRZ_QDMI_device_session_create_device_job(device_session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(LRZ_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_MAX, 0,
                                            nullptr, nullptr),
            QDMI_ERROR_INVALIDARGUMENT);
  LRZ_QDMI_device_job_free(job);
}

TEST_F(LRZBackendTest, ControlGetDataHistogramKeys) {

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
  ASSERT_EQ(LRZ_QDMI_device_job_wait(job, 600), QDMI_SUCCESS);
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

TEST_F(LRZBackendTest, ControlGetDataHistogramValue) {

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
  ASSERT_EQ(LRZ_QDMI_device_job_wait(job, 600), QDMI_SUCCESS);

  ASSERT_EQ(LRZ_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES, 0,
                                            nullptr, &histogram_values_size),
            QDMI_SUCCESS);

  std::vector<size_t> histogram_values(histogram_values_size / sizeof(size_t));

  ASSERT_EQ(LRZ_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_HIST_VALUES, histogram_values_size,
                static_cast<void *>(histogram_values.data()), nullptr),
            QDMI_SUCCESS);

  LRZ_QDMI_device_job_free(job);
}

} // namespace
