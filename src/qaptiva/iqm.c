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
int QDMI_control_readout_size(QDMI_Device dev, QDMI_Status *status, int *numbits)
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
    FILE *job_file = fopen(filename, "wb");
    if (job_file == NULL) {
        free(filename);
        free(job_string);  // Free job_string since we're not using buffer anymore
        return QDMI_ERROR_BACKEND;
    }

    free(filename);

    // Write the job string to the file
    size_t content_length = strlen(job_string);
    fwrite(job_string, sizeof(char), content_length, job_file);
    free(job_string);  // Free job_string after use

    fclose(job_file);

    // Clean up JSON object
    cJSON_Delete(json_object);

    return QDMI_SUCCESS;
}

int QDMI_control_readout_raw_num(QDMI_Device dev, QDMI_Status *status, int task_id, int *num)
{
    printf("   [Backend].............Returning results\n");

    int err = 0, numbits = 0;

    err = QDMI_control_readout_size(dev, status, &numbits);
    CHECK_ERR(err, "QDMI_control_readout_raw_num");

    // Initialize the array to zeros
    for (unsigned int i = 0; i < (1U << numbits); i++)
        num[i] = 0;

    // Get filename using task_id
    char *filedict_name = get_json_file_path();
    size_t size = snprintf(NULL, 0, "%d", task_id);
    char *task_id_str = (char *)malloc(size + 1);
    sprintf(task_id_str, "%d", task_id);
    char *filename = (char *)malloc(strlen(filedict_name) + strlen(task_id_str) + 6); // +6 due to .json and /
    sprintf(filename, "%s/%s.json", filedict_name, task_id_str);

    // Open JSON file
    FILE *job_file = fopen(filename, "r");
    if (!job_file) {
        free(filename);
        printf("   [Backend].............Job not done\n");
        return QDMI_ERROR_BACKEND;
    }

    // Get the file size and allocate memory
    fseek(job_file, 0, SEEK_END);
    long file_size = ftell(job_file);
    rewind(job_file);

    char *job_data = (char *)malloc(file_size + 1);
    if (job_data == NULL) {
        perror("Memory allocation failed");
        fclose(job_file);
        free(filename);
        return QDMI_ERROR_BACKEND;
    }

    // Read data from JSON file
    size_t read_size = fread(job_data, 1, file_size, job_file);
    job_data[file_size] = '\0';
    fclose(job_file);
    free(filename);

    if (read_size != file_size) {
        perror("File read failed");
        free(job_data);
        return QDMI_ERROR_BACKEND;
    }

    // Parse the JSON data
    cJSON *json_object = cJSON_Parse(job_data);
    free(job_data);

    if (json_object == NULL) {
        printf("   [Backend].............Error parsing JSON\n");
        return QDMI_ERROR_BACKEND;
    }

    // Iterate over JSON and write probabilities into num array
    cJSON *probability_array = cJSON_GetObjectItemCaseSensitive(json_object, "probabilities");
    if (probability_array && cJSON_IsArray(probability_array)) {
        cJSON *probability_object = NULL;
        cJSON_ArrayForEach(probability_object, probability_array) {
            char *bitstring_string = probability_object->string;
            long bitstring_idx = strtol(bitstring_string, NULL, 0);
            int amount = cJSON_GetNumberValue(probability_object);
            num[bitstring_idx] = amount;
        }
    } else {
        printf("   [Backend].............Job not done\n");
        cJSON_Delete(json_object);
        return QDMI_ERROR_BACKEND;
    }

    cJSON_Delete(json_object);
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
    
    return "./jobfiles";
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
