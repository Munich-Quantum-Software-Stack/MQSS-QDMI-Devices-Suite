#!/home/debian/venvs/qlm3-test/bin/python3
# Copyright 2024 science+computing AG / Eviden
# 
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this
# file except in compliance with the License.  You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software distributed
# under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied.  See the License for the specific
# language governing permissions and limitations under the License.

import sys
import os
import time
import getopt
import json
import copy
import re

from qat.core.qpu import RemoteQPU
from qat.interop.openqasm import OqasmParser

global DEBUG
DEBUG = False

global RELEASE_VERSION, RELEASE_DATE
RELEASE_VERSION = "0.3"
RELEASE_DATE = "2024-08-21"

global WORKDIR

if 'QAPTIVA_WORK_DIR' in os.environ:
    WORKDIR = os.environ['QAPTIVA_WORK_DIR']
else:
    sys.exit('The environment variable QAPTIVA_WORK_DIR has to be defined!')


global DEFAULT_REMOTEQPU_PORT, DEFAULT_REMOTEQPU_HOSTNAME
DEFAULT_REMOTEQPU_PORT = 20500
DEFAULT_REMOTEQPU_HOSTNAME = 'qlm3.for.lrz.de'

global CIRCUIT_FILENAME
CIRCUIT_FILENAME = None

global STATUS_LIST
STATUS_LIST = [
    'running',
    'waiting',
    'failed',
    'completed',
]

global ERROR_MESSAGES
ERROR_MESSAGES = {
    'no_input_file_on_command_line': 'No input file specified on command line.',
    'circuit_file_read_error': 'Circuit file not readable or no proper json file.',
    'qasm_parser_error': 'Parser error - input file does not provide a valid openQASM string.',
    'remoteqpu_connection_error': 'Connection to RemoteQPU on Qaptiva Appliance failed.',
    'job_submit_error': 'Job submission to RemoteQPU on Qaptiva Appliance failed.',
    'circuit_file_write_error': 'Circuit file could not be written to disk.',
    'status_file_write_error': 'New status file could not be written to disk.',
    'status_file_delete_error': 'Old status file could not be deleted from disk.',
    'chdir_failed': 'Changing workig directory failed.',
}


def main():
    circuit_filename, remote_qpu, args, input_file_is_openqasm = parse_commandline()
    change_to_workdir(WORKDIR)
    swap_status('running')
    json_data_raw = read_circuit_file(circuit_filename, input_file_is_openqasm)
    job_data, circuit = parse_circuit(json_data_raw)
    job = create_job(job_data, circuit)
    result = submit_job(job, remote_qpu)
    success = write_circuit_file(result, json_data_raw, circuit_filename)
    sys.exit(success)


def print_usage():
    print(
        f'''_______________________________________________________________________________________________\n
{os.path.basename(sys.argv[0])} - send qasm quantum circuit to Qaptiva and write back simulated results

    Usage:  {os.path.basename(sys.argv[0])} [options] <circuit_file>

                <circuit_file> \t name of json file with qasm circuit definition; this file
                               \t serves as input AND output file!

    Options:    -h/--help      \t print this information
                -d/--debug     \t print debugging information to stderr
                -r/--remoteqpu \t <hostname>:<port>
                               \t Default: "{DEFAULT_REMOTEQPU_HOSTNAME}:{DEFAULT_REMOTEQPU_PORT}"
                -o/--openqasm  \t <circuit_file> is OpenQASM formatted instead of JSON
                               \t output will be written to <circuit_file_basename>.json

    Release {RELEASE_VERSION} ({RELEASE_DATE})
_______________________________________________________________________________________________
  Copyright 2024 science+computing AG / Eviden - Licensed under the Apache License, Version 2.0\n'''
    )


def parse_commandline():
    global DEBUG
    global CIRCUIT_FILENAME
    global DEFAULT_REMOTEQPU_PORT
    global DEFAULT_REMOTEQPU_HOSTNAME
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hdr:o", ["help", "debug", "--remoteqpu=", "--openqasm"])
    except getopt.GetoptError:
        print_usage()
        sys.exit(2)

    remote_qpu = (DEFAULT_REMOTEQPU_PORT, DEFAULT_REMOTEQPU_HOSTNAME)
    input_file_is_openqasm = False

    for o, a in opts:
        if o in ("-h", "--help"):
            print_usage()
            sys.exit(0)
        elif o in ("-d", "--debug"):
            DEBUG = True
        elif o in ("-r", "--remoteqpu"):
            parted = a.partition(':')
            remote_qpu = (parted[2], parted[0])
        elif o in ("-o", "--openqasm"):
            input_file_is_openqasm = True

    try:
        CIRCUIT_FILENAME = args[0]
    except IndexError:
        print_usage()
        eprint('no_input_file_on_command_line', 3)

    dprint("circuit_filename", CIRCUIT_FILENAME)
    return CIRCUIT_FILENAME, remote_qpu, args, input_file_is_openqasm


def change_to_workdir(workdir=None):
    dprint('old cwd', os.getcwd())
    try:
        if workdir:
            os.chdir(workdir)
    except:
        _type, _value, _traceback = sys.exc_info()
        eprint('chdir_failed', 1, _value)

    dprint('new cwd', os.getcwd())


def read_circuit_file(filename, input_file_is_openqasm):
    '''Example:
           {
               "qasm_string": "here goes the qasm circuit as string",
               "num_shots": 100000,
               "probabilities": {
                    "00": 0.4,
                    "01": 0.05,
                    "10": 0.05,
                    "11": 0.4
               }
               "counts": {
                    "00": 49999,
                    "01": 0,
                    "10": 0,
                    "11": 49999
               }
           }
    '''
    try:
        with open(filename, 'r') as circuit_file:
            if input_file_is_openqasm:
                json_data_raw = {
                    'qasm_string': circuit_file.read()
                }
            else:
                json_data_raw = json.loads(circuit_file.read())
    except:
        _type, _value, _traceback = sys.exc_info()
        eprint('circuit_file_read_error', 1, _value)

    dprint('json_data_raw', json_data_raw)
    return json_data_raw


def parse_circuit(json_data_raw):
    job_data = copy.deepcopy(json_data_raw)
    parser = OqasmParser()
    try:
        circuit = parser.compile(json_data_raw['qasm_string'])
        if not circuit:
            raise Exception('openqasm parser failed')
    except:
        eprint('qasm_parser_error', 1)

    del(job_data['qasm_string'])
    dprint('job_data', job_data)
    dprint('circuit', [x for x in circuit.iterate_simple()])
    return job_data, circuit


def create_job(job_data, circuit):
    job = circuit.to_job()
    return job


def create_remote_qpu(remote_qpu):
    try:
        qpu = RemoteQPU(int(remote_qpu[0]), str(remote_qpu[1]))
    except:
        _type, _value, _traceback = sys.exc_info()
        eprint('remoteqpu_connection_error', 1, _value)

    return qpu


def submit_job(job, remote_qpu):
    dprint(f'Connecting to RemoteQPU {remote_qpu}...')
    qpu = create_remote_qpu(remote_qpu)
    try:
        result = qpu.submit(job)
    except:
        _type, _value, _traceback = sys.exc_info()
        eprint('job_submit_error', 1, _value)

    for y in result:
        dprint(f" Result state {y.state}:  probability = {y.probability}")

    return result


def write_circuit_file(result, json_data_raw, circuit_file):
    nb_shots = 100000

    basename = os.path.splitext(os.path.basename(circuit_file))[0]
    out_file = basename + '.json'
    write_data = copy.deepcopy(json_data_raw)
    write_data['probabilities'] = {}
    write_data['counts'] = {}
    for y in result:
        _state = str(y.state)[1:-1]
        write_data['probabilities'][_state] = y.probability
        write_data['counts'][_state] = int(y.probability*nb_shots)

    dprint('write_data', write_data)
    dprint('out_file', out_file)

    try:
        CF = open(out_file, 'w')
        json.dump(write_data, CF, sort_keys=True, indent=4)
        CF.close()
    except:
        _type, _value, _traceback = sys.exc_info()
        eprint('circuit_file_write_error', 1, _value)

    swap_status('completed')
    return 0


def swap_status(new_status):
    if CIRCUIT_FILENAME:
        basename = os.path.splitext(os.path.basename(CIRCUIT_FILENAME))[0]
    else:
        return False

    try:
        for s in STATUS_LIST:
            if os.path.islink(basename+'.'+s):
                os.unlink(basename+'.'+s)
    except:
        _type, _value, _traceback = sys.exc_info()
        eprint('status_file_delete_error', 1, _value)

    dest_file = basename + '.' + new_status
    if new_status == 'completed':
        src_file = basename + '.json'
    else:
        src_file = CIRCUIT_FILENAME

    try:
        os.symlink(src_file, dest_file)
    except:
        _type, _value, _traceback = sys.exc_info()
        eprint('status_file_write_error', 1, _value)

    dprint('New status link created', dest_file)


def eprint(message, rc=1, traceback=''):
    """Print error message and exit."""
    if message in ERROR_MESSAGES:
        out = ERROR_MESSAGES[message]
    else:
        out = message

    if rc == 0:
        print(f'''\nWARNING: {out}''', file=sys.stderr)
    else:
        swap_status('failed')
        print(f'''\t {traceback}\n\nERROR:\t {out}  Exit {rc}.\n''', file=sys.stderr)
        sys.exit(rc)


def dprint (*debug_var):
    """Print debugging messages - if "DEBUG" exists and True."""
    try:
        if DEBUG:
            print (
                "%s\t%s%s" %(
                    time.time(),
                    sys._getframe(1).f_code.co_name,
                    debug_var,
                ),
                file=sys.stderr
            )
    except:
        pass


if __name__ == '__main__':
   main()