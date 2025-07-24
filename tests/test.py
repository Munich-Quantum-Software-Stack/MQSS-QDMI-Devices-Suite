
from qat.core.qpu import RemoteQPU
from qat.interop.openqasm import OqasmParser
from qiskit import QuantumCircuit, qasm2

qc = QuantumCircuit(2)
qc.h(0)
qc.cx(0, 1)
qc.measure_all()
qasm_string = qasm2.dumps(qc)
host = "host.docker.internal:20501"
url, port = host.split(":")
qpu = RemoteQPU(port, url)

parser = OqasmParser()
circuit = parser.compile(qasm_string)
job = circuit.to_job(nbshots=1024)
raw_results = qpu.submit(job)
states = []
probabilities = []
for result in raw_results:
    states.append(result.state.bitstring)
    probabilities.append((result.probability))
return_value = [",".join(states)] + list(probabilities)
print(return_value)
