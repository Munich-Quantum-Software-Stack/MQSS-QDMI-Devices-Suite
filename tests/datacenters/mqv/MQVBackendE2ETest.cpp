/*
 * Copyright (c) 2026 Munich Quantum Valley
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/** @file
 * @brief End-to-end tests of the MQV QDMI device against an MQP-API.
 * @details Skipped unless MQV_QDMI_E2E_BASEURL is set. Run them against the
 * bundled mock with `tests/datacenters/mqv/mock_mqp/run_e2e.sh` (MQV site,
 * resource "Q4": 4 qubits, all-to-all, native gates r rz rxx measure).
 *
 * Environment: MQV_QDMI_E2E_BASEURL, MQV_QDMI_E2E_TOKEN, MQV_QDMI_E2E_RESOURCE.
 */

#include "mqv_qdmi/device.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string envOr(const char *name, const char *fallback = "") {
  const char *value = std::getenv(name);
  return value != nullptr ? value : fallback;
}

class MQVDeviceE2E : public ::testing::Test {
protected:
  MQV_QDMI_Device_Session session = nullptr;
  std::string baseUrl;
  std::string token;
  std::string resource;

  void SetUp() override {
    baseUrl = envOr("MQV_QDMI_E2E_BASEURL");
    if (baseUrl.empty()) {
      GTEST_SKIP() << "set MQV_QDMI_E2E_BASEURL (see "
                      "tests/datacenters/mqv/mock_mqp/run_e2e.sh)";
    }
    token = envOr("MQV_QDMI_E2E_TOKEN");
    resource = envOr("MQV_QDMI_E2E_RESOURCE", "Q4");
    ASSERT_EQ(MQV_QDMI_device_initialize(), QDMI_SUCCESS);
    session = openSession(baseUrl, token, resource);
    ASSERT_NE(session, nullptr);
  }

  void TearDown() override {
    if (session != nullptr) {
      MQV_QDMI_device_session_free(session);
    }
    MQV_QDMI_device_finalize();
  }

  static MQV_QDMI_Device_Session openSession(const std::string &url,
                                             const std::string &tok,
                                             const std::string &res) {
    MQV_QDMI_Device_Session s = nullptr;
    if (MQV_QDMI_device_session_alloc(&s) != QDMI_SUCCESS) {
      return nullptr;
    }
    const auto set = [&](QDMI_Device_Session_Parameter p,
                         const std::string &v) {
      return MQV_QDMI_device_session_set_parameter(s, p, v.size() + 1,
                                                   v.c_str());
    };
    if (set(QDMI_DEVICE_SESSION_PARAMETER_BASEURL, url) != QDMI_SUCCESS ||
        set(QDMI_DEVICE_SESSION_PARAMETER_TOKEN, tok) != QDMI_SUCCESS ||
        set(QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1, res) != QDMI_SUCCESS ||
        MQV_QDMI_device_session_init(s) != QDMI_SUCCESS) {
      MQV_QDMI_device_session_free(s);
      return nullptr;
    }
    return s;
  }

  template <class T> T queryValue(QDMI_Device_Property prop) {
    T value{};
    EXPECT_EQ(MQV_QDMI_device_session_query_device_property(
                  session, prop, sizeof(T), &value, nullptr),
              QDMI_SUCCESS);
    return value;
  }

  template <class T> std::vector<T> queryList(QDMI_Device_Property prop) {
    size_t size = 0;
    EXPECT_EQ(MQV_QDMI_device_session_query_device_property(session, prop, 0,
                                                            nullptr, &size),
              QDMI_SUCCESS);
    std::vector<T> values(size / sizeof(T));
    EXPECT_EQ(MQV_QDMI_device_session_query_device_property(
                  session, prop, size, values.data(), nullptr),
              QDMI_SUCCESS);
    return values;
  }

  std::string queryString(QDMI_Device_Property prop) {
    size_t size = 0;
    EXPECT_EQ(MQV_QDMI_device_session_query_device_property(session, prop, 0,
                                                            nullptr, &size),
              QDMI_SUCCESS);
    std::string value(size, '\0');
    EXPECT_EQ(MQV_QDMI_device_session_query_device_property(
                  session, prop, size, value.data(), nullptr),
              QDMI_SUCCESS);
    value.resize(size > 0 ? size - 1 : 0);
    return value;
  }

  /// Submit @p program, wait for a terminal state, return the final status.
  QDMI_Job_Status run(MQV_QDMI_Device_Job job, const std::string &program,
                      QDMI_Program_Format format, size_t shots) {
    const size_t bytes = program.size() + 1;
    const void *programs[] = {program.c_str()};
    EXPECT_EQ(
        MQV_QDMI_device_job_set_programs(job, format, 1, &bytes, programs),
        QDMI_SUCCESS);
    EXPECT_EQ(
        MQV_QDMI_device_job_set_parameter(
            job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(size_t), &shots),
        QDMI_SUCCESS);
    EXPECT_EQ(MQV_QDMI_device_job_submit(job), QDMI_SUCCESS);
    EXPECT_EQ(MQV_QDMI_device_job_wait(job, 120), QDMI_SUCCESS);
    QDMI_Job_Status status = QDMI_JOB_STATUS_CREATED;
    EXPECT_EQ(MQV_QDMI_device_job_check(job, &status), QDMI_SUCCESS);
    return status;
  }

  static std::map<std::string, size_t> histogram(MQV_QDMI_Device_Job job) {
    size_t keysSize = 0;
    EXPECT_EQ(MQV_QDMI_device_job_get_results(job, 0, QDMI_JOB_RESULT_HIST_KEYS,
                                              0, nullptr, &keysSize),
              QDMI_SUCCESS);
    std::string keys(keysSize, '\0');
    EXPECT_EQ(MQV_QDMI_device_job_get_results(job, 0, QDMI_JOB_RESULT_HIST_KEYS,
                                              keysSize, keys.data(), nullptr),
              QDMI_SUCCESS);
    keys.resize(keysSize > 0 ? keysSize - 1 : 0);
    size_t valuesSize = 0;
    EXPECT_EQ(MQV_QDMI_device_job_get_results(
                  job, 0, QDMI_JOB_RESULT_HIST_VALUES, 0, nullptr, &valuesSize),
              QDMI_SUCCESS);
    std::vector<size_t> values(valuesSize / sizeof(size_t));
    EXPECT_EQ(
        MQV_QDMI_device_job_get_results(job, 0, QDMI_JOB_RESULT_HIST_VALUES,
                                        valuesSize, values.data(), nullptr),
        QDMI_SUCCESS);
    std::map<std::string, size_t> hist;
    std::stringstream stream(keys);
    std::string key;
    size_t i = 0;
    while (std::getline(stream, key, ',') && i < values.size()) {
      hist[key] = values[i++];
    }
    return hist;
  }
};

// A Bell pair on qubits 0 and 1 using only the native gates of Q4:
// rxx(pi/2)|00> = (|00> - i|11>)/sqrt(2).
constexpr const char *BELL_QASM2 = R"(OPENQASM 2.0;
include "qelib1.inc";
qreg q[4];
creg c[4];
rxx(pi/2) q[0],q[1];
measure q -> c;
)";

constexpr const char *ZERO_QASM3 = R"(OPENQASM 3.0;
include "stdgates.inc";
qubit[4] q;
bit[4] c;
rz(0.5) q[0];
c = measure q;
)";

// `h` is not a native gate of Q4: the backend must reject the job.
constexpr const char *NON_NATIVE_QASM2 = R"(OPENQASM 2.0;
include "qelib1.inc";
qreg q[4];
creg c[4];
h q[0];
measure q -> c;
)";

TEST_F(MQVDeviceE2E, ReportsTheResource) {
  EXPECT_EQ(queryString(QDMI_DEVICE_PROPERTY_NAME), "MQV " + resource);
  EXPECT_EQ(queryValue<QDMI_Device_Status>(QDMI_DEVICE_PROPERTY_STATUS),
            QDMI_DEVICE_STATUS_IDLE);
  EXPECT_EQ(queryValue<size_t>(QDMI_DEVICE_PROPERTY_QUBITSNUM), 4U);
  EXPECT_EQ(queryList<MQV_QDMI_Site>(QDMI_DEVICE_PROPERTY_SITES).size(), 4U);
  // Default: all-to-all on 4 qubits, both directions: 12 pairs = 24 site
  // handles. Resources with another layout set MQV_QDMI_E2E_COUPLING_SITES.
  EXPECT_EQ(queryList<MQV_QDMI_Site>(QDMI_DEVICE_PROPERTY_COUPLINGMAP).size(),
            std::stoul(envOr("MQV_QDMI_E2E_COUPLING_SITES", "24")));

  std::vector<std::string> names;
  for (auto *op :
       queryList<MQV_QDMI_Operation>(QDMI_DEVICE_PROPERTY_OPERATIONS)) {
    size_t size = 0;
    ASSERT_EQ(MQV_QDMI_device_session_query_operation_property(
                  session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_NAME, 0, nullptr, &size),
              QDMI_SUCCESS);
    std::string name(size, '\0');
    ASSERT_EQ(MQV_QDMI_device_session_query_operation_property(
                  session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_NAME, size, name.data(), nullptr),
              QDMI_SUCCESS);
    names.emplace_back(name.c_str());
    // Every operation must say where it applies, or no circuit can be
    // placed (resources listing instructions as {None: None}).
    size_t sitesSize = 0;
    ASSERT_EQ(MQV_QDMI_device_session_query_operation_property(
                  session, op, 0, nullptr, 0, nullptr,
                  QDMI_OPERATION_PROPERTY_SITES, 0, nullptr, &sitesSize),
              QDMI_SUCCESS);
    EXPECT_GT(sitesSize, 0U) << "operation " << names.back() << " has no sites";
  }
  for (const auto *expected : {"r", "rz", "rxx", "measure"}) {
    EXPECT_NE(std::find(names.begin(), names.end(), expected), names.end())
        << "missing operation " << expected;
  }
}

TEST_F(MQVDeviceE2E, RunsABellCircuitInQasm2) {
  MQV_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(MQV_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  constexpr size_t shots = 200;
  ASSERT_EQ(run(job, BELL_QASM2, QDMI_PROGRAM_FORMAT_QASM2, shots),
            QDMI_JOB_STATUS_DONE);

  size_t idSize = 0;
  EXPECT_EQ(MQV_QDMI_device_job_query_property(job, QDMI_DEVICE_JOB_PROPERTY_ID,
                                               0, nullptr, &idSize),
            QDMI_SUCCESS);
  EXPECT_GT(idSize, 1U);

  const auto hist = histogram(job);
  size_t total = 0;
  for (const auto &[key, count] : hist) {
    EXPECT_EQ(key.size(), 4U) << "key width must equal the number of clbits";
    EXPECT_TRUE(key == "0000" || key == "0011") << "unexpected outcome " << key;
    total += count;
  }
  EXPECT_EQ(total, shots);
  EXPECT_TRUE(hist.contains("0000") && hist.contains("0011"));

  EXPECT_EQ(MQV_QDMI_device_job_get_results(job, 1, QDMI_JOB_RESULT_HIST_KEYS,
                                            0, nullptr, &idSize),
            QDMI_ERROR_OUTOFRANGE);
  EXPECT_EQ(MQV_QDMI_device_job_cancel(job), QDMI_ERROR_INVALIDARGUMENT);
  MQV_QDMI_device_job_free(job);
}

TEST_F(MQVDeviceE2E, RunsQasm3) {
  MQV_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(MQV_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(run(job, ZERO_QASM3, QDMI_PROGRAM_FORMAT_QASM3, 50),
            QDMI_JOB_STATUS_DONE);
  const auto hist = histogram(job);
  ASSERT_EQ(hist.size(), 1U);
  EXPECT_EQ(hist.begin()->first, "0000");
  EXPECT_EQ(hist.begin()->second, 50U);
  MQV_QDMI_device_job_free(job);
}

TEST_F(MQVDeviceE2E, ReportsARejectedCircuitAsFailed) {
  MQV_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(MQV_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);
  ASSERT_EQ(run(job, NON_NATIVE_QASM2, QDMI_PROGRAM_FORMAT_QASM2, 10),
            QDMI_JOB_STATUS_FAILED);
  size_t size = 0;
  EXPECT_EQ(MQV_QDMI_device_job_get_results(job, 0, QDMI_JOB_RESULT_HIST_KEYS,
                                            0, nullptr, &size),
            QDMI_ERROR_BADSTATE);
  MQV_QDMI_device_job_free(job);
}

TEST_F(MQVDeviceE2E, UnknownResourceIsAnError) {
  auto *other = openSession(baseUrl, token, "NO_SUCH_RESOURCE");
  ASSERT_NE(other, nullptr) << "init must not contact the MQP-API";
  size_t qubits = 0;
  EXPECT_EQ(MQV_QDMI_device_session_query_device_property(
                other, QDMI_DEVICE_PROPERTY_QUBITSNUM, sizeof(size_t), &qubits,
                nullptr),
            QDMI_ERROR_FATAL);
  QDMI_Device_Status status = QDMI_DEVICE_STATUS_IDLE;
  EXPECT_EQ(
      MQV_QDMI_device_session_query_device_property(
          other, QDMI_DEVICE_PROPERTY_STATUS, sizeof(status), &status, nullptr),
      QDMI_SUCCESS);
  EXPECT_EQ(status, QDMI_DEVICE_STATUS_OFFLINE);
  MQV_QDMI_device_session_free(other);
}

TEST_F(MQVDeviceE2E, WrongTokenLooksOffline) {
  auto *other = openSession(baseUrl, "wrong-token", resource);
  ASSERT_NE(other, nullptr);
  QDMI_Device_Status status = QDMI_DEVICE_STATUS_IDLE;
  EXPECT_EQ(
      MQV_QDMI_device_session_query_device_property(
          other, QDMI_DEVICE_PROPERTY_STATUS, sizeof(status), &status, nullptr),
      QDMI_SUCCESS);
  EXPECT_EQ(status, QDMI_DEVICE_STATUS_OFFLINE);
  MQV_QDMI_device_session_free(other);
}

} // namespace
