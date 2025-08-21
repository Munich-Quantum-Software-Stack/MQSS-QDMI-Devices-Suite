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

#include "sensors.h"
#include "session.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <future>
#include <vector>

#include "qdmi/constants.h"

/** @file
 * @brief The QDMI Device implementation of the DCDB at Leibniz Supercomputing
 * Centre.
 * @details The Data Center Data Base (DCDB) is a modular, continuous, and
 * holistic monitoring framework targeted at HPC telemetries. This device
 * implementation allows a `QDMI Client` to query the telemetry data from LRZ's
 * DCDB Instance.
 */

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
 * href="https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/ebff5b1aa4638fe7c1165f9f75faa48efab33f13/examples/device/cxx/device.cpp#L235-L251">the
 * example C++ device</a> in the QDMI repository.
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
 * href="https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/ebff5b1aa4638fe7c1165f9f75faa48efab33f13/examples/device/cxx/device.cpp#L218-L234">the
 * example C++ device</a> in the QDMI repository.
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
 * href="https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/ebff5b1aa4638fe7c1165f9f75faa48efab33f13/examples/device/cxx/device.cpp#L252-L271">the
 * example C++ device</a> in the QDMI repository.
 *
 */
#define ADD_LIST_PROPERTY(prop_name, prop_type, prop_values, prop, size,       \
                          value, size_ret)                                     \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < (prop_values).size() * sizeof(prop_type)) {               \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        memcpy(static_cast<void *>(value),                                     \
               static_cast<const void *>((prop_values).data()),                \
               (prop_values).size() * sizeof(prop_type));                      \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = (prop_values).size() * sizeof(prop_type);                  \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

/**
 * @brief Device-side implementation of the encapsulated type
 * `QDMI_Device_TelemetrySensor_Query`.
 *
 * @details This struct holds all necessary inputs to perform a
 * `QDMI_TelemetrySensor_Query` using DCDB, as well as the query results.
 *
 * @note The `QDMI_Device_TelemetrySensor_Query` type is encapsulated within
 * the `QDMI Device Telemetry Sensor Query` interface. This design allows
 * devices to implement the query type in a device-specific manner. This struct
 * represents the opaque pointer type used on the device side, encapsulating all
 * information required for an telemetry sensor query.
 */
struct DCDB_QDMI_Device_TelemetrySensor_Query_impl_d {
  /**
   * @brief The start time of the `QDMI_TelemetrySensor_Query` to be queried.
   *
   * @details Both the `start_time and the `end_time` needs to be valid UNIX
   * timestamp
   */
  uint64_t start_time;

  /// @see DCDB_QDMI_Device_TelemetrySensor_Query_impl_d::start_time
  uint64_t end_time;

  /// The `QDMI_TelemetrySensor` being queried.
  DCDB_QDMI_TelemetrySensor telemetrysensor{};

  /// Status of the `QDMI_TelemetrySensor_Query`.
  QDMI_TelemetrySensor_Query_Status status{};

  /// Pointer to the session used for authentication and connection metadata.
  DCDB_QDMI_Device_Session_impl_d *session;

  /// Results of the `QDMI_TelemetrySensor_Query`.
  std::list<DCDB::SensorDataStoreReading> results;

  /// Number of elements in `results`.
  size_t result_length{};

  /**
   * @brief Handle for asynchronous execution of the query.
   *
   * This variable represents the asynchronous task. For more information, refer
   * to
   * @ref DCDB_QDMI_device_telemetrysensor_query_submit.
   */
  std::future<void> async_query;
};

/**
 * @brief Static function to maintain the device status.
 * @return A pointer to the device status.
 * @note This function should not be used outside of this file. Therefore, it is
 * not part of any header file.
 */
static QDMI_Device_Status *DCDB_QDMI_get_device_status(void) {
  static QDMI_Device_Status device_status = QDMI_DEVICE_STATUS_OFFLINE;
  return &device_status;
}

/**
 * @brief The local function to read the device status.
 * @return The current device status.
 * @note This function should not be used outside of this file. Therefore, it is
 * not part of any header file.
 */
QDMI_Device_Status DCDB_QDMI_read_device_status(void) {
  return *DCDB_QDMI_get_device_status();
}

/**
 * @brief The local function to set the device status.
 * @param status the new device status.
 * @note This function should not be used outside of this file. Therefore, it is
 * not part of any header file.
 */
void DCDB_QDMI_set_device_status(QDMI_Device_Status status) {
  *DCDB_QDMI_get_device_status() = status;
}

/**
 * @brief The function to initiate the DCDB Device.
 *
 * @details This function is called to turn on the device's functionalities. The
 * device can only be used after this function is called.
 *
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>.
 *
 */
int DCDB_QDMI_device_initialize() {
  DCDB_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  return QDMI_SUCCESS;
}

/**
 * @brief The function to finalize the DCDB Device.
 *
 * @details The `QDMI_Client` calls this function to turn off the device's
 * functionalities. After this function is called, the device cannot be used.
 *
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>.
 *
 */
int DCDB_QDMI_device_finalize() {
  DCDB_QDMI_set_device_status(QDMI_DEVICE_STATUS_OFFLINE);
  return QDMI_SUCCESS;
}

/**
 * @brief The function to allocate a session for the DCDB device.
 *
 * @details The function is the main entry point for a driver to establish a
 * session with the DCDB Device. The returned handle can be used to refer to the
 * `session` throughout the `device session interface`.
 * @param[out] session A handle to the session that is allocated. Must not be
 * @c NULL. The session must be freed by calling @ref
 DCDB_QDMI_device_session_free
 * when it is no longer used.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 if the session was allocated successfully.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 if the @p session is @c NULL.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a0cf40a28841e7b8bcba223ae28a3713d">QDMI_ERROR_OUTOFMEM</a>
 if memory space ran out.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 if an unexpected error occurred.
 * @see DCDB_QDMI_device_session_init
 * @see DCDB_QDMI_device_session_set_parameter
 * @see DCDB_QDMI_device_session_free
 */
int DCDB_QDMI_device_session_alloc(DCDB_QDMI_Device_Session *session) {
  if (session == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  *session = new DCDB_QDMI_Device_Session_impl_d();
  if (*session == nullptr)
    return QDMI_ERROR_OUTOFMEM;
  (*session)->setStatus(DCDB_QDMI_DEVICE_SESSION_STATUS::ALLOCATED);
  return QDMI_SUCCESS;
}

/**
 * @brief The function to init the session.
 *
 * @details This function establishes a connection to the DCDB host using
 * @ref DCDB_QDMI_Device_Session_impl_d::connect function.

 * @param[out] session A handle to the session that is initialted. Must not be
 * @c NULL. The session must be freed by calling @ref
 DCDB_QDMI_device_session_free
 * when it is no longer used.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the session was allocated successfully.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if the @p session is @c NULL.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if an unexpected error occurred.
 * @see DCDB_QDMI_device_session_alloc
 * @see DCDB_QDMI_device_session_set_parameter
 * @see DCDB_QDMI_device_session_free
 */
int DCDB_QDMI_device_session_init(DCDB_QDMI_Device_Session session) {
  if (session == nullptr)
    return QDMI_ERROR_INVALIDARGUMENT;
  return session->connect();
}

/**
 * @brief The function to free the session.
 *
 * @details This function frees the `session`. After the session is freed, it
 * cannot be used. While freing, the existing connection closes as well.

 * @param[in] session A handle to the session that is freed.
 * @see DCDB_QDMI_device_session_alloc
 * @see DCDB_QDMI_device_session_init
 * @see DCDB_QDMI_device_session_set_parameter
 */
void DCDB_QDMI_device_session_free(DCDB_QDMI_Device_Session session) {
  delete session;
}
/**
 * @brief Set a parameter for the device session.
 *
 * @details This function is used to set parameters for not-initiated, yet
 allocated sessions.
 * The set parameters are used to connecting to the DCDB host.
 * @param[in] session A handle to the session to set the parameter for. Must not
 * be @c NULL.
 * @param[in] param The parameter to set. Must be one of the values specified
 * for <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#ab99cb3929c8d79596e66fb276711ebda">QDMI_Device_Session_Parameter</a>
 * @param[in] size The size of the data pointed by @p value in bytes. Must not
 * be zero, except when @p value is @c NULL, in which case it is ignored.
 * @param[in] value A pointer to the memory location that contains the value of
 * the parameter to be set. The data pointed to by @p value is copied and can be
 * safely reused after this function returns. If this is @c NULL, it is ignored.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the device supports the specified <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#ab99cb3929c8d79596e66fb276711ebda">QDMI_Device_Session_Parameter</a> and, when @p value is not @c NULL, the value of
 * the parameter was set successfully.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 * if the device does not support the parameter or the value of the parameter.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if
 *  - @p session is @c NULL,
 *  - @p param is invalid, or
 *  - @p value is not @c NULL and @p size is zero or not the expected size for
 *    the parameter (if specified by the  <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#ab99cb3929c8d79596e66fb276711ebda">QDMI_Device_Session_Parameter</a>
 *    documentation).
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a916e0810bf915e2ad67f2c1430c54fec">QDMI_ERROR_BADSTATE</a>
 *  if the parameter cannot be set in the
 * current state of the session, for example, because the session is not
 * allocated or is initialized.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if an unexpected error occurred.
 * @see DCDB_QDMI_device_session_alloc
 * @see DCDB_QDMI_device_session_init
 * @see DCDB_QDMI_device_session_free
 */
int DCDB_QDMI_device_session_set_parameter(
    DCDB_QDMI_Device_Session session, const QDMI_Device_Session_Parameter param,
    const size_t size, const void *value) {

  if (session == NULL || (value != NULL && size == 0) ||
      (param >= QDMI_DEVICE_SESSION_PARAMETER_MAX &&
       param != QDMI_DEVICE_SESSION_PARAMETER_BASEURL &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (session->getStatus() != DCDB_QDMI_DEVICE_SESSION_STATUS::ALLOCATED) {
    return QDMI_ERROR_BADSTATE;
  }

  if (param == QDMI_DEVICE_SESSION_PARAMETER_BASEURL)
    session->setHostnameAndPort((char *)value);
  else if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1)
    session->setUsername((char *)value);
  else if (param == QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2)
    session->setPassword((char *)value);

  return QDMI_SUCCESS;
}

/**
 * @brief Creates a job.
 *
 * @details Despite the presence of the functions from the <a
 href="https://munich-quantum-software-stack.github.io/QDMI/group__device__job__interface.html">QDMI
 * Device Job Interface</a>, they are not operational since the DCDB device does
 not support `QDMI_Job`.
 *
 * @param[in] session The session to create the job on.
 * @param[out] job A pointer to a handle that will store the created job.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 */
int DCDB_QDMI_device_session_create_device_job(DCDB_QDMI_Device_Session session,
                                               DCDB_QDMI_Device_Job *job) {
  return QDMI_ERROR_NOTSUPPORTED;
}
/**
 * @brief Frees the job.
 *
 * @param[in] job The job to free.
 * @details @see DCDB_QDMI_device_session_create_device_job
 */
void DCDB_QDMI_device_job_free(DCDB_QDMI_Device_Job job) {}

/**
 * @brief Sets parameters for the job
 * @details @see DCDB_QDMI_device_session_create_device_job
 * @param[in] job A handle to a job for which to set @p param.
 * @param[in] param The parameter whose value will be set.
 * @param[in] size The size of the data pointed to by @p value in bytes.
 * @param[in] value A pointer to the memory location that contains the value
 * of the parameter to be set.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 */
int DCDB_QDMI_device_job_set_parameter(DCDB_QDMI_Device_Job job,
                                       const QDMI_Device_Job_Parameter param,
                                       const size_t size, const void *value) {
  return QDMI_ERROR_NOTSUPPORTED;
}
/**
 * @brief Submits the job to the device.
 * @param[in] job The job to submit.
 * @details @see DCDB_QDMI_device_session_create_device_job
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 */
int DCDB_QDMI_device_job_submit(DCDB_QDMI_Device_Job job) {
  return QDMI_ERROR_NOTSUPPORTED;
}
/**
 * @brief Cancels a submitted job to the device.
 *
 * @param[in] job The job to cancel.
 * @details @see DCDB_QDMI_device_session_create_device_job
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 */
int DCDB_QDMI_device_job_cancel(DCDB_QDMI_Device_Job job) {
  return QDMI_ERROR_NOTSUPPORTED;
}
/**
 * @brief Checks the status of the job.
 * @param[in] job The job to check the status of.
 * @param[out] status The status of the job.
 * @details @see DCDB_QDMI_device_session_create_device_job
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 */
int DCDB_QDMI_device_job_check(DCDB_QDMI_Device_Job job,
                               QDMI_Job_Status *status) {
  return QDMI_ERROR_NOTSUPPORTED;
}
/**
 * @brief Waits the job to be completed.
 *
 * @param[in] job The job to wait for.
 * @param[in] timeout The timeout in seconds.
 * @details @see DCDB_QDMI_device_session_create_device_job
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 */
int DCDB_QDMI_device_job_wait(DCDB_QDMI_Device_Job job, size_t timeout) {
  return QDMI_ERROR_NOTSUPPORTED;
}

/**
 * @brief Gets the results of the job.
 *
 * @details @see DCDB_QDMI_device_session_create_device_job
 * @param[in] job The job to retrieve the results from.
 * @param[in] result The result to retrieve.
 * @param[in] size The size of the buffer pointed to by @p data in bytes.
 * @param[out] data A pointer to the memory location where the results will be
 * stored.
 * @param[out] size_ret The actual size of the data being queried in bytes.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 */
int DCDB_QDMI_device_job_get_results(DCDB_QDMI_Device_Job job,
                                     QDMI_Job_Result result, const size_t size,
                                     void *data, size_t *size_ret) {
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

int DCDB_QDMI_device_session_query_device_property(
    DCDB_QDMI_Device_Session session, const QDMI_Device_Property prop,
    const size_t size, void *value, size_t *size_ret) {

  if (prop >= QDMI_DEVICE_PROPERTY_MAX || (value == NULL && size_ret == NULL)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_NAME, "DCDB", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_VERSION, "0.0.1", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "0.0.1", prop, size,
                      value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_STATUS, QDMI_Device_Status,
                            DCDB_QDMI_read_device_status(), prop, size, value,
                            size_ret)
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS,
                    DCDB_QDMI_TelemetrySensor, DCDB_DEVICE_TELEMETRYSENSORS,
                    prop, size, value, size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}
/**
 * @brief Queries a site property.
 *
 * @details The DCDB device does not have any <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/types_8h.html#ab26279159380e378f258cb663968b9ec">QDMI_Site</a>
 * to query.
 * @param[in] session The session used for the query.
 * @param[in] site The site to query.
 * @param[in] prop The property to query.
 * @param[in] size The size of the memory pointed to by @p value in bytes.
 * @param[out] value A pointer to the memory location where the value of the
 * property will be stored.
 * @param[out] size_ret The actual size of the data being queried in bytes.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>.
 */
int DCDB_QDMI_device_session_query_site_property(
    DCDB_QDMI_Device_Session session, DCDB_QDMI_Site site,
    const QDMI_Site_Property prop, const size_t size, void *value,
    size_t *size_ret) {
  return QDMI_ERROR_NOTSUPPORTED;
}

/**
 * @brief Queries a operation property.
 *
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
 * @details The DCDB device does not have any <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/types_8h.html#ae777e8f92186c1e6f836eeaa53b149d7">QDMI_Operation</a>
 * to query.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>.
 */
int DCDB_QDMI_device_session_query_operation_property(
    DCDB_QDMI_Device_Session session, DCDB_QDMI_Operation operation,
    const size_t num_sites, const DCDB_QDMI_Site *sites,
    const size_t num_params, const double *params,
    const QDMI_Operation_Property prop, const size_t size, void *value,
    size_t *size_ret) {
  return QDMI_ERROR_NOTSUPPORTED;
}

/**
 * @brief Query an telemetry sensor property.
 * @param[in] session The session used for the query.
 * @param[in] telemetrysensor The telemetry to query.
 * @param[in] prop The property to query.
 * @param[in] size The size of the memory pointed to by @p value in bytes.
 * @param[out] value A pointer to the memory location where the value of the
 * property will be stored.
 * @param[out] size_ret The actual size of the data being queried in bytes.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 if the device supports the specified property and,
 * when @p value is not @c NULL, the property was successfully retrieved.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 if the device does not support the
 * property.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 if
 *  - @p session or @p query is @c NULL,
 *  - @p prop is invalid, or
 *  - @p value is not @c NULL and @p size is less than the size of the data
 *  being queried.
 * @return <a
href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a916e0810bf915e2ad67f2c1430c54fec">QDMI_ERROR_BADSTATE</a>
 * if the property cannot be queried in the current state of the session, for
example, because the session is not initialized.
 * @return <a
href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 if an unexpected error occurred.

 * @see ADD_STRING_PROPERTY
 * @see ADD_SINGLE_VALUE_PROPERTY
 * @see ADD_LIST_PROPERTY
 */

int DCDB_QDMI_device_session_query_telemetrysensor_property(
    DCDB_QDMI_Device_Session session, DCDB_QDMI_TelemetrySensor telemetrysensor,
    QDMI_TelemetrySensor_Property prop, size_t size, void *value,
    size_t *size_ret) {

  if (session == nullptr || telemetrysensor == nullptr ||
      (value != nullptr && size == 0) ||
      (prop >= QDMI_TELEMETRYSENSOR_PROPERTY_MAX &&
       prop != QDMI_TELEMETRYSENSOR_PROPERTY_CUSTOM1 &&
       prop != QDMI_TELEMETRYSENSOR_PROPERTY_CUSTOM2 &&
       prop != QDMI_TELEMETRYSENSOR_PROPERTY_CUSTOM3 &&
       prop != QDMI_TELEMETRYSENSOR_PROPERTY_CUSTOM4 &&
       prop != QDMI_TELEMETRYSENSOR_PROPERTY_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_STRING_PROPERTY(QDMI_TELEMETRYSENSOR_PROPERTY_ID,
                      telemetrysensor->id.c_str(), prop, size, value, size_ret)
  ADD_STRING_PROPERTY(QDMI_TELEMETRYSENSOR_PROPERTY_UNIT,
                      telemetrysensor->unit.c_str(), prop, size, value,
                      size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_TELEMETRYSENSOR_PROPERTY_SAMPLINGRATE, float,
                            telemetrysensor->sampling_rate.count(), prop, size,
                            value, size_ret)
  return QDMI_ERROR_NOTSUPPORTED;
}

/**
 * @brief Create an telemetry sensor query.
 * @details This is the main entry point for a driver to create an telemetry
 sensor
 * query for a device. The returned handle can be used throughout the "device
 * telemetry sensor query interface" to refer to the telemetry sensor query.
 * @param[in] session The session to create the telemetry sensor query on.
 Must not
 * be @c NULL.
 * @param[out] query A pointer to a handle that will store the created
 * telemetry sensor query. Must not be @c NULL. The telemetry sensor query
 must be freed
 * by calling
 * @ref DCDB_QDMI_device_telemetrysensor_query_free when it is no longer used.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 if the telemetry sensor query was successfully created.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 if @p session or @p query are @c
 * NULL.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a916e0810bf915e2ad67f2c1430c54fec">QDMI_ERROR_BADSTATE</a>
 if the session is not in a state allowing
 * the creation of an telemetry sensor query, for example, because the session
 is not
 * initialized.
 *
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if telemetry sensor query creation failed due to a
 * fatal error.
 *
 * @see DCDB_QDMI_device_telemetrysensor_query_set_parameter
 * @see DCDB_QDMI_device_telemetrysensor_query_submit
 * @see DCDB_QDMI_device_telemetrysensor_query_get_results
 * @see DCDB_QDMI_device_telemetrysensor_query_wait
 * @see DCDB_QDMI_device_telemetrysensor_query_check_status
 * @see DCDB_QDMI_device_telemetrysensor_query_cancel
 * @see DCDB_QDMI_device_telemetrysensor_query_free
 */
int DCDB_QDMI_device_session_create_device_telemetrysensor_query(
    DCDB_QDMI_Device_Session session,
    DCDB_QDMI_Device_TelemetrySensor_Query *query) {

  if (session == nullptr || query == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (session->getStatus() != DCDB_QDMI_DEVICE_SESSION_STATUS::INITIALIZED) {
    return QDMI_ERROR_BADSTATE;
  }
  *query = new DCDB_QDMI_Device_TelemetrySensor_Query_impl_d();
  (*query)->telemetrysensor = new DCDB_QDMI_TelemetrySensor_impl_d();
  (*query)->start_time = 0;
  (*query)->end_time = 0;

  (*query)->session = session;
  return QDMI_SUCCESS;
}

/**
 * @brief Set a parameter for an telemetry sensor query.
 * @param[in] query A handle to an telemetry sensor query for which to set @p
param.
 * Must not be @c NULL.
 * @param[in] param The parameter whose value will be set. Must be one of the
 * values specified for QDMI_Device_TelemetrySensor_Query_Parameter.
 * @param[in] size The size of the data pointed to by @p value in bytes. Must
 * not be zero, except when @p value is @c NULL, in which case it is ignored.
 * @param[in] value A pointer to the memory location that contains the value of
 * the parameter to be set. The data pointed to by @p value is copied and can be
 * safely reused after this function returns. If this is @c NULL, it is ignored.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 if the device supports the specified
 * QDMI_Device_Telemetry_Query_Parameter @p param and, when @p value is not @c
 * NULL, the parameter was successfully set.
 * @return <a
href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
if the device does not support the
 * parameter or the value of the parameter.
 * @return <a
href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
if
 *  - @p query is @c NULL,
 *  - @p param is invalid, or
 *  - @p value is not @c NULL and @p size is zero or not the expected size for
 *    the parameter (if specified by the
 * QDMI_Device_Telemetry_Query_Parameter documentation).
 * @return
<a
href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a916e0810bf915e2ad67f2c1430c54fec">QDMI_ERROR_BADSTATE</a>
if the parameter cannot be set in the
 * current state of the telemetry sensor query, for example, because the
telemetry sensor
 * query is already submitted.
 * @return
<a
href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 * if setting the parameter failed due to a fatal
 * error.
 *
 * @see DCDB_QDMI_Device_TelemetrySensor_Query_impl_d
 * @see DCDB_QDMI_device_session_create_telemetry_query
 * @see DCDB_QDMI_device_telemetrysensor_query_submit
 * @see DCDB_QDMI_device_telemetrysensor_query_get_results
 * @see DCDB_QDMI_device_telemetrysensor_query_wait
 * @see DCDB_QDMI_device_telemetrysensor_query_check_status
 * @see DCDB_QDMI_device_telemetrysensor_query_cancel
 * @see DCDB_QDMI_device_telemetrysensor_query_free
 */

int DCDB_QDMI_device_telemetrysensor_query_set_parameter(
    DCDB_QDMI_Device_TelemetrySensor_Query query,
    QDMI_Device_TelemetrySensor_Query_Parameter param, size_t size,
    const void *value) {

  if (query == nullptr || (value != nullptr && size == 0) ||
      (param >= QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_MAX &&
       param != QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_CUSTOM2 &&
       param != QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_CUSTOM3 &&
       param != QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_CUSTOM4 &&
       param != QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  /* TODO check state*/
  switch (param) {

  case QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_STARTTIME: {
    query->start_time = *(uint64_t *)value;
    return QDMI_SUCCESS;
  }
  case QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_ENDTIME: {
    query->end_time = *(uint64_t *)value;
    return QDMI_SUCCESS;
  }

  case QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_TELEMETRYSENSOR: {
    const auto *telemetry_ptr =
        static_cast<const DCDB_QDMI_TelemetrySensor *>(value);
    query->telemetrysensor = *telemetry_ptr;
    return QDMI_SUCCESS;
  }
  default:
    return QDMI_ERROR_NOTSUPPORTED;
  }
}
/**
 * @brief The auxiliary function to submit an telemetry sensor query.
 *
 * @details This function is used in the @ref
 * DCDB_QDMI_device_telemetrysensor_query_submit to submit an telemetry
 * sensor query asynchronously
 *
 * @param[in] query The telemetry sensor query to submit. Must not be @c NULL.
 *
 * @see DCDB_QDMI_device_telemetrysensor_query_submit
 */
void submit_query(DCDB_QDMI_Device_TelemetrySensor_Query query) {
  query->status = QDMI_TELEMETRYSENSOR_QUERY_STATUS_RUNNING;
  auto connection = query->session->getConnection();
  query->results = query->telemetrysensor->query(connection, query->start_time,
                                                 query->end_time);

  if (query->status != QDMI_TELEMETRYSENSOR_QUERY_STATUS_CANCELED)
    query->status = QDMI_TELEMETRYSENSOR_QUERY_STATUS_DONE;
}

/**
 * @brief Submit an telemetry sensor query to the device.
 * @details This functions asynchronously submit the telemetry sensor query
 using
 * the @ref submit_query function. The @ref
 * DCDB_QDMI_device_telemetrysensor_query_check_status function can be used to
 check
 * the status of the `telemetry sensor query`.
 * @param[in] query The telemetry sensor query to submit. Must not be @c NULL.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 if the job was successfully submitted.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 if @p job is @c NULL.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a916e0810bf915e2ad67f2c1430c54fec">QDMI_ERROR_BADSTATE</a>
 if the job is in an invalid state.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a74b2c0dafe09d9c6d819751e1ec120d3">QDMI_ERROR_FATAL</a>
 if the job submission failed.
 *
 * @see DCDB_QDMI_device_session_create_telemetry_query
 * @see DCDB_QDMI_device_telemetrysensor_query_set_parameter
 * @see DCDB_QDMI_device_telemetrysensor_query_get_results
 * @see DCDB_QDMI_device_telemetrysensor_query_wait
 * @see DCDB_QDMI_device_telemetrysensor_query_check_status
 * @see DCDB_QDMI_device_telemetrysensor_query_cancel
 * @see DCDB_QDMI_Device_TelemetrySensor_Query_free
 *
 */
int DCDB_QDMI_device_telemetrysensor_query_submit(
    DCDB_QDMI_Device_TelemetrySensor_Query query) {

  if (query == nullptr || query->telemetrysensor == nullptr ||
      (query->start_time > query->end_time)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  query->status = QDMI_TELEMETRYSENSOR_QUERY_STATUS_SUBMITTED;
  query->async_query = std::async(submit_query, query);

  return QDMI_SUCCESS;
}
/**
 * @brief Retrieve the results of an telemetry sensor query.
 * @param[in] query The telemetry sensor query to retrieve the results from.
 Must not
 * be @c NULL.
 * @param[in] result The result to retrieve. Must be one of the values specified
 * for QDMI_TelemetrySensor_Query_Result.
 * @param[in] size The size of the buffer pointed to by @p data in bytes. Must
 * be greater or equal to the size of the return type specified for the
 * QDMI_TelemetrySensor_Query_Result @p result, except when @p data is @c
 NULL, in
 * which case it is ignored.
 * @param[out] data A pointer to the memory location where the results will be
 * stored. If this is @c NULL, it is ignored.
 * @param[out] size_ret The actual size of the data being queried in bytes. If
 * this is @c NULL, it is ignored.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 if the device supports the specified result and,
 * when @p data is not @c NULL, the results were successfully retrieved.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if
 *  - @p query is @c NULL,
 *  - @p query has not finished,
 *  - @p query was canceled,
 *  - @p result is invalid, or
 *  - @p data is not @c NULL and @p size is smaller than the size of the data
 *    being queried.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 * if @p result is not supported.
 *
 * @see DCDB_QDMI_device_session_create_telemetry_query
 * @see DCDB_QDMI_device_telemetrysensor_query_set_parameter
 * @see DCDB_QDMI_device_telemetrysensor_query_submit
 * @see DCDB_QDMI_device_telemetrysensor_query_wait
 * @see DCDB_QDMI_device_telemetrysensor_query_check_status
 * @see DCDB_QDMI_device_telemetrysensor_query_cancel
 * @see DCDB_QDMI_Device_TelemetrySensor_Query_free
 */
int DCDB_QDMI_device_telemetrysensor_query_get_results(
    DCDB_QDMI_Device_TelemetrySensor_Query query,
    QDMI_TelemetrySensor_Query_Result result, size_t size, void *data,
    size_t *size_ret) {

  if (query == nullptr || (data != nullptr && size == 0) ||
      (result >= QDMI_TELEMETRYSENSOR_QUERY_RESULT_MAX &&
       result != QDMI_TELEMETRYSENSOR_QUERY_RESULT_CUSTOM1 &&
       result != QDMI_TELEMETRYSENSOR_QUERY_RESULT_CUSTOM2 &&
       result != QDMI_TELEMETRYSENSOR_QUERY_RESULT_CUSTOM3 &&
       result != QDMI_TELEMETRYSENSOR_QUERY_RESULT_CUSTOM4 &&
       result != QDMI_TELEMETRYSENSOR_QUERY_RESULT_CUSTOM5) ||
      query->status != QDMI_TELEMETRYSENSOR_QUERY_STATUS_DONE) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  size_t req_size = query->results.size();
  switch (result) {
  case QDMI_TELEMETRYSENSOR_QUERY_RESULT_TIMESTAMPS:
    req_size *= sizeof(uint64_t);
    if (data != nullptr) {
      if (size < req_size) {
        return QDMI_ERROR_INVALIDARGUMENT;
      }
      uint64_t *data_ptr = static_cast<uint64_t *>(data);
      for (DCDB::SensorDataStoreReading reading : query->results) {
        *data_ptr++ = reading.timeStamp.getRaw();
      }
    }

    if ((size_ret) != nullptr) {
      *(size_ret) = req_size;
    }
    return QDMI_SUCCESS;
  case QDMI_TELEMETRYSENSOR_QUERY_RESULT_VALUES:

    req_size *= sizeof(float);
    if (data != nullptr) {
      if (size < req_size) {
        return QDMI_ERROR_INVALIDARGUMENT;
      }
      int64_t *data_ptr = static_cast<int64_t *>(data);
      for (DCDB::SensorDataStoreReading reading : query->results)
        *data_ptr++ = reading.value;
    }
    if ((size_ret) != nullptr) {
      *(size_ret) = req_size;
    }
    return QDMI_SUCCESS;

  default:
    return QDMI_ERROR_NOTSUPPORTED;
  }

  return QDMI_SUCCESS;
}

/**
 * @brief Check the status of the telemetry sensor query.
 * @details This function is non-blocking and returns immediately with the
 * telemetry sensor query status. Since @ref
 * DCDB_QDMI_device_telemetrysensor_query_submit submits the telemetry
 sensor query
 * asynchronously, The `QDMI_Client` must check the status of the telemetry
 sensor
 * query before calling the @ref
 DCDB_QDMI_device_telemetrysensor_query_get_results.
 * @param[in] query The telemetry sensor query to check the status of. Must
 not be @c
 * NULL.
 * @param[out] status The status of the telemetry sensor query. Must not be @c
 NULL.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 if the telemetry sensor query status was successfully
 * checked.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 if @p query or @p status is @c NULL.
 *
 * @see DCDB_QDMI_device_session_create_telemetry_query
 * @see DCDB_QDMI_device_telemetrysensor_query_set_parameter
 * @see DCDB_QDMI_device_telemetrysensor_query_submit
 * @see DCDB_QDMI_device_telemetrysensor_query_wait
 * @see DCDB_QDMI_device_telemetrysensor_query_get_results
 * @see DCDB_QDMI_device_telemetrysensor_query_cancel
 * @see DCDB_QDMI_Device_TelemetrySensor_Query_free
 */
int DCDB_QDMI_device_telemetrysensor_query_check_status(
    DCDB_QDMI_Device_TelemetrySensor_Query query,
    QDMI_TelemetrySensor_Query_Status *status) {
  if (query == nullptr || status == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  *status = query->status;
  return QDMI_SUCCESS;
}

/**
 * @brief Wait for an telemetry sensor query to finish.
 * @details This function blocks until the @ref submit_query function has
 * finished or has been canceled.
 * @param[in] query The telemetry sensor query to wait for. Must not be @c
 * NULL.
 * @param[in] timeout The timeout in seconds.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 if the telemetry sensor query is finished or canceled.
 * @return <a
 href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 if @p query is @c NULL.
 *
 * @see DCDB_QDMI_device_session_create_telemetry_query
 * @see DCDB_QDMI_device_telemetrysensor_query_set_parameter
 * @see DCDB_QDMI_device_telemetrysensor_query_submit
 * @see DCDB_QDMI_device_telemetrysensor_query_check_status
 * @see DCDB_QDMI_device_telemetrysensor_query_get_results
 * @see DCDB_QDMI_device_telemetrysensor_query_cancel
 * @see DCDB_QDMI_Device_TelemetrySensor_Query_free
 *
 */

int DCDB_QDMI_device_telemetrysensor_query_wait(
    DCDB_QDMI_Device_TelemetrySensor_Query query, size_t timeout) {

  if (query == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  query->async_query.wait();

  query->status = QDMI_TELEMETRYSENSOR_QUERY_STATUS_DONE;
  return QDMI_SUCCESS;
}

/**
 * @brief Cancel an already submitted telemetry sensor query.
 * @details This function changes the status of the telemetry sensor query to
 * QDMI_TELEMETRYSENSOR_QUERY_STATUS_CANCELED.
 * @param[in] query The telemetry sensor query to cancel. Must not be @c NULL.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a8039f5cd8202553b2a91a1c0b01d6751">QDMI_SUCCESS</a>
 * if the telemetry sensor query was successfully canceled.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a72b5274b4f2a76101255ac8409410642">QDMI_ERROR_INVALIDARGUMENT</a>
 * if @p query is @c NULL or the job
 * already has the status QDMI_TELEMETRYSENSOR_QUERY_STATUS_DONE.
 *
 * @see DCDB_QDMI_device_session_create_telemetry_query
 * @see DCDB_QDMI_device_telemetrysensor_query_set_parameter
 * @see DCDB_QDMI_device_telemetrysensor_query_submit
 * @see DCDB_QDMI_device_telemetrysensor_query_check_status
 * @see DCDB_QDMI_device_telemetrysensor_query_get_results
 * @see DCDB_QDMI_device_telemetrysensor_query_wait
 * @see DCDB_QDMI_Device_TelemetrySensor_Query_free
 */
int DCDB_QDMI_device_telemetrysensor_query_cancel(
    DCDB_QDMI_Device_TelemetrySensor_Query query) {

  if (query == nullptr ||
      query->status == QDMI_TELEMETRYSENSOR_QUERY_STATUS_DONE) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  query->status = QDMI_TELEMETRYSENSOR_QUERY_STATUS_CANCELED;

  return QDMI_SUCCESS;
}

/**
 * @brief Free an telemetry sensor query.
 * @details Free the resources associated with a telemetry sensor query. Using
 * a telemetry sensor query handle after it has been freed is undefined
 * behavior.
 * @param[in] query The telemetry sensor query to free.
 *
 * @see DCDB_QDMI_device_session_create_telemetry_query
 * @see DCDB_QDMI_device_telemetrysensor_query_set_parameter
 * @see DCDB_QDMI_device_telemetrysensor_query_submit
 * @see DCDB_QDMI_device_telemetrysensor_query_check_status
 * @see DCDB_QDMI_device_telemetrysensor_query_get_results
 * @see DCDB_QDMI_device_telemetrysensor_query_wait
 * @see DCDB_QDMI_device_telemetrysensor_query_cancel
 */
void DCDB_QDMI_device_telemetrysensor_query_free(
    DCDB_QDMI_Device_TelemetrySensor_Query query) {
  delete query;
}
/**
 * @brief Query a job property.
 * @details @see DCDB_QDMI_device_session_create_device_job
 * @param[in] job A handle to a job for which to query @p prop. Must not be @c
 * NULL.
 * @param[in] prop The property to query. Must be one of the values specified
 * for <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a6e4d18c7fa5d383bbcc1498abe090d4f">QDMI_Device_Job_Property</a>
 * @param[in] size The size of the memory pointed to by @p value in bytes. Must
 * be greater or equal to the size of the return type specified for @p prop,
 * except when @p value is @c NULL, in which case it is ignored.
 * @param[out] value A pointer to the memory location where the value of the
 * property will be stored. If this is @c NULL, it is ignored.
 * @param[out] size_ret The actual size of the data being queried in bytes. If
 * this is @c NULL, it is ignored.
 * @return <a
 * href="https://munich-quantum-software-stack.github.io/QDMI/constants_8h.html#a450b1adf81abc6f0accbf0ce4abe92f8a327c1ff469cce7beacddd9c6d428b651">QDMI_ERROR_NOTSUPPORTED</a>
 */
int DCDB_QDMI_device_job_query_property(DCDB_QDMI_Device_Job job,
                                        QDMI_Device_Job_Property prop,
                                        size_t size, void *value,
                                        size_t *size_ret) {
  return QDMI_ERROR_NOTSUPPORTED;
}
