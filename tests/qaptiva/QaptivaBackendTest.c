
#include <Python.h>
#include <qdmi/device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {

  // submit_job(NULL);

  QDMI_Job theJob = NULL;

  char *circuit =
      "OPENQASM 2.0;\n\n//Allocate qubits and cbits\nqreg q[2];\ncreg "
      "c[2];\n\n//Define the circuit\nh q[0];\ncx "
      "q[0],q[1];\n\n//Measure\nmeasure q[0] -> c[0];\nmeasure q[1] -> c[1];";
  int err = QDMI_control_initialize_dev();

  err = QDMI_control_create_job_dev(QDMI_PROGRAM_FORMAT_QASM2, strlen(circuit),
                                    circuit, &theJob);

  const size_t n_shots = 1024;
  err = QDMI_control_set_parameter_dev(theJob, QDMI_JOB_PARAMETER_SHOTS_NUM,
                                       sizeof(const size_t), &n_shots);

  err = QDMI_control_submit_job_dev(theJob);

  err = QDMI_control_wait_dev(theJob);

  size_t size = 512;
  void *result = malloc(size);
  size_t ret_size;
  err = QDMI_control_get_data_dev(theJob, QDMI_JOB_RESULT_PROBABILITIES_DENSE,
                                  size, result, &ret_size);

  QDMI_control_finalize_dev();

  double *_result = (double *)(result);
  printf("%f\n", _result[0]);

  return 0;
}