// ParityQC © 2025. See LICENSE.txt in /src/isv/parityqc/ for details.

/** @file
 * @brief The QDMI Device implementation of the ParityOS Device at Leibniz
 * Supercomputing Centre.
 * @details TODO: add description!
 */

#include "qdmi/constants.h"
#include <Python.h>
#include <assert.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <parityos_qdmi/device.h>
#include <string>
#include <string_view>

//===----------------------------------------------------------------------===//
// Private code 1/2
//===----------------------------------------------------------------------===//

namespace {

/// Environment variable containing absolute path to the folder containing all
/// relevant python scripts.
#define SCRIPT_PATH "PARITYQC_SCRIPT_PATH"
/// Environment variable containing basename of the python script without
/// extension (e.g. `my_script`, not `my_script.py`).
#define SCRIPT_NAME "PARITYQC_PARITYOS_WRAPPER_SCRIPT_NAME"

/// TODO: setting the initial value upon first usage is a bit indirect. We
/// should put all the python stuff into one object for better resource
/// management.
///
/// It can happen that our device is called within a process which already runs
/// python. In that case one must not initialize (and release) the python
/// interpreter.
bool is_from_python() {
  static bool is_initialized = Py_IsInitialized() != 0;
  return is_initialized;
}

/// RAII pattern to manage python's global interpreter lock (GIL).
struct GilGuard {
  GilGuard() : gstate(PyGILState_LOCKED) {
    // NOTE: It is realistic that the API around PyGILState becomes deprecated
    // (see https://peps.python.org/pep-0788/, as of writing this the pep is
    // still a draft).
    gstate = PyGILState_Ensure();
  }

  ~GilGuard() { PyGILState_Release(gstate); }

private:
  PyGILState_STATE gstate;
};

enum class SESSION_STATUS {
  /// Session starts in allocated state.
  ALLOCATED,
  /// A session moves to the initialized state once
  /// ParityOS_QDMI_device_session_init was called.
  INITIALIZED,
};

/// TODO: handle device status? For us not everything makes sense, but the
/// following might make sense: OFFLINE, MAINTENANCE. Unfortunately there is
/// nothing in there for "everything is fine".

/** @brief Checks if there is a python related error.
 *
 * This macro checks whether the `value` is `nullptr` or `Py_None`. If it is, it
 * returns a `QDMI_ERROR_FATAL`.
 *
 * TODO: emitting FATAL on any error is not helpful for error diagnostics. Work
 * out a better strategy to communicate the error.
 */
#define CHECK_PYTHON_ERROR(value)                                              \
  {                                                                            \
    if (value == Py_None || value == nullptr) {                                \
      PyErr_Print();                                                           \
      return QDMI_ERROR_FATAL;                                                 \
    }                                                                          \
  }

/// Refers to the parityos wrapper script.
PyObject **get_parityos_wrapper_module() {
  static PyObject *python_module = nullptr;
  return &python_module;
}

/**
 * Call this function before any other python related functions.
 *
 * It checks if python is already initialized. If not it will initialize it.
 * Then the parityos wrapper script is loaded and available via
 * `get_parityos_module`.
 */
QDMI_STATUS initialize_python() {
  const auto script_path = std::getenv(SCRIPT_PATH);
  const auto script_name = std::getenv(SCRIPT_NAME);

  /// Useful for development. TODO: logger would be better for production.
  assert(script_path && "Missing script path");
  assert(script_name && "Missing script name");

  if (std::strlen(script_path) == 0 || std::strlen(script_name) == 0)
    return QDMI_ERROR_FATAL;

  if (!is_from_python()) {
    Py_Initialize();
    PyThreadState *_save = PyEval_SaveThread();
  }

  auto _gil_quard = GilGuard();

  PyObject *sys = PyImport_ImportModule("sys");
  PyObject *sys_path = PyObject_GetAttrString(sys, "path");

  PyList_Append(sys_path, PyUnicode_FromString(script_path));

  PyObject *pName = PyUnicode_DecodeFSDefault(script_name);
  CHECK_PYTHON_ERROR(pName);

  auto py_module = PyImport_Import(pName);
  *get_parityos_wrapper_module() = py_module;
  CHECK_PYTHON_ERROR(*get_parityos_wrapper_module());

  Py_XDECREF(pName);

  return QDMI_SUCCESS;
}

/// Calls the python function with the same name (and *essentially* the same
/// signature).
QDMI_STATUS create_parityos_client(PyObject **out, std::string_view base_url) {
  auto _gil_quard = GilGuard();

  /// TODO: we should distinguish FATAL from PERMISSION DENIED errors.

  PyObject *pFunc = PyObject_GetAttrString(*get_parityos_wrapper_module(),
                                           "create_parityos_client");
  CHECK_PYTHON_ERROR(pFunc);

  PyObject *pArgs = PyTuple_Pack(1, PyUnicode_FromString(base_url.data()));
  CHECK_PYTHON_ERROR(pArgs);

  PyObject *pResult = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pResult);

  *out = pResult;
  return QDMI_SUCCESS;
}

} // namespace

//===----------------------------------------------------------------------===//
// Macros
//===----------------------------------------------------------------------===//

/// Copy a scalar value and report its size:
/// - copy `prop_value` into `value` (conditionally)
/// - `size_ret` reports the length of the copied value (conditionally)
#define ADD_SINGLE_VALUE_PROPERTY(prop_name, prop_type, prop_value, prop,      \
                                  size, value, size_ret)                       \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < sizeof(prop_type)) {                                      \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        *static_cast<prop_type *>(value) = prop_value;                         \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = sizeof(prop_type);                                         \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

/// Copy a string and report its length including null-terminator:
/// - copy string `prop_value` into `value` (conditionally)
/// - `size_ret` reports the length of the copied value (conditionally)
#define ADD_STRING_PROPERTY(prop_name, prop_value, prop, size, value,          \
                            size_ret)                                          \
  {                                                                            \
    if ((prop) == (prop_name)) {                                               \
      if ((value) != nullptr) {                                                \
        if ((size) < strlen(prop_value) + 1) {                                 \
          return QDMI_ERROR_INVALIDARGUMENT;                                   \
        }                                                                      \
        strncpy(static_cast<char *>(value), prop_value, size);                 \
        static_cast<char *>(value)[size - 1] = '\0';                           \
      }                                                                        \
      if ((size_ret) != nullptr) {                                             \
        *size_ret = strlen(prop_value) + 1;                                    \
      }                                                                        \
      return QDMI_SUCCESS;                                                     \
    }                                                                          \
  }

/// If `value` is not a qdmi success return it.
#define CHECK_QDMI_ERROR(value)                                                \
  {                                                                            \
    if (value != QDMI_SUCCESS) {                                               \
      return value;                                                            \
    }                                                                          \
  }

//===----------------------------------------------------------------------===//
// QDMI opaque types implementation
//===----------------------------------------------------------------------===//

struct ParityOS_QDMI_Device_Session_impl_d {
  enum SESSION_STATUS status = SESSION_STATUS::ALLOCATED;
  /// For authentication with parityapi, e.g. `https://api.parityqc.com/`.
  std::string base_url = "";
  /// Let it be hardcoded for now.
  const unsigned api_version = 3;
  /// This is set iff it is in the `INITIALIZED` status (authentication with
  /// parityapi was successfull). The session owns the client.
  PyObject *client = nullptr;

  /// Whether the session has set all fields relevant for authentication with
  /// parityos.
  bool has_auth_data() { return base_url != ""; }

  ~ParityOS_QDMI_Device_Session_impl_d() {
    auto _gil_quard = GilGuard();
    Py_XDECREF(client);
  }
};

/// TODO: We might want to refactor this object once we have a working
/// prototype.
struct ParityOS_QDMI_Device_Job_impl_d {
  // Const fields:

  /// NOTE: Session *not* managed by this object!
  const ParityOS_QDMI_Device_Session session = nullptr;

  // Mutable fields:

  QDMI_Job_Status status = QDMI_JOB_STATUS_CREATED;
  /// Custom json format for the problem representation. We probably cannot
  /// validate at this point if the format is consistent with program. I think
  /// it is OK that the service just returns an error if the program is not as
  /// the format claims.
  QDMI_Program_Format format = QDMI_PROGRAM_FORMAT_CUSTOM1;
  /// ParityQC problem representation in json
  /// format
  std::string program;
  /// Gets a valid (strictly positive) value only after the job is submitted
  /// (status).
  unsigned long long submission_id = 0;
  /// A quake circuit solving the problem. TODO: In what sense (QAOA etc.)?
  std::string result;

  ParityOS_QDMI_Device_Job_impl_d(const ParityOS_QDMI_Device_Session session_)
      : session(session_) {
    assert(session != nullptr && "session must not be null");
  }
};

/// Trivial implementation because we provide a compilation service.
struct ParityOS_QDMI_Site_impl_d {};

/// Trivial implementation because we provide a compilation service.
struct ParityOS_QDMI_Operation_impl_d {};

//===----------------------------------------------------------------------===//
// Private code 2/2
//===----------------------------------------------------------------------===//

/// Call the python function with the same name and store the `submission_id` in
/// the `job`.
QDMI_STATUS submit_job(ParityOS_QDMI_Device_Job job) {
  assert(job && "job must not be null");
  assert(job->session->status == SESSION_STATUS::INITIALIZED &&
         "session not initialized");

  job->status = QDMI_JOB_STATUS_SUBMITTED;

  auto _gil_quard = GilGuard();

  PyObject *py_module = *get_parityos_wrapper_module();

  PyObject *pFunc = PyObject_GetAttrString(py_module, "submit_job");
  CHECK_PYTHON_ERROR(pFunc);

  /// TODO: Double check with program format if it is meant to be json.
  PyObject *json_string = PyUnicode_FromString(job->program.c_str());
  CHECK_PYTHON_ERROR(json_string);

  PyObject *pArgs = PyTuple_Pack(2, job->session->client, json_string);
  CHECK_PYTHON_ERROR(pArgs);

  job->status = QDMI_JOB_STATUS_RUNNING;

  PyObject *py_submission_id = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(py_submission_id);
  job->submission_id = PyLong_AsUnsignedLongLong(py_submission_id);
  if (PyErr_Occurred()) {
    return QDMI_ERROR_FATAL;
  }

  return QDMI_SUCCESS;
}

/** Call the python function of the same name.
 *
 * We call the parityos API to ask for the results of the job, using the
 * submission id to identify it. If the result is not yet available result will
 * contain a null option.
 *
 * TODO: The python side is not yet fully implemented, but once it is we intend
 * to store the result locally in order to quickly retrieve it once we have
 * already recieved it. That way you can use the function to test whether the
 * result is available. Once it is you can be assured that retrieval will be
 * quick.
 */
QDMI_STATUS poll_result(std::optional<std::string> &result,
                        ParityOS_QDMI_Device_Job job) {
  assert(job && "job must not be null");
  assert(job->status >= QDMI_JOB_STATUS_SUBMITTED && "job must be submitted");
  assert(job->status <= QDMI_JOB_STATUS_DONE &&
         "job must be in a normal state");

  auto _gil_quard = GilGuard();

  PyObject *py_module = *get_parityos_wrapper_module();

  PyObject *pFunc = PyObject_GetAttrString(py_module, "poll_result");
  CHECK_PYTHON_ERROR(pFunc);

  PyObject *py_submission_id = PyLong_FromUnsignedLongLong(job->submission_id);

  PyObject *pArgs = PyTuple_Pack(1, py_submission_id);
  CHECK_PYTHON_ERROR(pArgs);

  PyObject *py_result = PyObject_CallObject(pFunc, pArgs);

  if (py_result == Py_None) {
    result = std::nullopt;
    return QDMI_SUCCESS;
  }

  if (!PyUnicode_Check(py_result)) {
    return QDMI_ERROR_FATAL;
  }

  result = PyUnicode_AsUTF8(py_result);

  return QDMI_SUCCESS;
}

//===----------------------------------------------------------------------===//
// QDMI device API implementation
//===----------------------------------------------------------------------===//

int ParityOS_QDMI_device_initialize(void) {
  CHECK_QDMI_ERROR(initialize_python());
  return QDMI_SUCCESS;
}

int ParityOS_QDMI_device_finalize(void) {
  if (*get_parityos_wrapper_module() != nullptr) {
    Py_DECREF(*get_parityos_wrapper_module());
    *get_parityos_wrapper_module() = nullptr;
  }

  if (Py_IsInitialized() && !_Py_IsFinalizing() && !is_from_python()) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    Py_Finalize();
  }

  return QDMI_SUCCESS;
}

int ParityOS_QDMI_device_session_alloc(ParityOS_QDMI_Device_Session *session) {
  if (session == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  *session = new ParityOS_QDMI_Device_Session_impl_d();

  return QDMI_SUCCESS;
}

void ParityOS_QDMI_device_session_free(ParityOS_QDMI_Device_Session session) {
  if (session != nullptr) {
    delete session;
    session = nullptr;
  }
}

int ParityOS_QDMI_device_session_init(ParityOS_QDMI_Device_Session session) {
  if (session == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  // After a session is successfully initialized we do not allow any further
  // attempts.
  if (session->status != SESSION_STATUS::ALLOCATED) {
    return QDMI_ERROR_BADSTATE;
  }

  if (!session->has_auth_data()) {
    return QDMI_ERROR_PERMISSIONDENIED;
  }

  CHECK_QDMI_ERROR(create_parityos_client(&session->client, session->base_url));

  session->status = SESSION_STATUS::INITIALIZED;
  return QDMI_SUCCESS;
}

int ParityOS_QDMI_device_session_set_parameter(
    ParityOS_QDMI_Device_Session session,
    const QDMI_Device_Session_Parameter param, const size_t size,
    const void *value) {
  if (session == nullptr || (value != nullptr && size == 0) ||
      (param >= QDMI_DEVICE_SESSION_PARAMETER_MAX &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM3 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM4 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (session->status != SESSION_STATUS::ALLOCATED) {
    /// Ensures that this is called before ParityOS_QDMI_device_session_init.
    return QDMI_ERROR_BADSTATE;
  }

  if (param != QDMI_DEVICE_SESSION_PARAMETER_BASEURL) {
    return QDMI_ERROR_NOTSUPPORTED;
  }

  if (value != nullptr) {
    switch (param) {
    case QDMI_DEVICE_SESSION_PARAMETER_BASEURL:
      session->base_url = std::string(static_cast<const char *>(value), size);
      break;
    default:
      return QDMI_ERROR_NOTSUPPORTED;
    }
  }

  return QDMI_SUCCESS;
}

int ParityOS_QDMI_device_session_create_device_job(
    ParityOS_QDMI_Device_Session session, ParityOS_QDMI_Device_Job *job) {
  if (session == nullptr || job == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (session->status != SESSION_STATUS::INITIALIZED) {
    return QDMI_ERROR_BADSTATE;
  }

  *job = new ParityOS_QDMI_Device_Job_impl_d(session);

  return QDMI_SUCCESS;
}

void ParityOS_QDMI_device_job_free(ParityOS_QDMI_Device_Job job) { delete job; }

int ParityOS_QDMI_device_job_set_parameter(
    ParityOS_QDMI_Device_Job job, const QDMI_Device_Job_Parameter param,
    const size_t size, const void *value) {
  if (job == nullptr || (value != nullptr && size == 0) ||
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

  switch (param) {
  case QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT:
    if (value != nullptr) {
      const auto format = *static_cast<const QDMI_Program_Format *>(value);

      if (format >= QDMI_PROGRAM_FORMAT_MAX &&
          format != QDMI_PROGRAM_FORMAT_CUSTOM1 &&
          format != QDMI_PROGRAM_FORMAT_CUSTOM2 &&
          format != QDMI_PROGRAM_FORMAT_CUSTOM3 &&
          format != QDMI_PROGRAM_FORMAT_CUSTOM4 &&
          format != QDMI_PROGRAM_FORMAT_CUSTOM5) {
        return QDMI_ERROR_INVALIDARGUMENT;
      }

      /// TODO: our job format is the json form of the problem representation
      if (format != QDMI_PROGRAM_FORMAT_CUSTOM1) {
        return QDMI_ERROR_NOTSUPPORTED;
      }

      job->format = format;
    }
    return QDMI_SUCCESS;
  case QDMI_DEVICE_JOB_PARAMETER_PROGRAM:
    if (value != nullptr) {
      job->program = std::string(static_cast<const char *>(value), size);
    }
    return QDMI_SUCCESS;
  default:
    return QDMI_ERROR_NOTSUPPORTED;
  }
}

int ParityOS_QDMI_device_job_query_property(ParityOS_QDMI_Device_Job job,
                                            const QDMI_Device_Job_Property prop,
                                            const size_t size, void *value,
                                            size_t *size_ret) {
  if (job == nullptr || (value != nullptr && size == 0) ||
      (prop >= QDMI_DEVICE_JOB_PROPERTY_MAX &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM1 &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM2 &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM3 &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM4 &&
       prop != QDMI_DEVICE_JOB_PROPERTY_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  const auto id_str = std::to_string(job->submission_id);

  ADD_STRING_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_ID, id_str.c_str(), prop, size,
                      value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_JOB_PROPERTY_PROGRAMFORMAT,
                            QDMI_Program_Format, job->format, prop, size, value,
                            size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

int ParityOS_QDMI_device_job_submit(ParityOS_QDMI_Device_Job job) {
  // According to interface:
  if (job == nullptr || job->status != QDMI_JOB_STATUS_CREATED) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  CHECK_QDMI_ERROR(submit_job(job));

  return QDMI_SUCCESS;
}

int ParityOS_QDMI_device_job_cancel(ParityOS_QDMI_Device_Job job) {
  if (job == nullptr || job->status == QDMI_JOB_STATUS_DONE) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  /// TODO:
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int ParityOS_QDMI_device_job_check(ParityOS_QDMI_Device_Job job,
                                   QDMI_Job_Status *status) {
  if (job == nullptr || status == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  /// TODO:
  return QDMI_ERROR_NOTIMPLEMENTED;
}

int ParityOS_QDMI_device_job_wait(ParityOS_QDMI_Device_Job job,
                                  const size_t timeout) {
  if (job == nullptr) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  if (timeout != 0) {
    return QDMI_ERROR_NOTSUPPORTED;
  }

  if (job->status == QDMI_JOB_STATUS_DONE) {
    return QDMI_SUCCESS;
  }

  if (job->status < QDMI_JOB_STATUS_DONE) {
    /// TODO: At the moment the job is synchronous so we only have to retrieve
    /// the result without error.
    std::optional<std::string> maybe_result;
    CHECK_QDMI_ERROR(poll_result(maybe_result, job));

    if (maybe_result.has_value()) {
      job->status = QDMI_JOB_STATUS_DONE;
      job->result = maybe_result.value();
    }

    return QDMI_SUCCESS;
  }

  return QDMI_ERROR_FATAL;
}

int ParityOS_QDMI_device_job_get_results(ParityOS_QDMI_Device_Job job,
                                         const QDMI_Job_Result result,
                                         const size_t size, void *data,
                                         size_t *size_ret) {
  if (job == nullptr || job->status != QDMI_JOB_STATUS_DONE ||
      (data != nullptr && size == 0) ||
      (result >= QDMI_JOB_RESULT_MAX && result != QDMI_JOB_RESULT_CUSTOM1 &&
       result != QDMI_JOB_RESULT_CUSTOM2 && result != QDMI_JOB_RESULT_CUSTOM3 &&
       result != QDMI_JOB_RESULT_CUSTOM4 &&
       result != QDMI_JOB_RESULT_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  switch (result) {
  case QDMI_JOB_RESULT_CUSTOM1: {
    auto result_size = job->result.size() + 1; // including \0 terminator
    if (data != nullptr && size < result_size) {
      return QDMI_ERROR_INVALIDARGUMENT;
    }

    if (size_ret)
      *size_ret = result_size;

    if (data)
      memcpy(data, job->result.data(), size);

    return QDMI_SUCCESS;
  }
  default:
    return QDMI_ERROR_NOTSUPPORTED;
  }
}

int ParityOS_QDMI_device_session_query_device_property(
    ParityOS_QDMI_Device_Session session, const QDMI_Device_Property prop,
    const size_t size, void *value, size_t *size_ret) {
  if (prop >= QDMI_DEVICE_PROPERTY_MAX ||
      (value == nullptr && size_ret == nullptr)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  /// TODO: we do not have terribly important info to provide. But those entries
  /// are just adhoc value. Reconsider them at some point.

  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_NAME, "ParityOS", prop, size, value,
                      size_ret)
  // TODO: Version of parityos?
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_VERSION, "3.0.0", prop, size, value,
                      size_ret)
  // TODO: Version of QDMI. Hardcode at implementation time? Retrieve from
  // somewhere?
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "1.2.0", prop, size,
                      value, size_ret)

  // Anything else refers to a quantum computer.
  return QDMI_ERROR_NOTSUPPORTED;
}

int ParityOS_QDMI_device_session_query_site_property(
    ParityOS_QDMI_Device_Session session, ParityOS_QDMI_Site site,
    const QDMI_Site_Property prop, const size_t size, void *value,
    size_t *size_ret) {
  if (session == nullptr || site == nullptr ||
      (value != nullptr && size == 0) ||
      (prop >= QDMI_SITE_PROPERTY_MAX && prop != QDMI_SITE_PROPERTY_CUSTOM1 &&
       prop != QDMI_SITE_PROPERTY_CUSTOM2 &&
       prop != QDMI_SITE_PROPERTY_CUSTOM3 &&
       prop != QDMI_SITE_PROPERTY_CUSTOM4 &&
       prop != QDMI_SITE_PROPERTY_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  assert(false && "unreachable: user cannot create a site (in a normal way).");

  return QDMI_ERROR_NOTSUPPORTED;
}

int ParityOS_QDMI_device_session_query_operation_property(
    ParityOS_QDMI_Device_Session session, ParityOS_QDMI_Operation operation,
    const size_t num_sites, const ParityOS_QDMI_Site *sites,
    const size_t num_params, const double *params, QDMI_Operation_Property prop,
    const size_t size, void *value, size_t *size_ret) {
  if (session == nullptr || operation == nullptr ||
      (sites != nullptr && num_sites == 0) ||
      (params != nullptr && num_params == 0) ||
      (value != nullptr && size == 0) ||
      (prop >= QDMI_OPERATION_PROPERTY_MAX &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM1 &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM2 &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM3 &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM4 &&
       prop != QDMI_OPERATION_PROPERTY_CUSTOM5)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  assert(false &&
         "unreachable: user cannot create an operation (in a normal way).");

  return QDMI_ERROR_NOTSUPPORTED;
}
