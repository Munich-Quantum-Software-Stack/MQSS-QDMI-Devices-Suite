def create_remote_qpu(host):
    try:
        from qat.core.qpu import RemoteQPU
        url, port = host.split(":")
        qpu = RemoteQPU(port, url)
    except:
        return None
    return qpu

def submit_job(remote_qpu, qasm_string, nshots):
    try:
        from qat.interop.openqasm import OqasmParser
        parser = OqasmParser()
        circuit = parser.compile(qasm_string)
        job = circuit.to_job(nbshots=nshots)
        raw_results = remote_qpu.submit(job)
        states = []
        probabilities = []
        for result in raw_results:
            states.append(result.state.bitstring)
            probabilities.append((result.probability))
        return_value = [",".join(states)] + list(probabilities)
        return return_value
    except:
        return None

