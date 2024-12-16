
#include <qdmi/device.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



//int QDMI_session_init(QInfo info, QDMI_Session *session);
//int QDMI_session_finalize(QDMI_Session session);
//QDMI_Library find_library_by_name(const char *libname);

#define CHECK_ERR(a, b)                          \
    {                                            \
        if (a != QDMI_SUCCESS)                   \
        {                                        \
            printf("\n[Error]: %i at %s", a, b); \
            return 1;                            \
        }                                        \
    }

int main(int argc, char **argv)
{

    QDMI_control_initialize_dev();
    QDMI_Job theJob;

    char* circuit = "OPENQASM 2.0;\n\n//Allocate qubits and cbits\nqreg q[2];\ncreg c[2];\n\n//Define the circuit\nh q[0];\ncx q[0],q[1];\n\n//Measure\nmeasure q[0] -> c[0];\nmeasure q[1] -> c[1];";
    int err = QDMI_control_create_job_dev(QDMI_PROGRAM_FORMAT_QASM2, strlen(circuit), circuit, &theJob);

    printf("%d\n", err);
    const size_t n_shots = 1024;
    err = QDMI_control_set_parameter_dev(theJob, QDMI_JOB_PARAMETER_SHOTS_NUM, sizeof(const size_t), &n_shots);

    printf("%d\n", err);
    err = QDMI_control_submit_job_dev(theJob);

    err = QDMI_control_wait_dev(theJob);
    size_t size = 512;
    void* result = malloc(size);
    size_t ret_size;
    err = QDMI_control_get_data_dev(theJob, QDMI_JOB_RESULT_PROBABILITIES_DENSE, size, result, &ret_size);

    return 0;
}