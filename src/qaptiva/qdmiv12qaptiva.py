from time import sleep


def parse_circuit(qasm_string):
    from qat.interop.openqasm import OqasmParser
    parser = OqasmParser()
    try:
        circuit = parser.compile(qasm_string)
        if not circuit:
            raise Exception('openqasm parser failed')
    except:
        return None

    return circuit


def create_qat_job(qasm_string):
    circuit = parse_circuit(qasm_string)
    if circuit is None:
        return None
    return circuit.to_job()

def create_remote_qpu(remote_qpu):
    from qat.core.qpu import RemoteQPU
    import sys
    try:
        qpu = RemoteQPU(int(remote_qpu[0]), str(remote_qpu[1]))
    except:
        _type, _value, _traceback = sys.exc_info()
        print('remoteqpu_connection_error', 1, _value)
    return qpu


def submit_job(job):
    import sys
    from time import sleep
    #print(job)
    #for x in range(6):
    #    sleep(x)
    #    print("sleeeps in.. " + str(x) )
    #exit(1)
    #DEFAULT_REMOTEQPU_PORT = 20500
    #DEFAULT_REMOTEQPU_PORT = 443
    #DEFAULT_REMOTEQPU_HOSTNAME = 'qlm3.for.lrz.de'
    #remote_qpu = (DEFAULT_REMOTEQPU_PORT, DEFAULT_REMOTEQPU_HOSTNAME)
    #print(f'Connecting to RemoteQPU {remote_qpu}...')
    #qpu = create_remote_qpu(remote_qpu)
    #try:
    #    result = qpu.submit(job)
    #except:
    #    _type, _value, _traceback = sys.exc_info()
    #    print(_type)
    #    print(_value)
    #    print("-----")
    #    print(_traceback)
    #    print("-----")
    #    exit(1)

    results = {
        "01" : 0.25,
        "00" : 0.5,
        "11" : 0.25
    }


    retrunValue = [",".join(results.keys())] + list(results.values())
    #for y in result:
    #    print(f" Result state {y.state}:  probability = {y.probability}")
    #print(retrunValue)
    return retrunValue

#qasm = "OPENQASM 2.0;\n\n//Allocate qubits and cbits\nqreg q[2];\ncreg c[2];\n\n//Define the circuit\nh q[0];\ncx q[0],q[1];\n\n//Measure\nmeasure q[0] -> c[0];\nmeasure q[1] -> c[1];"
#job = create_qat_job(qasm)
#submit_job(job)