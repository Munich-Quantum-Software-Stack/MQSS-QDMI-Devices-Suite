# Copyright 2024 Munich Quantum Software Stack Project
#
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
# "License"); you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/blob/develop/LICENSE
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------



## @file qlm_auxiliary.py
#  @brief The Custom Module for handling the connection and submitting jobs.


def create_remote_qpu(host):
    """! Creates a remote QPU connection.

    @param host Hostname and port in the form "host:port".
    @return A RemoteQPU instance or None if the connection fails.
    """
    try:
        from qat.core.qpu import RemoteQPU
        url, port = host.split(":")
        qpu = RemoteQPU(port, url)
    except:
        return None
    return qpu


def submit_job(remote_qpu, qasm_string, nshots):
    """! Submits a quantum job to a remote QPU.

    @param remote_qpu A RemoteQPU instance.
    @param qasm_string QASM code string.
    @param nshots Number of shots to run the job.
    @return A list containing measurement states and their probabilities, or None if submission fails.
    """
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
