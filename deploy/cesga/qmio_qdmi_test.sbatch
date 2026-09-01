#!/bin/bash
#SBATCH -p qpu
#SBATCH --mem-per-cpu=4G
#SBATCH -c 1
#SBATCH -t 00:30:00
#SBATCH -o qmio_qdmi_test_%j.out
#SBATCH -e qmio_qdmi_test_%j.err
# ==============================================================================
# SITE-SPECIFIC template, adapted from the confirmed-working pattern in
# CESGA's qpu-latency-benchmark repo (jobs/run_slurm.sh). If anything in the
# rest of this file's assumptions changes (partition name, module names/
# versions, calibration file location), update it here to match.
#
# What this proves: the QMIO QDMI Device's whole execution model rests on
# one fact -- the process embedding it (QDMI client + driver + device) must
# itself run *inside* this allocation. There is no separate remote-submit
# step; QmioBackend.run() is a synchronous in-process call once you're here.
# ==============================================================================
set -euo pipefail

if [ -n "${SLURM_SUBMIT_DIR:-}" ]; then
    REPO_ROOT="$SLURM_SUBMIT_DIR"
else
    REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
fi
cd "$REPO_ROOT"

echo "========================================="
echo " QMIO QDMI Device — SLURM test run"
echo " SLURM_JOB_ID=${SLURM_JOB_ID:-none}"
echo " $(date)"
echo " Repo: $REPO_ROOT"
echo "========================================="

module purge || true
# SITE-SPECIFIC: match the module versions your CESGA contact confirms are
# current -- these are the ones pinned in qpu-latency-benchmark as of this
# writing.
module load qmio/hpc gcc/12.3.0 qmio-tools/0.2.1-python-3.11.9 \
    qiskit/1.2.4-python-3.11.9 || true

# Point the embedded Python interpreter at the auxiliary module. Adjust
# BUILD_DIR to wherever this repo was configured/built.
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
export QMIO_AUXILIARY_SCRIPT_LOCATION="$BUILD_DIR/src/qpus/qmio"
export QMIO_AUXILIARY_SCRIPT_NAME=qmio_auxiliary

# Optional: pin a specific calibration snapshot instead of letting the
# qmiotools library auto-discover the newest one from $QMIO_CALIBRATIONS
# (recommended for reproducible sweeps -- see qmio.c / docs/qmio_deployment.md).
# export QMIO_CALIBRATION_FILE=/path/to/calibrations/2026_03_13__12_00_01.json

# This test defaults to FakeQmio (no QPU needed). To exercise the REAL QPU
# from inside this qpu-partition allocation, uncomment:
# export QMIO_USE_REAL_QPU=1

echo "START $(date +%s.%N)"
"$BUILD_DIR/tests/qpus/qmio/QmioBackendTest"
echo "END $(date +%s.%N)"
echo "========================================="
echo " Done — $(date)"
echo "========================================="
