from time import sleep

def get_port_and_hostname():
    #For testing purposes, It needs to be updated!
    return 20501, "localhost" 

def create_remote_qpu(hostname, port):
    from qat.core.qpu import RemoteQPU
    try:
        qpu = RemoteQPU(port, hostname)
    except:
        return None
    return qpu

def parse(qasm_string):
    try:
        return circuit
    except Exception as e:
        print(e)
        return None


def submit_job(remote_qpu, qasm_string, nshots):
    #remote_qpu = create_remote_qpu("localhost", 20501)
    #print("-"*10)
    #print(remote_qpu is None)
    #print(qasm_string)
    #print(nshots)
    #print("-"*10)
    from qat.interop.openqasm import OqasmParser
    parser = OqasmParser()
    circuit = parser.compile(qasm_string)
    circuit = parse(qasm_string)
    if circuit is None:
        return None
    job = circuit.to_job(nbshots = nshots)
    raw_results = remote_qpu.submit(job)
    states = []
    probabilities = [] 
    for result in raw_results:
        states.append(result.state.bitstring)
        probabilities.append((result.probability))
    
    return_value = [",".join(states)] + list(probabilities)
    #print(return_value)
    return return_value
    