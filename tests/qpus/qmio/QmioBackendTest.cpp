/*------------------------------------------------------------------------------
Copyright 2025 Munich Quantum Software Stack Project / CESGA

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

/** @file
 * @brief Tests for the QMIO QDMI Device, mirroring QaptivaBackendTest.cpp.
 * @details By default this test uses FakeQmio (session CUSTOM2 = 1): an
 * AerSimulator built from real QMIO calibration data with a
 * thermal-relaxation noise model. That path needs qmiotools + qiskit-aer
 * and calibration files reachable via $QMIO_CALIBRATIONS, but does NOT need
 * a QPU allocation, so it can run in CI or on a login node. Set
 * QMIO_USE_REAL_QPU=1 (inside a Slurm allocation of the qpu partition,
 * after `module load qmio/hpc qmio-tools/... qiskit/...`) to exercise the
 * real QPU instead. See deploy/cesga/qmio_qdmi_test.sbatch.
 */

#include "qdmi/constants.h"
#include "qmio_qdmi/device.h"

#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr const char *GHZ_QASM2 = R"(OPENQASM 2.0;
include "qelib1.inc";
qreg q[3];
creg c[3];
h q[0];
cx q[0], q[1];
cx q[1], q[2];
measure q -> c;
)";

class QmioDeviceTest : public ::testing::Test {
protected:
  QMIO_QDMI_Device_Session session = nullptr;

  void SetUp() override {
    ASSERT_EQ(QMIO_QDMI_device_initialize(), QDMI_SUCCESS)
        << "Set QMIO_AUXILIARY_SCRIPT_LOCATION and "
           "QMIO_AUXILIARY_SCRIPT_NAME, make qmiotools importable "
           "(module load qmio/hpc qmio-tools/...), and ensure "
           "$QMIO_CALIBRATIONS points at readable calibration files.";
    ASSERT_EQ(QMIO_QDMI_device_session_alloc(&session), QDMI_SUCCESS);

    // Default to FakeQmio unless QMIO_USE_REAL_QPU=1.
    const char *real = std::getenv("QMIO_USE_REAL_QPU");
    int use_fake = (real != nullptr && std::strcmp(real, "1") == 0) ? 0 : 1;
    ASSERT_EQ(QMIO_QDMI_device_session_set_parameter(
                  session, QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2,
                  sizeof(use_fake), &use_fake),
              QDMI_SUCCESS);

    // Optionally pin a specific calibration snapshot for reproducibility.
    const char *pinned = std::getenv("QMIO_CALIBRATION_FILE");
    if (pinned != nullptr) {
      ASSERT_EQ(QMIO_QDMI_device_session_set_parameter(
                    session, QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1,
                    std::strlen(pinned) + 1, pinned),
                QDMI_SUCCESS);
    }
    ASSERT_EQ(QMIO_QDMI_device_session_init(session), QDMI_SUCCESS);
  }

  void TearDown() override {
    QMIO_QDMI_device_session_free(session);
    QMIO_QDMI_device_finalize();
  }
};

TEST_F(QmioDeviceTest, QueryMetadata) {
  size_t num_qubits = 0;
  ASSERT_EQ(QMIO_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_QUBITSNUM, sizeof(size_t),
                &num_qubits, nullptr),
            QDMI_SUCCESS);
  EXPECT_EQ(num_qubits, 32U) << "QMIO exposes 32 qubits";

  size_t ops_size = 0;
  ASSERT_EQ(QMIO_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_OPERATIONS, 0, nullptr,
                &ops_size),
            QDMI_SUCCESS);
  EXPECT_GT(ops_size, 0U);

  // Provenance check: which calibration snapshot got loaded.
  size_t calib_size = 0;
  ASSERT_EQ(QMIO_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_CUSTOM1, 0, nullptr,
                &calib_size),
            QDMI_SUCCESS);
  EXPECT_GT(calib_size, 1U) << "resolved_calibration_file should be non-empty";

  // Per-qubit T1 should be populated from the calibration (raw ns).
  size_t sites_size = 0;
  ASSERT_EQ(QMIO_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, 0, nullptr, &sites_size),
            QDMI_SUCCESS);
  std::vector<QMIO_QDMI_Site> sites(sites_size / sizeof(QMIO_QDMI_Site));
  ASSERT_EQ(QMIO_QDMI_device_session_query_device_property(
                session, QDMI_DEVICE_PROPERTY_SITES, sites_size, sites.data(),
                nullptr),
            QDMI_SUCCESS);
  ASSERT_FALSE(sites.empty());
  uint64_t t1_raw = 0;
  ASSERT_EQ(QMIO_QDMI_device_session_query_site_property(
                session, sites[0], QDMI_SITE_PROPERTY_T1, sizeof(t1_raw),
                &t1_raw, nullptr),
            QDMI_SUCCESS);
  EXPECT_GT(t1_raw, 0U) << "qubit 0 T1 should be a positive duration in ns";
}

TEST_F(QmioDeviceTest, RunGhzCircuit) {
  QMIO_QDMI_Device_Job job = nullptr;
  ASSERT_EQ(QMIO_QDMI_device_session_create_device_job(session, &job),
            QDMI_SUCCESS);

  size_t shots = 1000;
  QDMI_Program_Format format = QDMI_PROGRAM_FORMAT_QASM2;
  ASSERT_EQ(QMIO_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM, sizeof(shots),
                &shots),
            QDMI_SUCCESS);
  ASSERT_EQ(QMIO_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT, sizeof(format),
                &format),
            QDMI_SUCCESS);
  ASSERT_EQ(QMIO_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_PROGRAM,
                std::strlen(GHZ_QASM2) + 1, GHZ_QASM2),
            QDMI_SUCCESS);
  // seed_transpiler pinned explicitly (job creation already defaults to 42,
  // set again here to document that this must always be pinned for
  // reproducible routing -- see qmio.c's job struct docstring).
  int seed = 42;
  ASSERT_EQ(QMIO_QDMI_device_job_set_parameter(
                job, QDMI_DEVICE_JOB_PARAMETER_CUSTOM3, sizeof(seed), &seed),
            QDMI_SUCCESS);

  ASSERT_EQ(QMIO_QDMI_device_job_submit(job), QDMI_SUCCESS);

  QDMI_Job_Status status = QDMI_JOB_STATUS_CREATED;
  ASSERT_EQ(QMIO_QDMI_device_job_check(job, &status), QDMI_SUCCESS);
  ASSERT_EQ(status, QDMI_JOB_STATUS_DONE);

  size_t keys_size = 0;
  ASSERT_EQ(QMIO_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS, 0,
                                             nullptr, &keys_size),
            QDMI_SUCCESS);
  std::string keys(keys_size, '\0');
  ASSERT_EQ(QMIO_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_KEYS,
                                             keys_size, keys.data(), nullptr),
            QDMI_SUCCESS);
  keys.resize(std::strlen(keys.c_str()));

  size_t values_size = 0;
  ASSERT_EQ(QMIO_QDMI_device_job_get_results(
                job, QDMI_JOB_RESULT_HIST_VALUES, 0, nullptr, &values_size),
            QDMI_SUCCESS);
  std::vector<int> values(values_size / sizeof(int));
  ASSERT_EQ(QMIO_QDMI_device_job_get_results(job, QDMI_JOB_RESULT_HIST_VALUES,
                                             values_size, values.data(),
                                             nullptr),
            QDMI_SUCCESS);

  // Aggregate counts and check GHZ signature: |000> and |111> dominate.
  // Real-hardware noise (unlike a noiseless simulator) means this must be a
  // generous, noise-tolerant threshold, not an exact-match assertion.
  std::map<std::string, int> counts;
  std::stringstream ss(keys);
  std::string state;
  size_t i = 0;
  while (std::getline(ss, state, ',') && i < values.size())
    counts[state] = values[i++];

  int ghz = counts["000"] + counts["111"];
  EXPECT_GT(ghz, static_cast<int>(shots) / 2)
      << "|000> + |111> should dominate for a GHZ state on real hardware";

  QMIO_QDMI_device_job_free(job);
}

} // namespace
