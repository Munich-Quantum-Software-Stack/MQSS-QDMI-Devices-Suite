#include <qdmi_backend.h>
#include <private/qdmi_internal.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <qdmi.h>
#include <qinfo.h>


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
    

    QInfo info;
    QDMI_Session session;
    //QDMI_Job job;
    int err, count = 0;

    
    err = QInfo_create(&info);
    if QDMI_IS_ERROR(err) return err;
    
    err = QDMI_session_init(info, &session);
    
    int device_count = -1;
    
    err = QDMI_core_device_count(&session, &device_count);
    if QDMI_IS_ERROR(err) return err;


    QDMI_Device devices[device_count];
    for(int index = 0; index < device_count; index++)
        QDMI_core_open_device(&session, index, &info, &devices[index]);

    QDMI_Device device = devices[0];


    FILE *file;
    char *buffer;
    long file_size;

    file = fopen("./inputs/bell.qasm", "r");
    if (file == NULL){
        perror("Error opening qasm file");
        return 1;
    }

    //find the file size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    //allocate memory for the buffer to hold the file content
    buffer = (char *)malloc(sizeof(char) * (file_size+1));
    if (buffer == NULL){
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }

    fread(buffer, sizeof(char), file_size, file);
    buffer[file_size] = '\0'; //null-terminate the string

    fclose(file);
    QDMI_Fragment frag = (QDMI_Fragment)malloc(sizeof buffer);
    QDMI_control_pack_qasm2(device, buffer,  &frag);

    if (frag == NULL){
    	printf("\n[ERROR]: The fragment could not be created");
        exit(EXIT_FAILURE);
    }

    int num_qubits;
    err = QDMI_query_qubits_num(device, &num_qubits);
    CHECK_ERR(err, "QDMI_query_qubits_num");

    int status = 0;
    err = QDMI_device_status(device, info, &status);
    CHECK_ERR(err, "QDMI_device_status");

    // job_id as random int
    QDMI_Job job = (QDMI_Job)malloc(100*(sizeof(char)));
    srand(time(NULL));
    int task_id = rand();
    job->task_id = task_id;
    err = QDMI_control_submit(device, &frag, 1024, info, &job);
    CHECK_ERR(err, "QDMI_control_submit");

    int wait_status = QDMI_control_wait(device, &job, &status);

    // init results array
    int state_space = 1;
    for (int i=0; i<num_qubits; i++)
    {
	state_space *=2;
    }
    
   int *num = malloc(state_space * sizeof(double)); 
    if (num == NULL) {
        printf("Memory allocation failed\n");
        return -1; // Handle allocation failure
    }
    for(int i=0; i<state_space; i++)
    {
	num[i] = 0;
    }

    sleep(5);

    err = QDMI_control_readout_raw_num(device, &status, task_id, num);
    CHECK_ERR(err, "QDMI_control_readout_raw_num");

    for (int i=0; i<state_space; i++)
    {
	printf("Measurement: %i counts: %i\n", i, num[i]);
    }

    CHECK_ERR(err, "QDMI_session_finalize");

    err = QInfo_free(info);
    CHECK_ERR(err, "QInfo_free");

    free(frag);
    free(device);
    free(job);

    printf("\n[DEBUG]: Test finished\n\n");

    return 0;
}
