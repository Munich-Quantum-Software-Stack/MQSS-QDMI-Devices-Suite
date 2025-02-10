OPENQASM 2.0;

//Allocate qubits and cbits
qreg q[2];
creg c[2];

//Define the circuit
h q[0];
cx q[0],q[1];

//Measure
measure q[0] -> c[0];
measure q[1] -> c[1];
