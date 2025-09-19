/*------------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

/** @file
 * @brief The QDMI Device implementation of the Qaptiva Device at Leibniz
 * Supercomputing Centre.
 * @details Qaptiva enables enterprises and institutions to anticipate the
 * availability of advanced quantum computers and implement real-world use cases
 * by harnessing innovation through quantum computing technology. This device
 * implementation allows a `QDMI Client` to submit quantum job, query the device
 * information.
 * @note We need to use Eviden's Python package named myQLM to submit job and
 * connect to the device. Therefore, we obligated to use Python C API to call
 * Python functions.
 */

#include <Python.h>
#include <qaptiva_qdmi/device.h>
#include <qaptiva_qdmi/types.h>

/** @brief The definition for the auxiliary script's location.
 * @details `QAPTIVA_AUXILIARY_SCRIPT_LOCATION` is an environment variable that
 * corresponse to location of the `qlm_auxiliary.py`. In this project, it should
 * be {PROJECT_SOURCE_DIR}/src/eviden/
 */
#define SCRIPT_LOCATION "QAPTIVA_AUXILIARY_SCRIPT_LOCATION"

/** @brief The file name of the auxiliary script.
 * @details `QAPTIVA_AUXILIARY_SCRIPT_NAME` is an environment variable that
 * corresponse to the file name of the auxiliary script. In this project, it
 * should be `qlm_auxiliary`
 */
#define SCRIPT_NAME "QAPTIVA_AUXILIARY_SCRIPT_NAME"

/// The name of the function that used to create remote qpu.
#define REMOTE_QPU_CREATE_FUNCTION_NAME "create_remote_qpu"

/// The name of the function that used to submit quantum job.
#define SUBMIT_JOB_FUNCTION_NAME "submit_job"

/// The name of the function that used to submit quantum job.
#define SUBMIT_NOISY_JOB_FUNCTION_NAME "submit_noisy_job"

/**
 * @brief Enum of the session status that can be set internally.
 *
 * @see QAPTIVA_QDMI_device_session_alloc
 * @see QAPTIVA_QDMI_device_session_init
 */
enum QAPTIVA_QDMI_DEVICE_SESSION_STATUS {
  /// The session is allocated but not yet initialized
  ALLOCATED,
  /// The session is successfully initialized
  INITIALIZED
};

/**
 * @brief The implementation of the encapsulated type
 * QDMI_Device_Session on the device-side.
 * @details Implemented to hold all the required configuration to
 connect the Qaptiva device at LRZ.
 * @note QDMI_Device_Session is encapsulated in the QDMI Device Session
 * Interface to allow the device implement the type as needed.
 */
typedef struct QAPTIVA_QDMI_Device_Session_impl_d {

  /// The url of the Qaptiva device's host.
  char *url;

  /// Flag to indicate if this is a noisy session (HTTP-based)
  int *is_noisy_session;

  /// Current status of the session
  enum QAPTIVA_QDMI_DEVICE_SESSION_STATUS status;
} QAPTIVA_QDMI_Device_Session_impl_t;

/**
 * @brief Device-side implementation of the encapsulated type
 * `QDMI_Device_Job`.
 *
 * @details This struct holds all necessary inputs to submit a
 * `QDMI_Device_Job` to the device, as well as the results.
 *
 * @note The `QDMI_Device_Job` type is encapsulated within the
 * `QDMI Device Job Interface` interface. This design allows devices to
 * implement the job type in a device-specific manner. This struct represents
 * the opaque pointer type used on the device side, encapsulating all
 * information required for a job.
 */

typedef struct QAPTIVA_QDMI_Device_Job_impl_d {
  /// Pointer to the session used for authentication and connection metadata.
  QAPTIVA_QDMI_Device_Session session;

  /// The ID of the job, randomly created @see
  /// QAPTIVA_QDMI_device_session_create_device_job.
  int id;

  /// Status of the `QDMI_Job`.
  QDMI_Job_Status status;

  /// Status of the `QDMI_Job`.
  size_t *num_shots;

  /// The <a
  /// href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a475336f0c08bd0218dd76a6016098231">
  /// format </a> of the submitted program. Currently, only QASM2 is supported.
  QDMI_Program_Format *format;

  /// The program to be executed
  char *program;

  /// The probabilities of the result.
  double *probability_dense;

  /// The keys for the sparse probabilities of the result.
  char *probability_keys;

  /// The values for the sparse probabilities of the result.
  double *probability_values;

  /// The number of the possible states of the results.
  size_t *n_state;

  /// The size of the results.
  size_t *results_size;

  double *t1;

  double *t2;

} QAPTIVA_QDMI_Device_Job_impl_t;

/**
 * @brief The implementation of the encapsulated type
 * QDMI_Site on the device-side.
 * @details In this backend, `QDMI_Site` corresponses to the qubits of the
 * Qaptiva device at the LRZ. Even though variables like T1 and T2 is expected
 * in the definition, since the device is an emulator, the variables are not
 * available.
 */
typedef struct QAPTIVA_QDMI_Site_impl_d {
  /// The ID of the QDMI_Site
  size_t id;

} QAPTIVA_QDMI_Site_impl_t;

/// The array of the sites
const QAPTIVA_QDMI_Site DEVICE_SITES[] = {
    &(QAPTIVA_QDMI_Site_impl_t){0},  &(QAPTIVA_QDMI_Site_impl_t){1},
    &(QAPTIVA_QDMI_Site_impl_t){2},  &(QAPTIVA_QDMI_Site_impl_t){3},
    &(QAPTIVA_QDMI_Site_impl_t){4},  &(QAPTIVA_QDMI_Site_impl_t){5},
    &(QAPTIVA_QDMI_Site_impl_t){6},  &(QAPTIVA_QDMI_Site_impl_t){7},
    &(QAPTIVA_QDMI_Site_impl_t){8},  &(QAPTIVA_QDMI_Site_impl_t){9},
    &(QAPTIVA_QDMI_Site_impl_t){10}, &(QAPTIVA_QDMI_Site_impl_t){11},
    &(QAPTIVA_QDMI_Site_impl_t){12}, &(QAPTIVA_QDMI_Site_impl_t){13},
    &(QAPTIVA_QDMI_Site_impl_t){14}, &(QAPTIVA_QDMI_Site_impl_t){15},
    &(QAPTIVA_QDMI_Site_impl_t){16}, &(QAPTIVA_QDMI_Site_impl_t){17},
    &(QAPTIVA_QDMI_Site_impl_t){18}, &(QAPTIVA_QDMI_Site_impl_t){19},
    &(QAPTIVA_QDMI_Site_impl_t){20}, &(QAPTIVA_QDMI_Site_impl_t){21},
    &(QAPTIVA_QDMI_Site_impl_t){22}, &(QAPTIVA_QDMI_Site_impl_t){23},
    &(QAPTIVA_QDMI_Site_impl_t){24}, &(QAPTIVA_QDMI_Site_impl_t){25},
    &(QAPTIVA_QDMI_Site_impl_t){26}, &(QAPTIVA_QDMI_Site_impl_t){27},
    &(QAPTIVA_QDMI_Site_impl_t){28}, &(QAPTIVA_QDMI_Site_impl_t){29},
    &(QAPTIVA_QDMI_Site_impl_t){30}, &(QAPTIVA_QDMI_Site_impl_t){31},
    &(QAPTIVA_QDMI_Site_impl_t){32}, &(QAPTIVA_QDMI_Site_impl_t){33},
    &(QAPTIVA_QDMI_Site_impl_t){34}, &(QAPTIVA_QDMI_Site_impl_t){35},
    &(QAPTIVA_QDMI_Site_impl_t){36}, &(QAPTIVA_QDMI_Site_impl_t){37},

};

/**
 * @brief Adds a string property to the device.
 *
 * This macro copies the value of the `prop_value` to the `value` if `prop_name`
 * and the `prop` matches. If specified, the required size is written
 * in `size_ret`.
 *
 * @param prop_name The name of the property
 * @param prop_value The string value of the property (`char*`)
 * @param prop The property name passed as input
 * @param size The size of the memory pointed to by `value`.
 * @param value Pointer to the memory location where `prop_value` will be
 * copied.
 * @param size_ret If specified, the required size is written.
 *
 * @return If successful, <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>.
 * If the `size` is not sufficient, <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 *
 * @note This value is taken adapted from <a
 * href="https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/3aec97c03c714584c498cbe6b3acff7d0df1bd79/examples/device/c/device.c#L119-L135">the
 * example C device</a> in the QDMI repository.
 *
 */

#define ADD_STRING_PROPERTY(prop_name, prop_value, prop, size, value,          \
                            size_ret)                                          \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != NULL) {                                                   \
        if ((size) < strlen(prop_value) + 1) {                                 \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        strncpy((char *)(value), prop_value, (size) - 1);                      \
        ((char *)(value))[(size) - 1] = '\0';                                  \
      }                                                                        \
      if ((size_ret) != NULL) {                                                \
        *(size_ret) = strlen(prop_value) + 1;                                  \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }
/**
 * @brief Adds a primitive typed value property to the device.
 *
 * This macro copies the value of the `prop_value` to the `value` if `prop_name`
 * and the `prop` matches. If specified, the required size is written
 * in `size_ret`.
 *
 * @param prop_name The name of the property
 * @param prop_type The type of the property, i.e, `char`, `ìnt`.
 * @param prop_value The value of the property.
 * @param prop The property name passed as input
 * @param size The size of the memory pointed to by `value`.
 * @param value Pointer to the memory location where `prop_value` will be
 * copied.
 * @param size_ret If specified, the required size is written.
 *
 * @return If successful, <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>.
 * If the `size` is not sufficient, <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 *
 * @note This value is taken adapted from <a
 * href="https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/3aec97c03c714584c498cbe6b3acff7d0df1bd79/examples/device/c/device.c#L102-L117">the
 * example C device</a> in the QDMI repository.
 *
 */
#define ADD_SINGLE_VALUE_PROPERTY(prop_name, prop_type, prop_value, prop,      \
                                  size, value, size_ret)                       \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != NULL) {                                                   \
        if ((size) < sizeof(prop_type)) {                                      \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        *(prop_type *)(value) = prop_value;                                    \
      }                                                                        \
      if ((size_ret) != NULL) {                                                \
        *(size_ret) = sizeof(prop_type);                                       \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }
/**
 * @brief Adds a list typed value property to the device.
 *
 * This macro copies the value of the `prop_value` to the `value` if `prop_name`
 * and the `prop` matches. If specified, the required size is written
 * in `size_ret`.
 *
 * @param prop_name The name of the property
 * @param prop_type The type of the property, i.e, `double`, `ìnt`.
 * @param prop_values The values of the property.
 * @param prop The property name passed as input
 * @param size The size of the memory pointed to by `value`.
 * @param value Pointer to the memory location where `prop_value` will be
 * copied.
 * @param size_ret If specified, the required size is written.
 *
 * @return If successful, <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>.
 * If the `size` is not sufficient, <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 *
 * @note This value is taken adapted from <a
 * href="
https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/3aec97c03c714584c498cbe6b3acff7d0df1bd79/examples/device/c/device.c#L137-L153
">the
 * example C device</a> in the QDMI repository.
 *
 */
#define ADD_LIST_PROPERTY(prop_name, prop_type, prop_values, prop_length,      \
                          prop, size, value, size_ret)                         \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != NULL) {                                                   \
        if ((size) < (prop_length) * sizeof(prop_type)) {                      \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        memcpy((void *)(value), (const void *)(prop_values),                   \
               (prop_length) * sizeof(prop_type));                             \
      }                                                                        \
      if ((size_ret) != NULL) {                                                \
        *(size_ret) = (prop_length) * sizeof(prop_type);                       \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }
/** @brief Checks if there is a Python related error.
 *
 * This macro checks whether the `value`is `NULL` or `Py_None`. If it is, it
 * returns the `ret_val`.
 *
 * @param value The value to be checked
 * @param from_python True means that the `QDMI_Driver` that initiate the device
 * is written in Python. Therefore, we don't need to release the GIL.
 * @return if the `value`is `NULL` or `Py_None`, it returns the `ret_val`
 */
#define CHECK_PYTHON_ERROR(value, from_python)                                 \
  {                                                                            \
    if (value == Py_None || value == NULL) {                                   \
                                                                               \
      PyErr_Print();                                                           \
      if (!from_python) {                                                      \
        PyGILState_Release(gstate);                                            \
      }                                                                        \
      QAPTIVA_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);                 \
      return QDMI_ERROR_FATAL;                                                 \
    }                                                                          \
  }

/** @brief Checks whether the `value` is `QDMI_SUCCESS`.
 *
 * @param value The value to be checked
 * @return `value` if it is not `QDMI_SUCCESS`
 */
#define CHECK_QDMI_ERROR(value)                                                \
  {                                                                            \
    if (value != QDMI_SUCCESS) {                                               \
      return value;                                                            \
    }                                                                          \
  }
/**
 * @brief Writes and casts results to the `data_ptr`
 *
 * @param type The type to results will be cast.
 * @param multiplier The value that results will be multiplied by
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 */
#define GET_VALUE_DATA(type, multiplier)                                       \
  {                                                                            \
    required_size = *(job->results_size) * sizeof(type);                       \
    if (data) {                                                                \
      if (size < required_size)                                                \
        return QDMI_ERROR_INVALIDARGUMENT;                                     \
      type *data_ptr = data;                                                   \
      int max_index = (int)(size / sizeof(type));                              \
      for (int index = 0; index < max_index; index++) {                        \
        *data_ptr++ = (type)(job->probability_values[index] * multiplier);     \
      }                                                                        \
    }                                                                          \
    if (size_ret)                                                              \
      *size_ret = required_size;                                               \
    return QDMI_SUCCESS;                                                       \
  }

/**
 * @brief Static function to maintain the device status.
 * @return A pointer to the device status.
 * @note This function should not be used outside of this file. Therefore, it is
 * not part of any header file.
 */
static QDMI_Device_Status *QAPTIVA_QDMI_get_device_status(void) {
  static QDMI_Device_Status device_status = QDMI_DEVICE_STATUS_OFFLINE;
  return &device_status;
}

/**
 * @brief The local function to read the device status.
 * @return The current device status.
 * @note This function should not be used outside of this file. Therefore, it is
 * not part of any header file.
 */
QDMI_Device_Status QAPTIVA_QDMI_read_device_status(void) {
  return *QAPTIVA_QDMI_get_device_status();
}
/**
 * @brief The local function to set the device status.
 * @param status the new device status.
 * @note This function should not be used outside of this file. Therefore, it is
 * not part of any header file.
 */
void QAPTIVA_QDMI_set_device_status(QDMI_Device_Status status) {
  *QAPTIVA_QDMI_get_device_status() = status;
}

/**
 * @brief Static function to check if the driver is written in Python.
 * @return A pointer to the value. 1 if written in Python. Otherwise, 0
 * @note This function should not be used outside of this file. Therefore, it is
 * not part of any header file.
 */
static int *isFromPython(void) {
  static int isFromPython = 0;
  return &isFromPython;
}

/**
 *
 * @brief Static function to get the `remote qpu`.
 *
 * @details The `remote qpu` is created while initialting the session and
 * re-used while submitting jobs.
 * @return The `remote qpu` pointer if it is created. Otherwise, NULL;
 * @see submit_job
 * @see create_remote_qpu
 */

static PyObject **get_remote_qpu(void) {
  static PyObject *remote_qpu = NULL;
  return &remote_qpu;
}

/**
 * @brief Static function to get the noisy `remote qpu`.
 *
 * @details The noisy `remote qpu` is created while initialting the noisy
 * session and re-used while submitting noisy jobs via HTTP.
 * @return The noisy `remote qpu` pointer if it is created. Otherwise, NULL;
 * @see submit_job_http
 * @see create_noisy_remote_qpu
 */

static PyObject **get_noisy_remote_qpu(void) {
  static PyObject *noisy_remote_qpu = NULL;
  return &noisy_remote_qpu;
}

/**
 * @brief Static function to get the auxiliary Python Module
 *
 * @details The auxiliary Python Module is frequintly used thoughtout the
 * backend after it is imported while initialting the session.
 * @return The `custom python module` pointer if it is created. Otherwise, NULL;
 * @see create_remote_qpu
 * @see submit_job
 * @see initialize_python
 */

static PyObject **get_custom_python_module(void) {
  static PyObject *custom_python_module = NULL;
  return &custom_python_module;
}

/**
 * @brief Queries a operation property.
 * @details Since it is a emulator, there is no native gate set.
 * @param[in] session The session used for the query.
 * @param[in] operation The operation to query.
 * @param[in] num_sites The number of sites that the operation is applied to.
 * @param[in] sites A pointer to a list of handles where the sites that the
 * operation is applied to are stored.
 * @param[in] num_params The number of parameters that the operation takes.
 * @param[in] params A pointer to a list of parameters the operation takes.
 * @param[in] prop The property to query.
 * @param[in] size The size of the memory pointed to by @p value in bytes.
 * @param[out] value A pointer to the memory location where the value of the
 * property will be stored.
 * @param[out] size_ret The actual size of the data being queried in bytes.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>.
 */
int QAPTIVA_QDMI_device_session_query_operation_property(
    QAPTIVA_QDMI_Device_Session session, QAPTIVA_QDMI_Operation operation,
    const size_t num_sites, const QAPTIVA_QDMI_Site *sites,
    const size_t num_params, const double *params,
    const QDMI_Operation_Property prop, const size_t size, void *value,
    size_t *size_ret) {
  return QDMI_ERROR_NOTSUPPORTED;
}

/**
 * @brief Queries the device's properties.
 * @param[in] session The session used for the query. Must not be @c NULL.
 * @param[in] prop The property to query. Must be one of the values specified
 * for <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#ad251d8ae8fbbe9a5c7a10d66b243d526">QDMI_Device_Property</a>.
 * @param[in] size The size of the memory is pointed to by @p value in bytes.
 * Must be greater or equal to the size of the return type specified for @p
 * prop, except when @p value is @c NULL, in which case it is ignored.
 * @param[out] value A pointer to the memory location where the value of the
 * property will be stored. If this is @c NULL, it is ignored.
 * @param[out] size_ret The actual size of the data being queried in bytes. If
 * this is @c NULL, it is ignored.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the device supports the specified property and, when the @p value is not
 * @c NULL, the property was successfully retrieved.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 * if the device does not support the property.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if
 *  - the @p session is @c NULL,
 *  - the @p prop is invalid, or
 *  - the @p value is not @c NULL and the @p size is less than the size of the
 * data being queried.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a916e0810bf915e2ad67f2c1430c54fec">QDMI_ERROR_BADSTATE</a>
 * if the property cannot be queried in the current state of the session, for
 * example, because the session is not initialized.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if an unexpected error occurred.
 *
 * @see ADD_STRING_PROPERTY
 * @see ADD_SINGLE_VALUE_PROPERTY
 * @see ADD_LIST_PROPERTY
 */
int QAPTIVA_QDMI_device_session_query_device_property(
    QAPTIVA_QDMI_Device_Session session, const QDMI_Device_Property prop,
    const size_t size, void *value, size_t *size_ret) {
  if (prop >= QDMI_DEVICE_PROPERTY_MAX || (value == NULL && size_ret == NULL)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_NAME, "QAPTIVA", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_VERSION, "0.0.1", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "0.0.1", prop, size,
                      value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_QUBITSNUM, size_t, 38, prop,
                            size, value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_STATUS, QDMI_Device_Status,
                            QAPTIVA_QDMI_read_device_status(), prop, size,
                            value, size_ret)
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_SITES, QAPTIVA_QDMI_Site, DEVICE_SITES,
                    38, prop, size, value, size_ret)
  // Since it is a emulator, there is no coupling map.
  return QDMI_ERROR_NOTSUPPORTED;
}

/**
 * @brief Query a site property.
 * @details The Qaptiva device's `QDMI_Site`s only has indexes. Therefore, only
 * their indexes can be queried.
 * @param[in] session The session used for the query. Must not be @c NULL.
 * @param[in] site The site to query. Must not be @c NULL.
 * @param[in] prop The property to query. Must be one of the values specified
 * for <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a69ef10d452cc6f03cac8a917ba48d6e2">QDMI_Site_Property</a>.
 * @param[in] size The size of the memory pointed to by @p value in bytes. Must
 * be greater or equal to the size of the return type specified for @p prop,
 * except when @p value is @c NULL, in which case it is ignored.
 * @param[out] value A pointer to the memory location where the value of the
 * property will be stored. If this is @c NULL, it is ignored.
 * @param[out] size_ret The actual size of the data being queried in bytes. If
 * this is @c NULL, it is ignored.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the device supports the specified property and,
 * when @p value is not @c NULL, the property was successfully retrieved.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 + if the device does not support the
 * property.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if
 *  - @p session or @p site is @c NULL,
 *  - @p prop is invalid, or
 *  - @p value is not @c NULL and @p size is less than the size of the data
 *  being queried.
 *
 */
int QAPTIVA_QDMI_device_session_query_site_property(
    QAPTIVA_QDMI_Device_Session session, QAPTIVA_QDMI_Site site,
    const QDMI_Site_Property prop, const size_t size, void *value,
    size_t *size_ret) {

  // Since it is a emulator, there is no qubit property.
  if (session == NULL || site == NULL || (value != NULL && size == 0) ||
      prop >= QDMI_SITE_PROPERTY_MAX) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (prop != QDMI_SITE_PROPERTY_INDEX)
    return QDMI_ERROR_NOTSUPPORTED;

  ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_INDEX, uint64_t, site->id, prop,
                            size, value, size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

/* QUERY INTERFACE ENDS*/

/* CONTROL INTERFACE STARTS*/

/**
 * @brief Create a job.
 * @details This is the main entry point for a driver to create a job for a
 * device. After the required memory is allocated, the job is initiaited
 * @param[in] session The session to create the job on.
 * @param[out] job A pointer to a handle that will store the created job. The
 * job must be freed by calling @ref QAPTIVA_QDMI_device_job_free when it is no
 * longer used.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the job was successfully created.
 * @return @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if @p session or @p job are @c NULL.
 *
 * @see QAPTIVA_QDMI_device_job_submit
 * @see QAPTIVA_QDMI_device_job_set_parameter
 * @see QAPTIVA_QDMI_device_job_cancel
 * @see QAPTIVA_QDMI_device_job_check
 * @see QAPTIVA_QDMI_device_job_wait
 * @see QAPTIVA_QDMI_device_job_get_results
 * @see QAPTIVA_QDMI_device_job_free
 */

int QAPTIVA_QDMI_device_session_create_device_job(
    QAPTIVA_QDMI_Device_Session session, QAPTIVA_QDMI_Device_Job *job) {

  if (session == NULL || job == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  *job =
      (QAPTIVA_QDMI_Device_Job)malloc(sizeof(QAPTIVA_QDMI_Device_Job_impl_t));
  (*job)->session = session;

  (*job)->id = rand();
  (*job)->status = QDMI_JOB_STATUS_CREATED;
  (*job)->num_shots = NULL;
  (*job)->program = NULL;

  (*job)->probability_dense = NULL;
  (*job)->probability_keys = NULL;
  (*job)->probability_values = NULL;
  (*job)->results_size = NULL;
  (*job)->n_state = NULL;

  (*job)->t1 = NULL;
  (*job)->t2 = NULL;

  return QDMI_SUCCESS;
}

/**
 * @brief Set a parameter for a job.
 * @param[in] job A handle to a job for which to set @p param. Must not be @c
 * NULL.
 * @param[in] param The parameter whose value will be set. Must be one of the
 * values specified for <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a40dd25c531ebf99fb4b46469083b609e">QDMI_Device_Job_Parameter</a>.
 * @param[in] size The size of the data pointed to by @p value in bytes. Must
 * not be zero, except when @p value is @c NULL, in which case it is ignored.
 * @param[in] value A pointer to the memory location that contains the value of
 * the parameter to be set. The data pointed to by @p value is copied and can be
 * safely reused after this function returns. If this is @c NULL, it is ignored.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the device supports the specified <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a40dd25c531ebf99fb4b46469083b609e">QDMI_Device_Job_Parameter</a> @p param and, when @p value is not @c NULL, the
 * parameter was successfully set.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 * if the device does not support the
 * parameter or the value of the parameter.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if
 *  - @p job is @c NULL,
 *  - @p param is invalid, or
 *  - @p value is not @c NULL and @p size is zero or not the expected size for
 *    the parameter (if specified by the <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a40dd25c531ebf99fb4b46469083b609ec">QDMI_Device_Job_Parameter</a>
 * documentation).
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a916e0810bf915e2ad67f2c1430c54fec">QDMI_ERROR_BADSTATE</a> if the parameter cannot be set in the
 * current state of the job, for example, because the job is already submitted.
 *
 * @see QAPTIVA_QDMI_device_job_submit
 * @see QAPTIVA_QDMI_device_job_set_parameter
 * @see QAPTIVA_QDMI_device_job_cancel
 * @see QAPTIVA_QDMI_device_job_check
 * @see QAPTIVA_QDMI_device_job_wait
 * @see QAPTIVA_QDMI_device_job_get_results
 * @see QAPTIVA_QDMI_device_job_free
 */
int QAPTIVA_QDMI_device_job_set_parameter(QAPTIVA_QDMI_Device_Job job,
                                          const QDMI_Device_Job_Parameter param,
                                          const size_t size,
                                          const void *value) {
  if (job == NULL || (value != NULL && size == 0) || (value == NULL) ||
      (param >= QDMI_DEVICE_JOB_PARAMETER_MAX &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM2 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM3 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM4 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (job->status != QDMI_JOB_STATUS_CREATED) {
    return QDMI_ERROR_BADSTATE;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT) {
    QDMI_Program_Format format = *(const QDMI_Program_Format *)value;
    if (format != QDMI_PROGRAM_FORMAT_QASM2)
      return QDMI_ERROR_NOTSUPPORTED;
    job->format = &format;
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAM) {
    job->program = malloc(size);
    strncpy(job->program, (const char *)value, size - 1);
    job->program[size - 1] = '\0';
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM) {
    size_t shots = *(const size_t *)value;
    job->num_shots = malloc(sizeof(size_t));
    memcpy(job->num_shots, (size_t *)value, sizeof(size_t));
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_CUSTOM1) {

    if (size != sizeof(double))
      return QDMI_ERROR_INVALIDARGUMENT;

    size_t t1 = *(const size_t *)value;
    job->t1 = malloc(sizeof(double));
    memcpy(job->t1, (size_t *)value, sizeof(double));
    return QDMI_SUCCESS;
  }

  if (param == QDMI_DEVICE_JOB_PARAMETER_CUSTOM2) {
    if (size != sizeof(double))
      return QDMI_ERROR_INVALIDARGUMENT;
    size_t t2 = *(const size_t *)value;
    job->t2 = malloc(sizeof(double));
    memcpy(job->t2, (size_t *)value, sizeof(double));
    return QDMI_SUCCESS;
  }

  return QDMI_ERROR_NOTSUPPORTED;
}
/**
 * @brief An auxiliary function for submitting job
 *
 * @param[in] job A handle to a job to be submitted
 * @details To submit a job this device, we need to use Qaptiva's Python Package
 * named myQLM. Therefore, this function calls the `submit_job` from the
 * `qlm_auxiliary` to submit the given job.
 *
 *
 * @see QAPTIVA_QDMI_device_job_submit
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the job submitted successfully
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if an error occur
 */

int submit_job(QAPTIVA_QDMI_Device_Job job) {
  PyGILState_STATE gstate;

  if (!*isFromPython())
    gstate = PyGILState_Ensure();

  PyObject *custom_python_module = *get_custom_python_module();

  PyObject *pFunc =
      PyObject_GetAttrString(custom_python_module, SUBMIT_JOB_FUNCTION_NAME);
  CHECK_PYTHON_ERROR(pFunc, *isFromPython())

  PyObject **remote_qpu = get_remote_qpu();
  CHECK_PYTHON_ERROR(*remote_qpu, *isFromPython())

  PyObject *qasm_string = PyUnicode_FromString(job->program);
  CHECK_PYTHON_ERROR(qasm_string, *isFromPython())

  PyObject *num_shot = PyLong_FromUnsignedLong(*(job->num_shots));
  CHECK_PYTHON_ERROR(num_shot, *isFromPython())

  PyObject *pArgs = PyTuple_Pack(3, *remote_qpu, qasm_string, num_shot);
  CHECK_PYTHON_ERROR(pArgs, *isFromPython())

  job->status = QDMI_JOB_STATUS_RUNNING;
  QAPTIVA_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);

  PyObject *pResults = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pResults, *isFromPython())

  unsigned long resultSize = (unsigned long)PyList_GET_SIZE(pResults);
  PyObject *pBitring = PyList_GET_ITEM(pResults, 0);

  asprintf(&(job->probability_keys), "%s", PyUnicode_AsUTF8(pBitring));
  job->probability_values = malloc(sizeof(double) * resultSize);
  PyObject *pProbability;
  for (size_t i = 1; i < resultSize; i++) {
    pProbability = PyList_GetItem(pResults, (long)i);
    job->probability_values[i - 1] = PyFloat_AS_DOUBLE(pProbability);
  }
  job->results_size = malloc(sizeof(size_t));
  memcpy(job->results_size, &resultSize, sizeof(size_t));

  job->status = QDMI_JOB_STATUS_DONE;
  QAPTIVA_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);

  if (!*isFromPython())
    PyGILState_Release(gstate);

  return QDMI_SUCCESS;
}

int submit_noisy_job(QAPTIVA_QDMI_Device_Job job) {

  PyGILState_STATE gstate;
  if (!*isFromPython())
    gstate = PyGILState_Ensure();

  PyObject *custom_python_module = *get_custom_python_module();
  PyObject *pFunc = PyObject_GetAttrString(custom_python_module,
                                           SUBMIT_NOISY_JOB_FUNCTION_NAME);
  CHECK_PYTHON_ERROR(pFunc, *isFromPython());

  PyObject *pHost = PyUnicode_FromString(job->session->url);
  CHECK_PYTHON_ERROR(pHost, *isFromPython());

  PyObject *qasm_string = PyUnicode_FromString(job->program);
  CHECK_PYTHON_ERROR(qasm_string, *isFromPython());

  PyObject *num_shots = PyLong_FromUnsignedLong(*(job->num_shots));
  CHECK_PYTHON_ERROR(num_shots, *isFromPython());

  // Add t1 and t2 parameters
  PyObject *py_t1 = PyFloat_FromDouble(*(job->t1));
  PyObject *py_t2 = PyFloat_FromDouble(*(job->t2));

  PyObject *pArgs =
      PyTuple_Pack(5, pHost, qasm_string, num_shots, py_t1, py_t2);
  CHECK_PYTHON_ERROR(pArgs, *isFromPython());

  job->status = QDMI_JOB_STATUS_RUNNING;
  QAPTIVA_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);

  PyObject *pResults = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pResults, *isFromPython());

  // Parse results
  Py_ssize_t resultSize = PyList_GET_SIZE(pResults);
  if (resultSize < 1) {
    job->status = QDMI_JOB_STATUS_FAILED;
    QAPTIVA_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
    return QDMI_ERROR_FATAL;
  }

  PyObject *pBitstring = PyList_GET_ITEM(pResults, 0);
  asprintf(&(job->probability_keys), "%s", PyUnicode_AsUTF8(pBitstring));

  job->probability_values = malloc(sizeof(double) * (resultSize - 1));
  for (Py_ssize_t i = 1; i < resultSize; ++i) {
    PyObject *pValue = PyList_GET_ITEM(pResults, i);
    job->probability_values[i - 1] = PyFloat_AsDouble(pValue);
  }

  memcpy(job->results_size, &resultSize, sizeof(size_t));
  job->status = QDMI_JOB_STATUS_DONE;

  QAPTIVA_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);

  if (!*isFromPython())
    PyGILState_Release(gstate);

  return QDMI_SUCCESS;
}

/**
 * @brief Submit a job to the device.
 * @details To submit a job this device, we need to use Qaptiva's Python Package
 * named myQLM. The out-of-the box job submission function is a
 * blocking-function that waits until job is completed.
 * @param[in] job The job to submit. Must not be @c NULL.
 * @return @ref QDMI_SUCCESS if the job was successfully submitted.
 * @return @ref QDMI_ERROR_INVALIDARGUMENT if @p job is @c NULL.
 * @return @ref QDMI_ERROR_FATAL if the job submission failed.
 *
 * @see submit_job
 * @see QAPTIVA_QDMI_device_session_create_device_job
 * @see QAPTIVA_QDMI_device_job_set_parameter
 * @see QAPTIVA_QDMI_device_job_cancel
 * @see QAPTIVA_QDMI_device_job_check
 * @see QAPTIVA_QDMI_device_job_wait
 * @see QAPTIVA_QDMI_device_job_get_results
 * @see QAPTIVA_QDMI_device_job_free
 */
int QAPTIVA_QDMI_device_job_submit(QAPTIVA_QDMI_Device_Job job) {

  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status != QDMI_JOB_STATUS_CREATED || job->program == NULL ||
      job->num_shots == NULL) {

    return QDMI_ERROR_INVALIDARGUMENT;
  }

  int err = 0;
  job->status = QDMI_JOB_STATUS_SUBMITTED;
  if (job->session->is_noisy_session) {
    if (job->t1 == NULL || job->t2 == NULL)
      return QDMI_ERROR_INVALIDARGUMENT;
    err = submit_noisy_job(job);
  } else {
    err = submit_job(job);
  }

  if (err) {
    job->status = QDMI_JOB_STATUS_FAILED;
  }
  return QDMI_SUCCESS;
}

/**
 * @brief Cancel an already created job.
 * @details The submitted jobs cannot be canceled due to nature of the device.
 * Therefore, this function is not supported.
 * @param[in] job The job to cancel. Must not be @c NULL.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the job was successfully canceled.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if @p job is @c NULL or the job already has the status QDMI_JOB_STATUS_DONE.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if the job could not be canceled.
 * @see QAPTIVA_QDMI_device_session_create_device_job
 * @see QAPTIVA_QDMI_device_job_submit
 * @see QAPTIVA_QDMI_device_job_set_parameter
 * @see QAPTIVA_QDMI_device_job_check
 * @see QAPTIVA_QDMI_device_job_wait
 * @see QAPTIVA_QDMI_device_job_get_results
 * @see QAPTIVA_QDMI_device_job_free
 */
int QAPTIVA_QDMI_device_job_cancel(QAPTIVA_QDMI_Device_Job job) {
  return QDMI_ERROR_NOTSUPPORTED;
}

/**
 * @brief Check the status of a job.
 * @details This function is non-blocking and returns immediately with the job
 * status.
 * @param[in] job The job to check the status of. Must not be @c NULL.
 * @param[out] status The status of the job. Must not be @c NULL.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the job status was successfully checked.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if @p job or @p status is @c NULL.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if the job status could not be checked.
 */
int QAPTIVA_QDMI_device_job_check(QAPTIVA_QDMI_Device_Job job,
                                  QDMI_Job_Status *status) {

  if (job == NULL || status == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (job->status < QDMI_JOB_STATUS_CREATED ||
      job->status > QDMI_JOB_STATUS_CANCELED)
    return QDMI_ERROR_INVALIDARGUMENT;

  *status = job->status;

  return QDMI_SUCCESS;
}

/**
 * @brief Wait for a job to finish.
 * @details Since @ref QAPTIVA_QDMI_device_job_submit is a blocking function,
 * this function makes sure that status is set to QDMI_JOB_STATUS_DONE.
 * @param[in] job The job to wait for. Must not be @c NULL.
 * @param[in] timeout The timeout in seconds.
 * If this is zero, the function waits indefinitely until the job has finished.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the job is finished or canceled.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if @p job is @c NULL.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if the job could not be waited for and this function returns before the job
 * has finished or has been canceled.
 */
int QAPTIVA_QDMI_device_job_wait(QAPTIVA_QDMI_Device_Job job, size_t timeout) {
  return QDMI_ERROR_NOTSUPPORTED;
}

/**
 * @brief Retrieve the results of a job.
 * @details The QLM device returns the results as a state-probability pair.
 * Therefore, this function provides the probability values, keys, and density,
 * as well as the histogram keys and values, which are calculated by multiplying
 * the probability values by the number of shots.
 * @param[in] job The job to retrieve the results from. Must not be @c NULL.
 * @param[in] result The result to retrieve. Must be one of the values specifie
 * for <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#aa154b942b0f67e437c393ad7b33cadd1">QDMI_Job_Result</a>
 * @param[in] size The size of the buffer pointed to by @p data in bytes. Must
 * be greater or equal to the size of the return type specified for the <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#aa154b942b0f67e437c393ad7b33cadd1">QDMI_Job_Result</a>
 * @p result, except when @p data is @c NULL, in which case it is ignored.
 * @param[out] data A pointer to the memory location where the results will be
 * stored. If this is @c NULL, it is ignored.
 * @param[out] size_ret The actual size of the data being queried in bytes. If
 * this is @c NULL, it is ignored.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the device supports the specified result and, when @p data is not @c NULL,
 * the results were successfully retrieved.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if
 *  - @p job is @c NULL,
 *  - @p job has not completed,
 *  - @p result is invalid, or
 *  - @p data is not @c NULL and @p size is smaller than the size of the data
 *    being queried.
 */
int QAPTIVA_QDMI_device_job_get_results(QAPTIVA_QDMI_Device_Job job,
                                        QDMI_Job_Result result, size_t size,
                                        void *data, size_t *size_ret) {

  if (job == NULL || job->status != QDMI_JOB_STATUS_DONE)
    return QDMI_ERROR_INVALIDARGUMENT;

  if (result == QDMI_JOB_RESULT_STATEVECTOR_DENSE ||
      result == QDMI_JOB_RESULT_STATEVECTOR_SPARSE_KEYS ||
      result == QDMI_JOB_RESULT_STATEVECTOR_SPARSE_VALUES)
    return QDMI_ERROR_NOTSUPPORTED;

  size_t required_size;
  if (result == QDMI_JOB_RESULT_HIST_KEYS ||
      result == QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS) {
    required_size = strlen(job->probability_keys);
    if (data) {
      if (size < required_size)
        return QDMI_ERROR_INVALIDARGUMENT;
      strncpy(data, job->probability_keys, size - 1);
      ((char *)data)[size - 1] = '\0';
      return QDMI_SUCCESS;
    }
    if (size_ret)
      *size_ret = required_size;
    return QDMI_SUCCESS;
  }

  if (result == QDMI_JOB_RESULT_HIST_VALUES)
    GET_VALUE_DATA(double, *(job->num_shots))

  if (result == QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES)
    GET_VALUE_DATA(double, 1)

  if (result == QDMI_JOB_RESULT_PROBABILITIES_DENSE) {
    if (job->probability_dense == NULL) {
      char *bitstream, *key, *endptr;

      size_t n_qubit = 0;
      int stream_index = 0;

      asprintf(&bitstream, "%s", job->probability_keys);
      while (bitstream[++n_qubit] != ',')
        ;
      job->n_state = malloc(sizeof(size_t));
      *(job->n_state) = 1 << n_qubit;
      required_size = *(job->n_state) * sizeof(double);

      job->probability_dense = malloc(required_size);
      memset(job->probability_dense, 0, sizeof(double) * *(job->n_state));

      key = strtok(bitstream, ",");
      while (key != NULL) {
        long index = strtol(key, &endptr, 2);
        key = strtok(NULL, ",");
        job->probability_dense[index] = job->probability_values[stream_index++];
      }
    }
    required_size = *(job->n_state) * sizeof(double);
    if (size_ret)
      *size_ret = required_size;
    if (data) {
      if (size < required_size)
        return QDMI_ERROR_INVALIDARGUMENT;
      double *data_ptr = data;
      memset(data_ptr, 0, sizeof(double) * *(job->n_state));
      for (int index = 0; index < required_size / sizeof(double); index++) {
        *data_ptr++ = job->probability_dense[index];
      }
    }
  }

  return QDMI_SUCCESS;
}
/**
 * @brief Free a job.
 * @details Free the resources associated with a job. Using a job handle after
 * it was freed is undefined behavior.
 * @param[in] job The job to free.
 */
void QAPTIVA_QDMI_device_job_free(QAPTIVA_QDMI_Device_Job job) {
  free(job->probability_dense);
  job->probability_dense = NULL;
  free(job->probability_keys);
  job->probability_keys = NULL;
  free(job->probability_values);
  job->probability_values = NULL;

  free(job);
  job = NULL;
}
/**
 * @brief Query a job property.
 * @param[in] job A handle to a job for which to query @p prop. Must not be @c
 * NULL.
 * @param[in] prop The property to query. Must be one of the values specified
 * for a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a6e4d18c7fa5d383bbcc1498abe090d4f">QDMI_Device_Job_Property</a>
 * @p result, except when @p data is @c NULL, in which case it is ignored..
 * @param[in] size The size of the memory pointed to by @p value in bytes. Must
 * be greater or equal to the size of the return type specified for @p prop,
 * except when @p value is @c NULL, in which case it is ignored.
 * @param[out] value A pointer to the memory location where the value of the
 * property will be stored. If this is @c NULL, it is ignored.
 * @param[out] size_ret The actual size of the data being queried in bytes. If
 * this is @c NULL, it is ignored.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the job supports the specified property and, when @p value is not @c NULL,
 * the property was successfully retrieved.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>.
 * if the job does not support the property.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if
 *  - @p job is @c NULL,
 *  - @p prop is invalid, or
 *  - @p value is not @c NULL and @p size is less than the size of the data
 *    being queried.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a916e0810bf915e2ad67f2c1430c54fec">QDMI_ERROR_BADSTATE</a>
 * if the property cannot be queried in the current state of the job, for
 * example, because the job failed or the property is not initialized because it
 * has no default value and was not set.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if an unexpected error occurred.
 */
int QAPTIVA_QDMI_device_job_query_property(QAPTIVA_QDMI_Device_Job job,
                                           QDMI_Device_Job_Property prop,
                                           size_t size, void *value,
                                           size_t *size_ret) {
  return QDMI_ERROR_NOTIMPLEMENTED;
}
/**
 * @brief Initializes the Python interpreter.
 *
 * @details Since we are obligated to use myQLM Python package, we need to
 * initialize the interpreter. However, if the library is used by a Python based
 * client, the interpreter does not need to be initialized.
 *
 * This function initializes the Python interpreter and imports the custom
 * auxiliary module which is named qlm_auxiliary.py by default. The module is
 * used in @ref create_remote_qpu and @ref submit_job.
 *
 * @returns <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if the python interpreter cannot be initialized .
 * @returns <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the python interpreter is successfully initialized.
 */
int initialize_python(void) {

  char *script_location = getenv(SCRIPT_LOCATION);
  char *script_name = getenv(SCRIPT_NAME);

  *isFromPython() = Py_IsInitialized();
  PyGILState_STATE gstate;
  if (!*isFromPython()) {
    Py_Initialize();
    PyThreadState *_save = PyEval_SaveThread();
    gstate = PyGILState_Ensure();
  }

  PyObject *sysPath = PyImport_ImportModule("sys");
  PyObject *path = PyObject_GetAttrString(sysPath, "path");

  PyList_Append(path, PyUnicode_FromString(script_location));

  PyObject *pName = PyUnicode_DecodeFSDefault(script_name);
  CHECK_PYTHON_ERROR(pName, *isFromPython())

  *get_custom_python_module() = PyImport_Import(pName);

  CHECK_PYTHON_ERROR(*get_custom_python_module(), *isFromPython());

  Py_XDECREF(pName);

  if (!*isFromPython())
    PyGILState_Release(gstate);

  return QDMI_SUCCESS;
}
/**
 * @brief Connects to the remote qpu.
 *
 * @details This function is connects to the remote qpu by using the
 * create_remote_qpu function from the qlm_auxiliary.
 *
 * @param[in] hostname The name of the host where the device is located. Must be
 * in the form "host:port".
 * @returns <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if the connection faills.
 * @returns <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the connection success.
 */
int create_remote_qpu(const char *hostname) {

  PyGILState_STATE gstate = PyGILState_Ensure();
  PyObject *pFunc = PyObject_GetAttrString(*get_custom_python_module(),
                                           REMOTE_QPU_CREATE_FUNCTION_NAME);

  CHECK_PYTHON_ERROR(pFunc, *isFromPython())

  PyObject *pArgs = PyTuple_Pack(1, PyUnicode_FromString(hostname));
  CHECK_PYTHON_ERROR(pArgs, *isFromPython())

  PyObject *pResult = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pResult, *isFromPython())

  *get_remote_qpu() = pResult;
  PyGILState_Release(gstate);
  return QDMI_SUCCESS;
}

int create_noisy_remote_connection(const char *hostname) {
  // For noisy sessions, we don't need to create a QLM QPU object
  // since we use HTTP requests directly. We just store the hostname
  // and validate the connection is available.

  // We could add a simple HTTP ping here to validate the server is up
  // For now, we just assume it's working since it's a local server

  return QDMI_SUCCESS;
}
/** @brief Checks if the required environment variables are set
 *
 * @details Two environment variables needs to be set to locate the custom
  * auxiliary module: @ref SCRIPT_LOCATION indicates the location of the file
 and


 * @ref SCRIPT_NAME indicates the name of the file.
 * @returns returns <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if either one is NULL.
 * @returns returns <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if both is set.
 */
int check_env_variable(void) {
  if (getenv(SCRIPT_LOCATION) == NULL || getenv(SCRIPT_NAME) == NULL)
    return QDMI_ERROR_FATAL;
  return QDMI_SUCCESS;
}

/**
 *  @brief Initializes the device
 *
 *  @details This function check if the required environment variables are set
 * and the Python interpreter can be initialized.
 *
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if the environment variables are null or Python interpreter cannot be
 * initialized.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the device succuessly is initialized.
 */
int QAPTIVA_QDMI_device_initialize(void) {
  int err = check_env_variable();
  CHECK_QDMI_ERROR(err)

  err = initialize_python();
  CHECK_QDMI_ERROR(err)

  QAPTIVA_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  return QDMI_SUCCESS;
}

/**
 * @brief Finalizes the device
 *
 *  @details This function finalizes the device by freeing the custom module,
 * and finalizesthe Python interpreter.
 *
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the device succuessly is finalized.
 */
int QAPTIVA_QDMI_device_finalize(void) {
  QAPTIVA_QDMI_set_device_status(QDMI_DEVICE_STATUS_OFFLINE);
  if (*get_custom_python_module() != NULL) {
    Py_DECREF(*get_custom_python_module());
    *get_custom_python_module() = NULL;
  }

  // Clean up regular QPU
  if (*get_remote_qpu() != NULL) {
    Py_DECREF(*get_remote_qpu());
    *get_remote_qpu() = NULL;
  }

  // Clean up noisy QPU (if any Python objects were created)
  if (*get_noisy_remote_qpu() != NULL) {
    Py_DECREF(*get_noisy_remote_qpu());
    *get_noisy_remote_qpu() = NULL;
  }

  if (Py_IsInitialized() && !_Py_IsFinalizing() && !*isFromPython()) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    Py_Finalize();
  }
  return QDMI_SUCCESS;
}
/**
 * @brief Allocates a new device session.
 * @param[out] session A handle to the session that is allocated. Must not be
 * @c NULL.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the session was allocated successfully.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if @p session is @c NULL.
 * @see QDMI_device_session_set_parameter
 * @see QDMI_device_session_init
 */
int QAPTIVA_QDMI_device_session_alloc(QAPTIVA_QDMI_Device_Session *session) {
  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  *session = (QAPTIVA_QDMI_Device_Session)malloc(
      sizeof(QAPTIVA_QDMI_Device_Session_impl_t));
  (*session)->url = NULL;
  (*session)->is_noisy_session = NULL; // Default to regular session
  (*session)->status = ALLOCATED;
  return QDMI_SUCCESS;
}
/**
 * @brief Initializes a device session.
 * @details This function initializes the device session and prepares it for
 * use based on the set parameters.
 * @param[out] session A handle to the session that is allocated.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the session was allocated successfully.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if @p session is @c NULL.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if the device's status is QDMI_DEVICE_STATUS_ERROR or
 * QDMI_DEVICE_STATUS_OFFLINE or QDMI_DEVICE_STATUS_MAINTENANCE
 * @see QAPTIVA_QDMI_device_session_alloc
 * @see QDMI_device_session_set_parameter
 */
int QAPTIVA_QDMI_device_session_init(QAPTIVA_QDMI_Device_Session session) {
  if (session == NULL) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  switch (QAPTIVA_QDMI_read_device_status()) {
  case QDMI_DEVICE_STATUS_ERROR:
  case QDMI_DEVICE_STATUS_OFFLINE:
  case QDMI_DEVICE_STATUS_MAINTENANCE:
    return QDMI_ERROR_FATAL;
  default:
    break;
  }

  if (session->url == NULL) {
    return QDMI_ERROR_BADSTATE;
  }

  int err;
  if (session->is_noisy_session)
    err = create_noisy_remote_connection(session->url);
  else
    err = create_remote_qpu(session->url);

  CHECK_QDMI_ERROR(err)

  session->status = INITIALIZED;

  return QDMI_SUCCESS;
}
/**
 * @brief Free a QDMI device session.
 * @param[in] session The session to free.
 */
void QAPTIVA_QDMI_device_session_free(QAPTIVA_QDMI_Device_Session session) {
  if (session != NULL) {
    free(session->url);
    free(session);
  }
}
/**
 * @brief Set a parameter for a device session.
 * @param[in] session A handle to the session to set the parameter for.
 * @param[in] param The parameter to set. Must be one of the values specified
 * for <a href="
 *  https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a9f1e467b2b3870263b0e9d7e5d36cea4">QDMI_Device_Session_Parameter</a>.
 * @param[in] size The size of the data pointed by @p value in bytes.
 * @param[in] value A pointer to the memory location that contains the value of
 * the parameter to be set.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the device supports the specified <a href="
 *  https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a9f1e467b2b3870263b0e9d7e5d36cea4">QDMI_Device_Session_Parameter</a>
 * and, when @p value is not @c NULL, the value of the parameter was set
 * successfully.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>.
 * if the device does not support the parameter or the value of the parameter.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if
 *  - @p session is @c NULL,
 *  - @p param is invalid, or
 *  - @p value is not @c NULL and @p size is zero or not the expected size for
 *    the parameter (if specified by the <a href="
 *  https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a9f1e467b2b3870263b0e9d7e5d36cea4">QDMI_Device_Session_Parameter</a>
 * documentation).
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a916e0810bf915e2ad67f2c1430c54fec">QDMI_ERROR_BADSTATE</a>
 * if the parameter cannot be set in the current state of the session.
 * @see QAPTIVA_QDMI_device_session_alloc
 * @see QDMI_device_session_init
 */
int QAPTIVA_QDMI_device_session_set_parameter(
    QAPTIVA_QDMI_Device_Session session,
    const QDMI_Device_Session_Parameter param, const size_t size,
    const void *value) {
  if (session == NULL || (value != NULL && size == 0) ||
      (param >= QDMI_DEVICE_SESSION_PARAMETER_MAX &&
       param != QDMI_DEVICE_SESSION_PARAMETER_TOKEN &&
       param != QDMI_DEVICE_SESSION_PARAMETER_BASEURL &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM3 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM4 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM5)) {

    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (session->status != ALLOCATED) {
    return QDMI_ERROR_BADSTATE;
  }

  if (param == QDMI_DEVICE_SESSION_PARAMETER_BASEURL) {
    session->url = (char *)malloc(size);
    strncpy(session->url, (const char *)value, size - 1);
    session->url[size - 1] = '\0';
  }

  if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1) {
    session->is_noisy_session = malloc(sizeof(int));
    memcpy(session->is_noisy_session, (int *)value, size);
  }

  return QDMI_SUCCESS;
}
