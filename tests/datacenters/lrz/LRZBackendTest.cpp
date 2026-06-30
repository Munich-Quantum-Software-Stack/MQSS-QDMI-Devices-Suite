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

#include "lrz_qdmi/constants.h"
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

  static void TearDownTestSuite() { LRZ_QDMI_device_finalize(); }

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

  const char *hardwareToSet = hardwares[0].c_str();

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



} // namespace
