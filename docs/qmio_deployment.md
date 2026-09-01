# QMIO QDMI Device — deployment notes

Verified against the `qmiotools` source
(https://github.com/gomeztato/qmiotools) and CESGA's internal
`qpu-latency-benchmark` project.

## Execution model

There is **no remote submit/poll step** and **no host:port** for QMIO.
`qmiotools.integrations.qiskitqmio.QmioBackend` is a Qiskit `BackendV2`
whose `.run()` is **synchronous** (the library docstring states this
explicitly, and it must be called from a node with a QPU):

```python
from qmiotools.integrations.qiskitqmio import QmioBackend
backend = QmioBackend()          # loads newest calibration from $QMIO_CALIBRATIONS
job = backend.run(circuit, shots=1000)
result = job.result()            # blocks until the QPU returns
print(result.get_counts())
```

All communication with the Quantum Control Node happens inside
`qmio.QmioRuntimeService`, invisible to the caller. This is why `qmio.c`
embeds Python and calls `backend.run()` directly (same pattern as the
Qaptiva device) — no SSH/paramiko layer is needed inside the C device.

## What actually needs Slurm

The **whole process** that embeds this device — QDMI client + driver +
`libqmio_device.so` — must run inside a Slurm allocation of the `qpu`
partition. This is a deployment/launch concern, not something the device's
C code handles. Reference pattern (from `qpu-latency-benchmark`'s
`jobs/run_slurm.sh`):

```bash
#SBATCH -p qpu
module load qmio/hpc gcc/12.3.0 qmio-tools/0.2.1-python-3.11.9 qiskit/1.2.4-python-3.11.9
python -m your_qdmi_client_entrypoint ...
```

See `deploy/cesga/qmio_qdmi_test.sbatch` for a template that runs this
suite's own `QmioBackendTest` the same way. For MQSS integration, whatever
process hosts the QDMI driver needs the equivalent wrapper — worth raising
with the MQSS team: does their scheduler support "this device must be
launched inside a Slurm job," or does that need adding?

## FakeQmio: the CI / development path

`qmiotools` ships `FakeQmio`, which builds an `AerSimulator` from the
**same** calibration data with a thermal-relaxation noise model. It needs
`qmiotools` + `qiskit-aer` and calibration files (via `$QMIO_CALIBRATIONS`)
but **no QPU allocation**, so it runs on a login node or in CI.

Select it with session parameter **CUSTOM2 = 1** (int). The device's own
`QmioBackendTest` defaults to FakeQmio and only switches to the real QPU
when `QMIO_USE_REAL_QPU=1`. Note FakeQmio returns a plain `AerSimulator`,
which may report `coupling_map is None` (all-to-all, no routing) and does
not carry the resolved calibration path — the device handles both.

## Authentication

CESGA username + password is used once, over SSH, to reach the login node
from which `sbatch` is called. It happens *before* the QDMI device (or the
process embedding it) is ever started. The device's
`QDMI_DEVICE_SESSION_PARAMETER_USERNAME/PASSWORD/BASEURL` are therefore
accepted and silently ignored — see the docstring on
`QMIO_QDMI_device_session_set_parameter` in `qmio.c` — so a generic QDMI
client that always sets them doesn't fail, but they carry no meaning here.

## Calibration selection

QMIO's calibration data is periodically dumped to timestamped JSON files
named `YYYY_MM_DD__HH_MM_SS.json`. Discovery is handled **by the library**:
with `calibration_file=None`, `QmioBackend` loads the newest file (by
ctime) matching that glob from the directory named by the
**`QMIO_CALIBRATIONS`** environment variable. The device does not
reimplement this — it either passes an explicit path or lets the library
decide.

- Session parameter **CUSTOM1** (string): pin an explicit calibration file
  path (recommended for reproducible sweeps).
- Otherwise: ensure `QMIO_CALIBRATIONS` is exported (the module files
  normally set this) and the newest snapshot is used.

The resolved path is read back from the backend and is queryable via
`QDMI_DEVICE_PROPERTY_CUSTOM1` after `session_init` — useful provenance,
since calibration drift changes which qubits are excluded and therefore
shifts routing distance and depth between otherwise-identical runs.

## Native gate set and qubit metadata

Confirmed from `QmioBackend`'s Target construction:

- Single-qubit: `sx`, `x` (0 params), `rz(theta)` (1 param)
- Two-qubit: `ecr` — the only native entangling gate
- `measure`, `delay(t)`

The Target also carries **per-qubit T1, T2, and drive frequency** (loaded
from the calibration file), plus per-gate durations and errors. The device
exposes T1/T2 through `QDMI_SITE_PROPERTY_T1`/`T2` as raw values in
nanoseconds (`QDMI_DEVICE_PROPERTY_DURATIONUNIT` = "ns",
`DURATIONSCALEFACTOR` = 1.0). A QDMI FOMAC uses these for
calibration-aware qubit mapping. Per-gate error/duration exposure via
`QDMI_OPERATION_PROPERTY_*` is a natural next extension (the data is right
there in `target[gate][(qubits)]`).

## Transpilation knobs

`qpu-latency-benchmark` surfaced a real, previously-silent issue: leaving
`seed_transpiler` unset produces different depths for the *identical*
circuit across separate runs (observed: 742 vs. 541 for the same n=8 QFT
circuit), because Qiskit's routing pass is stochastic on sparse coupling
maps at any `optimization_level`. This device defaults job parameters to
`optimization_level=0` (CUSTOM2) and `seed_transpiler=42` (CUSTOM3) to
match the values CESGA's own benchmarking pins. Always set these explicitly
if reproducibility matters.

## Shot limits

`QmioBackend` enforces a `max_shots` cap (`shots × n_circuits`); exceeding
it raises `QmioException`. The device catches any exception from the
submit path and surfaces it as `QDMI_JOB_STATUS_FAILED` (the job's
`get_results` then returns nothing). If MQSS needs the cap value up front,
it can be exposed as a device property in a follow-up — the value is
computed in `QmioBackend` as `max_circuits × (default_shots × 10)`.
