//home/debian/venvs/qlm3-test/bin/python3
// Copyright 2024 science+computing AG / Eviden
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this
// file except in compliance with the License.  You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied.  See the License for the specific
// language governing permissions and limitations under the License.


#include <qdmi_backend.h>
#include <private/qdmi_internal.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <cJSON.h>

char * get_json_file_path();
int fileExists(const char *filename);
char * get_json_file_path();
char * get_job_status(const char *filedict_name, const char *task_id);
int key_exists(cJSON *json_object, const char *keyname);


#define CHECK_ERR(a, b)                          \
    {                                            \
        if (a != QDMI_SUCCESS)                   \
        {                                        \
            printf("\n[Error]: %i at %s", a, b); \
            return 1;                            \
        }                                        \
    }

// init not needed for Qaptiva backend
int QDMI_backend_init(QInfo info)
{
    printf("   [Backend].............Initializing Qaptiva IQM (simulator) via QDMI\n");

    char *uri = NULL;
    void *regpointer = NULL;
    int err;

    //err = QDMI_core_register_belib(uri, regpointer); // this function is not defined anywhere
    // CHECK_ERR(err, "QDMI_core_register_belib");

    return QDMI_SUCCESS;
}

// num classical bits in measurement, same as qubits. Why status needed?
int QDMI_control_readout_size(QDMI_Device dev, QDMI_Status *status, QDMI_Job job,
                              int *numbits)
{
    printf("   [Backend].............Returning size\n");
    
    *numbits = 5;
    return QDMI_SUCCESS;
}

// hardcoded coupling map as for each qubit, what are the two neighbours. Are QDMI qubits 1 indexed? Assuming yes and full connectivity for chip.
int QDMI_set_coupling_mapping(QDMI_Device dev, int qubit_index, QDMI_Qubit qubit)
{
    qubit->index = qubit_index;

    int i;
    switch (qubit_index) {
        case 0:
            qubit->coupling_mapping = (QDMI_qubit_index*)malloc(1 * sizeof(QDMI_qubit_index));
            qubit->coupling_mapping[0] = 2;
            qubit->size_coupling_mapping = 1;
            break;
        case 1:
            qubit->coupling_mapping = (QDMI_qubit_index*)malloc(1 * sizeof(QDMI_qubit_index));
            qubit->coupling_mapping[0] = 2;
            qubit->size_coupling_mapping = 1;
            break;
        case 2:
            qubit->coupling_mapping = (QDMI_qubit_index*)malloc(1 * sizeof(QDMI_qubit_index));
            qubit->coupling_mapping[0] = 0;
            qubit->coupling_mapping[1] = 1;
            qubit->coupling_mapping[2] = 3;
            qubit->coupling_mapping[3] = 4;
            qubit->size_coupling_mapping = 4;
            break;
        case 3:
            qubit->coupling_mapping = (QDMI_qubit_index*)malloc(1 * sizeof(QDMI_qubit_index));
            qubit->coupling_mapping[0] = 2;
            qubit->size_coupling_mapping = 1;
            break;
        case 4:
            qubit->coupling_mapping = (QDMI_qubit_index*)malloc(1 * sizeof(QDMI_qubit_index));
            qubit->coupling_mapping[0] = 2;
            qubit->size_coupling_mapping = 1;
            break;
        default:
            qubit->coupling_mapping = NULL;
            qubit->size_coupling_mapping = 0;
            break; 
    }
    return QDMI_SUCCESS;
}

// number of qubits
int QDMI_query_qubits_num(QDMI_Device dev, int *num_qubits)
{
    *num_qubits = 5;
    return QDMI_SUCCESS;
}

int QDMI_device_status(QDMI_Device dev, QInfo info, int *status){
    *status = 1;
    return QDMI_SUCCESS;
}

//why is QInfo and device given as argument here
int QDMI_control_submit(QDMI_Device dev, QDMI_Fragment *frag, int numshots, QInfo info, QDMI_Job *job)
{
    printf("   [Backend].............Circuit received\n");

    // Create a JSON object
    cJSON *json_object = cJSON_CreateObject();

    // Add information to JSON object that are needed for job submission
    cJSON *json_value_qasmstr = cJSON_CreateString((*frag)->qasmstr);
    cJSON_AddItemToObject(json_object, "qasm_string", json_value_qasmstr);

    size_t size_ns = snprintf(NULL, 0, "%d", numshots);
    char *num_shots_string = (char *)malloc(size_ns + 1);
    sprintf(num_shots_string, "%d", numshots);
    cJSON *json_value_numshots = cJSON_CreateString(num_shots_string);
    cJSON_AddItemToObject(json_object, "numshots", json_value_numshots);

    // Get the JSON string representation
    char *job_string = cJSON_Print(json_object);

    // Define filename using task_id
    const char *filedict_name = get_json_file_path();
    size_t size_tid = snprintf(NULL, 0, "%d", (*job)->task_id);
    char *task_id_str = (char *)malloc(size_tid + 1);
    sprintf(task_id_str, "%d", (*job)->task_id);
    char *filename = (char *)malloc(strlen(filedict_name) + strlen(task_id_str) + 6);  // +6 for '/' + '.json' + null terminator
    sprintf(filename, "%s/%s.json", filedict_name, task_id_str);

    free(num_shots_string);
    free(task_id_str);

    // Open a file for writing
    printf("%s\n%s\n", job_string, filename);
    FILE *job_file = fopen(filename, "w+");
    if (job_file == NULL) {
        perror("error");
        printf("jobfile cannot be created");
        free(filename);
        free(job_string);  // Free job_string since we're not using buffer anymore
        return QDMI_ERROR_BACKEND;
    }


    // Write the job string to the file
    size_t content_length = strlen(job_string);
    fwrite(job_string, sizeof(char), content_length, job_file);
    free(job_string);  // Free job_string after use

    fclose(job_file);

    // Clean up JSON object
    cJSON_Delete(json_object);

    //TODO: make path configurable
    // Start wrapper to submit job to Qaptiva
    //const char path_to_wrapper[] = "/home/debian/venvs/qlm3-test/bin/python3 /home/debian/C_Projects/q-exa-qdmi-interface/QaptivaWrapper-develop/qdmi2qaptiva.py";
    //const char source_path_python[] = "/home/debian/venvs/qlm3-test/bin/activate";

    char *qaptiva_wrapper_command = NULL;
    char *qaptiva_wrapper_command_default = "/home/debian/venvs/qlm3-test/bin/python3 /home/debian/C_Projects/q-exa-qdmi-interface/backends-develop/src/qaptiva/qdmi2qaptiva.py";
    char *qaptiva_wrapper_command_varname = "QAPTIVA_WRAPPER_COMMAND";
    qaptiva_wrapper_command = getenv(qaptiva_wrapper_command_varname);
    if (qaptiva_wrapper_command == NULL)
        qaptiva_wrapper_command = qaptiva_wrapper_command_default;

    char file_argument[1000];
    strcpy(file_argument, " ");
    strcat(file_argument, filename);

 
    //char source_command[1000];
    //strcpy(source_command, ". ");
    //strcat(source_command, source_path_python);


    char system_command[1000];
    strcpy(system_command, qaptiva_wrapper_command);
    strcat(system_command, file_argument);
    
    //char terminal_command[5000];
    //strcpy(terminal_command, source_command);
    //strcat(terminal_command, " && ");
    //strcat(terminal_command, wrapper_command);
    printf("%s\n", system_command);
    //system(system_command);

    free(filename);

    return QDMI_SUCCESS;
}

int QDMI_control_readout_raw_num(QDMI_Device dev, QDMI_Status *status, 
                                QDMI_Job job, int *num)
{
    printf("   [Backend].............Returning results\n");

    int err = 0, numbits = 0;
    err = QDMI_control_readout_size(dev, status, job, &numbits);
    CHECK_ERR(err, "QDMI_control_readout_raw_num");

    // Initialize the array to zeros
    for (unsigned int i = 0; i < (1U << numbits); i++)
        num[i] = 0;

    // Get filename using task_id
    char *filedict_name = get_json_file_path();
    int task_id = job->task_id;
    size_t size = snprintf(NULL, 0, "%d", task_id);
    char *task_id_str = (char *)malloc(size + 1);
    sprintf(task_id_str, "%d", task_id);
    char *filename = (char *)malloc(strlen(filedict_name) + strlen(task_id_str) + 6); // +6 due to .json and /
    sprintf(filename, "%s/%s.json", filedict_name ,task_id_str);

   
    // Open and read the JSON file
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening result file\n");
        return QDMI_ERROR_BACKEND;
    }

    // Read the entire JSON file into a buffer
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(length + 1);
    fread(data, 1, length, file);
    fclose(file);
    data[length] = '\0';  // Null-terminate the string

    // Parse the JSON
    cJSON *json_object = cJSON_Parse(data);
    if (!json_object) {
        printf("Error parsing JSON\n");
        free(data);
        return QDMI_ERROR_BACKEND;
    }

    // Extract the counts object
    cJSON *counts = cJSON_GetObjectItemCaseSensitive(json_object, "counts");
    if (!counts || !cJSON_IsObject(counts)) {
        printf("No valid 'counts' object found in the JSON\n");
        cJSON_Delete(json_object);
        free(data);
        return QDMI_ERROR_BACKEND;
    }

    // Iterate over the key-value pairs in the counts object
    cJSON *counts_object = NULL;
    cJSON_ArrayForEach(counts_object, counts) {
        const char *bitstring = counts_object->string;  // Get the key (bitstring)
        double counts_value = counts_object->valuedouble;  // Get the value (counts)

        if (bitstring && cJSON_IsNumber(counts_object)) {
            long bitstring_idx = strtol(bitstring, NULL, 2);  // Convert bitstring (binary) to an index
            if (bitstring_idx >= 0) {
                num[bitstring_idx] = counts_value;
            } else {
                printf("Invalid bitstring index: %ld\n", bitstring_idx);
                return QDMI_ERROR_BACKEND;

            }
        }
    }

    // Clean up
    cJSON_Delete(json_object);
    free(data);
    return QDMI_SUCCESS;
}

int QDMI_control_test(QDMI_Device dev, QDMI_Job *job, int *flag, QDMI_Status *status)
{
    printf("   [Backend].............Querying status\n");
    // Define filename using task_id
    char *filedict_name = get_json_file_path();
    if (filedict_name == NULL) {
        return QDMI_ERROR_BACKEND;
    }

    // Allocate memory for task_id_str
    size_t size = snprintf(NULL, 0, "%d", (*job)->task_id);
    char *task_id_str = (char *)malloc(size + 1);
    if (task_id_str == NULL) {
        return QDMI_ERROR_BACKEND;
    }
    sprintf(task_id_str, "%d", (*job)->task_id);
    // Get job status
    const char *job_status = get_job_status(filedict_name, task_id_str);
    free(task_id_str); // Free allocated memory for task_id_str

    if (job_status == NULL) {
        return QDMI_ERROR_BACKEND;
    }
    // Proper string comparison
    if (strcmp(job_status, "running") == 0) {
        (*flag) = QDMI_EXECUTING;
    }
    else if (strcmp(job_status, "completed") == 0) {
        (*flag) = QDMI_COMPLETE;
    }
    else {
        (*flag) = QDMI_HALTED; 
        return QDMI_ERROR_BACKEND;
    }

    return QDMI_SUCCESS;
}

int QDMI_control_wait(QDMI_Device dev, QDMI_Job *job, QDMI_Status *status)
{
    bool done = false;
    int flag = QDMI_EXECUTING;

    while (!done)
    {
        QDMI_control_test(dev, job, &flag, status);

        if (flag == QDMI_COMPLETE)
        {
            break;
        }
        else if (flag == QDMI_HALTED)
        {
            return QDMI_ERROR_BACKEND;
        }

        sleep(5);
    }
    return QDMI_SUCCESS;
}

int QDMI_control_pack_qir(QDMI_Device dev, void *qirmod, QDMI_Fragment *frag)
{
    (*frag)->qirmod = qirmod;

    return QDMI_SUCCESS;
}

int QDMI_control_pack_qasm2(QDMI_Device dev, char *qasmstr, QDMI_Fragment *frag)
{
    (*frag)->qasmstr = qasmstr;

    return QDMI_SUCCESS;
}

int key_exists(cJSON *json_object, const char *key_name) {
    cJSON *item = cJSON_GetObjectItem(json_object, key_name);
    return (!item);
}

char * get_json_file_path(){
    char *qaptiva_work_dir = NULL;
    char *qaptiva_work_dir_default = "/home/ubuntu/update/backend_update/jobfiles";

    char *qaptiva_work_dir_varname = "QAPTIVA_WORK_DIR";
    qaptiva_work_dir = getenv(qaptiva_work_dir_varname);

    if (qaptiva_work_dir == NULL)
        qaptiva_work_dir = qaptiva_work_dir_default;    
    return qaptiva_work_dir;
}

int fileExists(const char *filename){
    FILE *file = fopen(filename, "r");

    if (file){
        fclose(file);
        return 1;
    }

    return 0;
}

char *get_job_status(const char *filedict_name, const char *task_id) {
    // List of possible statuses
    char *status_list[] = {"running", "completed", "failed"};
    size_t number_of_statuses = sizeof(status_list) / sizeof(status_list[0]);

    // Determine the maximum length needed for filename
    size_t max_filename_len = strlen(filedict_name) + strlen(task_id) + 20; // +20 for slashes and statuses

    // Allocate memory for filename
    char *filename = (char *)malloc(max_filename_len);
    if (filename == NULL) {
        return "failed"; // or handle memory allocation failure appropriately
    }

    for (size_t i = 0; i < number_of_statuses; i++) {
        // Format filename with the current status
        snprintf(filename, max_filename_len, "%s/%s.%s", filedict_name, task_id, status_list[i]);

        // Check if the file exists
        if (fileExists(filename)) {
            free(filename); // Free allocated memory before returning
            return status_list[i];
        }
    }

    // Free the allocated memory if no status matched
    free(filename);
    return "failed";
}