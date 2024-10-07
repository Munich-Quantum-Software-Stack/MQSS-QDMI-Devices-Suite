def create_aqt_job_from_qir(token , qir_bitcode: bytes, shots: int):
    from qir_qiskit.translate import to_qiskit_circuit
    from qiskit_aqt_provider.aqt_provider import AQTProvider
    from qiskit import transpile
    from qiskit_aqt_provider.circuit_to_aqt import circuits_to_aqt_job

    qiskit_circuit = to_qiskit_circuit(qir_bitcode)
    qiskit_circuit.data = [
        instruction for instruction in qiskit_circuit.data
        if instruction.operation.name != "reset"
    ]

    provider = AQTProvider(token)
    backend = provider.get_backend("marmot", workspace="lrz")

    transpiled_circuit = transpile(qiskit_circuit, backend)
    job = circuits_to_aqt_job([transpiled_circuit], shots)
    
    return job.json()
