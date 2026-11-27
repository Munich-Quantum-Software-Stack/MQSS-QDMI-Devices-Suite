"""Mock of an HPC site's MQSS front door (the MQP-API, ``/v1`` routes).

Stands in for "<site>'s MQSS client" in the federated-network diagram. It
mirrors the routes and response shapes of quantum-pathway/MQP-API that
MQSS-Client's REST transport consumes, including the string-encoded
``connectivity``/``instructions`` fields, and executes circuits with Qiskit's
BasicSimulator. Circuits that violate the advertised native gate set or
coupling map are rejected (status FAILED), so a successful run proves that the
device model travelled correctly through QDMI to the user's transpiler.

Usage: python mqp_mock.py --site MQV --port 8402 --token mqv-secret [--host 0.0.0.0]

Copied from Quantum-Pathway/mqss-qdmi-network@cca69b0 (poc/mock_site/mqp_mock.py)
as a test fixture for the MQV QDMI device. Changes: a --host option so the
mock can be reached from containers (default stays 127.0.0.1), and options
that mimic the MQV test instance (--deny-resource-detail, --result-shape maqcs,
--require-no-modify).
"""

import argparse
import ast
import json
import threading
import time
import uuid
from datetime import datetime, timezone

from flask import Flask, jsonify, request
from qiskit import qasm2, qasm3, transpile
from qiskit.providers.basic_provider import BasicSimulator


def _linear(n):
    return [[i, i + 1] for i in range(n - 1)] + [[i + 1, i] for i in range(n - 1)]


def _all_to_all(n):
    return [[i, j] for i in range(n) for j in range(n) if i != j]


# Resource catalogs per mimicked site (names are placeholders, not real QPUs).
SITES = {
    "LRZ": {"Q5": {"qubits": 5, "edges": _linear(5), "gates": ["rx", "rz", "cz", "measure"]}},
    "MQV": {
        "Q4": {"qubits": 4, "edges": _all_to_all(4), "gates": ["r", "rz", "rxx", "measure"]},
        # Encoded like AQT20 on the MQV test instance (2026-10-27):
        # connectivity "None", every instruction as {None: None}.
        "A4": {"qubits": 4, "edges": _all_to_all(4), "gates": ["rz", "r", "rxx", "measure"],
               "unrestricted": True},
        # Encoded like MAQCS on the MQV test instance (2026-10-28): connectivity
        # as "[(a, b), ...]" (tuples, one direction), explicit gate placements.
        # A 2x2 grid.
        "G4": {"qubits": 4, "edges": [(0, 1), (2, 3), (0, 2), (1, 3)],
               "gates": ["r", "rz", "rxx", "measure"]},
    },
}
TWO_QUBIT = {"cz", "rxx", "swap"}


def _resource_json(name, res):
    """Encode a resource exactly like MQP-API's resource_to_json."""
    if res.get("unrestricted"):
        return {
            "name": name,
            "qubits": res["qubits"],
            "online": True,
            "connectivity": "None",
            "instructions": ", ".join(f"('{g}',{{None: None}})" for g in res["gates"]),
        }
    singles = {(q,): None for q in range(res["qubits"])}
    pairs = {tuple(e): None for e in res["edges"]}
    instructions = [(g, pairs if g in TWO_QUBIT else singles) for g in res["gates"]]
    return {
        "name": name,
        "qubits": res["qubits"],
        "online": True,
        "connectivity": str(res["edges"]),
        "instructions": repr(instructions),
    }


def _now():
    return datetime.now(timezone.utc).isoformat()


def create_app(site, token, exec_delay=0.5, deny_resource_detail=False,
               result_shape="plain", require_no_modify=False):
    app = Flask(f"mqp-mock-{site}")
    resources = SITES[site]
    jobs = {}
    lock = threading.Lock()

    @app.before_request
    def _auth():
        if request.headers.get("Authorization") != f"Bearer {token}":
            return "FORBIDDEN", 403
        return None

    @app.get("/v1/resources")
    def all_resources():
        return jsonify({n: _resource_json(n, r) for n, r in resources.items()})

    @app.get("/v1/resources/<name>")
    def resource(name):
        # The MQV test instance (2026-10-27) answers 403 here for a token
        # that may list all resources; the device must fall back to the list.
        if deny_resource_detail:
            return "Forbidden", 403
        if name not in resources:
            return "RESOURCE NOT FOUND", 404
        return jsonify(_resource_json(name, resources[name]))

    @app.get("/v1/resources/<name>/num_pending_jobs")
    def pending(name):
        with lock:
            n = sum(1 for j in jobs.values() if j["resource"] == name and j["status"] in ("PENDING", "WAITING"))
        return jsonify({"num_pending_jobs": n})

    @app.post("/v1/job")
    def submit():
        body = request.get_json(force=True)
        name = body.get("resource_name")
        if name not in resources:
            return "BACKEND NOT FOUND", 404
        job_id = str(uuid.uuid4())
        with lock:
            jobs[job_id] = {"resource": name, "status": "PENDING", "result": None,
                            "submitted": _now(), "scheduled": None, "completed": None}
        threading.Thread(target=_execute, args=(job_id, body), daemon=True).start()
        app.logger.warning("[%s] job %s accepted for %s (%s, %s shots)",
                           site, job_id[:8], name, body.get("circuit_format"), body.get("shots"))
        return jsonify({"uuid": job_id}), 201

    def _execute(job_id, body):
        time.sleep(exec_delay)
        with lock:
            if jobs[job_id]["status"] == "CANCELLED":
                return
            # MQSS's QRM&CI selector currently routes OpenQASM jobs only with
            # no_modify=true; otherwise "No suitable backend found".
            if require_no_modify and not body.get("no_modify"):
                jobs[job_id].update(status="CANCELLED", completed=_now())
                app.logger.warning("[%s] job %s CANCELLED: no_modify=false", site, job_id[:8])
                return
            jobs[job_id].update(status="WAITING", scheduled=_now())
        try:
            res = resources[body["resource_name"]]
            circuits = body["circuit"]
            # MQSS-Client develop sends one program per job over REST; the
            # legacy Python client sends a stringified list. Accept both.
            if isinstance(circuits, str) and circuits.lstrip().startswith("["):
                circuits = ast.literal_eval(circuits)
            if isinstance(circuits, str):
                circuits = [circuits]
            load = qasm3.loads if body["circuit_format"] == "qasm3" else (
                lambda s: qasm2.loads(s, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS))
            counts = []
            for src in circuits:
                qc = load(src)
                _validate(qc, res)
                run = BasicSimulator().run(transpile(qc, optimization_level=0), shots=int(body["shots"]))
                counts.append(run.result().get_counts())
            if result_shape == "maqcs":  # as MAQCS's worker returns it
                result = [{"counts": c} for c in counts]
            else:
                result = counts[0] if len(counts) == 1 else counts
            with lock:
                jobs[job_id].update(status="COMPLETED", result=json.dumps(result), completed=_now())
            app.logger.warning("[%s] job %s completed: %s", site, job_id[:8], result)
        except Exception as exc:  # report as a failed job, like the real stack
            app.logger.warning("[%s] job %s FAILED: %s", site, job_id[:8], exc)
            with lock:
                jobs[job_id].update(status="FAILED", completed=_now())

    def _validate(qc, res):
        edges = {tuple(e) for e in res["edges"]}
        allowed = set(res["gates"]) | {"barrier"}
        for inst in qc.data:
            name = inst.operation.name
            if name not in allowed:
                raise ValueError(f"gate '{name}' is not native on this resource")
            qubits = tuple(qc.find_bit(q).index for q in inst.qubits)
            if len(qubits) == 2 and qubits not in edges:
                raise ValueError(f"'{name}' on {qubits} violates the coupling map")

    def _job(job_id):
        with lock:
            return dict(jobs[job_id]) if job_id in jobs else None

    @app.get("/v1/job/<job_id>/status")
    def status(job_id):
        job = _job(job_id)
        return (jsonify({"status": job["status"]}) if job else ("JOB NOT FOUND", 404))

    @app.get("/v1/job/<job_id>/result")
    def result(job_id):
        job = _job(job_id)
        if job is None:
            return "JOB NOT FOUND", 404
        return jsonify({"result": job["result"] or "",
                        "timestamp_submitted": job["submitted"],
                        "timestamp_scheduled": job["scheduled"] or "",
                        "timestamp_completed": job["completed"] or ""})

    @app.delete("/v1/job/<job_id>")
    def cancel(job_id):
        with lock:
            if job_id in jobs and jobs[job_id]["status"] in ("PENDING", "WAITING"):
                jobs[job_id]["status"] = "CANCELLED"
        return "OK"

    return app


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--site", required=True, choices=sorted(SITES))
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--token", required=True)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--deny-resource-detail", action="store_true",
                        help="answer 403 on /v1/resources/<name> (like the MQV test instance)")
    parser.add_argument("--result-shape", choices=["plain", "maqcs"], default="plain",
                        help='"maqcs": results as [{"counts": {...}}], like MAQCS on the MQV test instance')
    parser.add_argument("--require-no-modify", action="store_true",
                        help="cancel jobs sent with no_modify=false, like MQSS's selector today")
    args = parser.parse_args()
    create_app(args.site, args.token, deny_resource_detail=args.deny_resource_detail,
               result_shape=args.result_shape, require_no_modify=args.require_no_modify).run(
        host=args.host, port=args.port, threaded=True)
