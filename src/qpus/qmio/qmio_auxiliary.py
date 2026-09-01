# Copyright 2025 Munich Quantum Software Stack Project / CESGA
#
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------

## @file qmio_auxiliary.py
#  @brief Auxiliary module for the QMIO QDMI Device.
#
#  Loaded by the embedded Python interpreter inside the QMIO QDMI Device
#  (qmio.c) and wraps CESGA's `qmiotools` package
#  (https://github.com/gomeztato/qmiotools).
#
#  Deployment model (verified against the qmiotools source):
#    - `QmioBackend` is a Qiskit BackendV2 whose `.run()` is SYNCHRONOUS
#      (the library docstring states this explicitly). It must be called
#      from a node with a QPU, i.e. inside a Slurm allocation of the qpu
#      partition. There is no host/URL and no submit/poll step; all QPU
#      communication is internal to qmiotools (qmio.QmioRuntimeService).
#    - Calibration discovery is handled BY THE LIBRARY: with
#      calibration_file=None it loads the newest file matching
#      ????_??_??__??_??_??.json from the directory named by the
#      QMIO_CALIBRATIONS environment variable (newest by ctime). We do NOT
#      reimplement that here -- we pass either an explicit path or None.
#    - `FakeQmio` builds an AerSimulator from the SAME calibration data with
#      a thermal-relaxation noise model, so it also needs a calibration
#      file / QMIO_CALIBRATIONS but requires no QPU. It is the CI/dev path.
#
#  Native gate set (from QmioBackend Target construction): sx, x, rz(theta),
#  ecr, measure, delay. Two-qubit gate: ecr. Parameterized: rz.


def create_backend(calibration_file="", use_fake=0):
    """! Creates a QMIO backend handle.

    @param calibration_file Explicit calibration JSON path. If empty, the
           library auto-discovers the newest snapshot from $QMIO_CALIBRATIONS.
    @param use_fake If nonzero, build FakeQmio (AerSimulator + QMIO noise
           model) instead of the real QmioBackend. Still needs calibration
           data, but runs without a QPU allocation.
    @return [backend, resolved_calibration_file_path] or None on failure.

    @details Provenance note: FakeQmio is a *function* that internally builds
    a QmioBackend, then returns a bare AerSimulator which does NOT carry the
    resolved calibration path. QmioBackend itself stores it on
    `_calibration_file`, but to get a correct path for BOTH backends we
    resolve it directly via the same library helper the backend uses
    (Calibrations.import_last_calibration(...).get_filename()), rather than
    reading an attribute that only one of the two backends exposes.
    """
    try:
        cf = calibration_file or None

        # Resolve the path the library WOULD load (same logic QmioBackend and
        # FakeQmio use internally), so provenance is correct regardless of
        # which backend we build.
        resolved = ""
        try:
            from qmiotools.integrations.utils import Calibrations
            resolved = Calibrations.import_last_calibration(cf).get_filename()
        except Exception:  # noqa: BLE001
            resolved = cf or ""

        if int(use_fake):
            from qmiotools.integrations.qiskitqmio import FakeQmio
            backend = FakeQmio(calibration_file=cf)
        else:
            from qmiotools.integrations.qiskitqmio import QmioBackend
            backend = QmioBackend(calibration_file=cf)
            # Prefer the backend's own record when available (real path).
            resolved = getattr(backend, "_calibration_file", None) or resolved

        return [backend, resolved]
    except Exception as exc:  # noqa: BLE001
        print("QMIO backend creation failed:", exc)
        return None


def get_metadata(backend):
    """! Extracts device metadata from the backend Target.

    @param backend A backend created by create_backend() (backend object,
           i.e. result[0]).
    @return [num_qubits, "op1,op2,...", "a-b;c-d;..."] or None on failure.
            Third element is the directed coupling map as semicolon-separated
            "control-target" edges. For FakeQmio (AerSimulator) the coupling
            map may be None -> empty string (all-to-all, no routing needed).
    """
    try:
        num_qubits = int(backend.num_qubits)

        ops = sorted(set(backend.operation_names))
        ops_csv = ",".join(ops)

        cmap = backend.coupling_map
        if cmap is None:
            edges_csv = ""
        else:
            edges_csv = ";".join(f"{a}-{b}" for (a, b) in cmap.get_edges())

        return [num_qubits, ops_csv, edges_csv]
    except Exception as exc:  # noqa: BLE001
        print("QMIO metadata query failed:", exc)
        return None


def get_site_properties(backend):
    """! Extracts per-qubit calibration data from the backend Target.

    @param backend A backend created by create_backend().
    @return A list of length num_qubits, each entry "t1,t2,frequency" with
            values in Qiskit's native units (T1/T2 in seconds, frequency in
            Hz), or "nan,nan,nan" where a qubit has no properties (e.g.
            excluded by calibration). None on failure.

    @details QmioBackend populates Target.qubit_properties from the
    calibration file (T1 (s), T2 (s), Drive Frequency (Hz)). This is what a
    QDMI FOMAC consumer wants for T1/T2-aware mapping.
    """
    try:
        num_qubits = int(backend.num_qubits)
        props = backend.target.qubit_properties
        rows = []
        for i in range(num_qubits):
            p = props[i] if props is not None and i < len(props) else None
            if p is None:
                rows.append("nan,nan,nan")
                continue
            t1 = getattr(p, "t1", None)
            t2 = getattr(p, "t2", None)
            freq = getattr(p, "frequency", None)
            rows.append(
                "%s,%s,%s"
                % (
                    "nan" if t1 is None else repr(float(t1)),
                    "nan" if t2 is None else repr(float(t2)),
                    "nan" if freq is None else repr(float(freq)),
                )
            )
        return rows
    except Exception as exc:  # noqa: BLE001
        print("QMIO site-property query failed:", exc)
        return None


def _parse_circuit(program, program_format):
    """Parse an OpenQASM 2/3 string into a QuantumCircuit."""
    if program_format == 3:
        from qiskit.qasm3 import loads as q3loads
        return q3loads(program)
    from qiskit.qasm2 import loads as q2loads
    return q2loads(program)


def submit_job(backend, program, program_format, nshots, do_transpile=1,
               optimization_level=0, seed_transpiler=42):
    """! Transpiles and executes a circuit on QMIO (blocking).

    @param backend A backend created by create_backend() (backend object).
    @param program OpenQASM program as a string.
    @param program_format 2 for OpenQASM 2, 3 for OpenQASM 3.
    @param nshots Number of shots. NOTE: QmioBackend enforces a max_shots
           cap (shots * n_circuits); exceeding it raises QmioException,
           which is caught here and surfaced as a failed job (returns None).
    @param do_transpile If nonzero, transpile against the backend Target
           (maps to sx/x/rz/ecr + QMIO topology) before running.
    @param optimization_level Qiskit transpiler optimization level. Default
           0, matching CESGA's own benchmarking pins for comparable,
           reproducible transpiled depth.
    @param seed_transpiler Explicit transpiler seed. Routing is stochastic
           on a sparse coupling map at any optimization_level; pin it for
           reproducibility (see qmio.c for the observed 742-vs-541 case).
    @return ["state1,state2,...", p1, p2, ...] with probabilities per state,
            or None on failure.
    """
    try:
        circuit = _parse_circuit(program, int(program_format))

        if int(do_transpile):
            from qiskit import transpile
            circuit = transpile(
                circuit,
                backend=backend,
                optimization_level=int(optimization_level),
                seed_transpiler=int(seed_transpiler),
            )

        job = backend.run(circuit, shots=int(nshots))
        result = job.result()
        counts = result.get_counts()

        states = list(counts.keys())
        shots = float(nshots)
        probabilities = [counts[s] / shots for s in states]
        return [",".join(states)] + probabilities
    except Exception as exc:  # noqa: BLE001
        print("QMIO job submission failed:", exc)
        return None
