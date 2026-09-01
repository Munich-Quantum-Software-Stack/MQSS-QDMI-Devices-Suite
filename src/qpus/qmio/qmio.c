/*------------------------------------------------------------------------------
Copyright 2025 Munich Quantum Software Stack Project / CESGA

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

/** @file
 * @brief The QDMI Device implementation of the QMIO QPU at CESGA (Centro de
 * Supercomputación de Galicia).
 *
 * @details QMIO is a 32-qubit superconducting QPU (OQC) tightly integrated
 * with CESGA's HPC infrastructure. Access is performed through CESGA's
 * `qmiotools` Python package: circuits are executed via the Qiskit backend
 * `QmioBackend(calibration_file=...)`. From the caller's side this is a
 * plain synchronous Qiskit BackendV2 -- `.run()` blocks and returns a
 * completed Result. There is no host/URL to connect to and no
 * submit-then-poll step; all ZMQ communication with the Quantum Control
 * Node is internal to qmiotools.
 *
 * This device therefore embeds a Python interpreter (same pattern as the
 * Qaptiva device) and delegates QMIO-specific work to `qmio_auxiliary.py`.
 *
 * Deployment model (confirmed against CESGA's internal qpu-latency-benchmark
 * project): the process embedding this device (QDMI client + driver +
 * device) must itself be launched inside a Slurm allocation of the `qpu`
 * partition, e.g. via `sbatch`, after `module load qmio/hpc
 * qmio-tools/... qiskit/...`. There is no remote submission from this
 * device -- the *entire process* runs on a qpu-partition node, wrapped by
 * whatever sbatch script the deployment uses (see
 * docs/qmio_deployment.md for a template).
 *
 * Authentication to CESGA (username + password over SSH) happens once,
 * interactively or via key, to reach the login node from which `sbatch` is
 * called. It happens *before* this device (or the process embedding it) is
 * ever started, so QDMI's standard USERNAME/PASSWORD/BASEURL session
 * parameters are accepted-and-ignored here rather than consumed -- see
 * QMIO_QDMI_device_session_set_parameter.
 *
 * What this device DOES need at session-init time is which calibration
 * snapshot to load: QMIO's calibration data is periodically dumped to
 * timestamped JSON files (e.g. `2026_03_13__12_00_01.json`). The qmiotools
 * library discovers the newest one from the directory named by the
 * `QMIO_CALIBRATIONS` environment variable; an explicit file may instead be
 * pinned via session parameter CUSTOM1 for reproducibility across a sweep.
 * We rely on the library's own discovery rather than reimplementing it.
 *
 * For CI/development without a QPU allocation, session parameter CUSTOM2
 * (nonzero) selects `FakeQmio` -- an AerSimulator built from the same
 * calibration data with a thermal-relaxation noise model. It still needs
 * calibration files present, but runs anywhere qmiotools + qiskit-aer are
 * installed.
 *
 * Native gate set (verified against qmiotools' QmioBackend Target): sx, x,
 * rz(theta), ecr (the only native 2-qubit gate), measure, delay.
 */

#include <Python.h>
#include <qmio_qdmi/device.h>
#include <qmio_qdmi/types.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/// Environment variable: directory containing qmio_auxiliary.py
#define SCRIPT_LOCATION "QMIO_AUXILIARY_SCRIPT_LOCATION"
/// Environment variable: module name of the auxiliary script (qmio_auxiliary)
#define SCRIPT_NAME "QMIO_AUXILIARY_SCRIPT_NAME"

#define CREATE_BACKEND_FUNCTION_NAME "create_backend"
#define GET_METADATA_FUNCTION_NAME "get_metadata"
#define GET_SITE_PROPERTIES_FUNCTION_NAME "get_site_properties"
#define SUBMIT_JOB_FUNCTION_NAME "submit_job"

/// Maximum number of operations we expect to expose.
#define MAX_NUM_OP 32

enum QMIO_QDMI_DEVICE_SESSION_STATUS { ALLOCATED, INITIALIZED };

typedef struct QMIO_QDMI_Operation_impl_d {
  char *name;
  size_t n_qubit;
  size_t n_param;
} QMIO_QDMI_Operation_impl_t;

struct QMIO_QDMI_Site_impl_d {
  size_t index;
  /// Per-qubit calibration data from the backend Target's qubit_properties.
  /// T1/T2 in seconds, frequency in Hz. NaN where the qubit has no
  /// properties (e.g. excluded by calibration).
  double t1;
  double t2;
  double frequency;
};

typedef struct QMIO_QDMI_Device_Session_impl_d {
  /// Explicit calibration file path (session param CUSTOM1). If empty, the
  /// qmiotools library auto-discovers the newest snapshot from the
  /// $QMIO_CALIBRATIONS directory -- we do not reimplement that discovery.
  char *calibration_file;
  /// If nonzero, build FakeQmio (AerSimulator + QMIO noise model) instead
  /// of the real QmioBackend (session param CUSTOM2). Still needs
  /// calibration data, but runs without a qpu-partition allocation --
  /// this is the CI/development path.
  int use_fake;
  /// The calibration file actually resolved and loaded at session init
  /// (read back from the backend; may differ from calibration_file if that
  /// was left empty). Recorded for provenance: calibration drift can change
  /// which qubits are excluded, so knowing which snapshot a run used matters.
  char *resolved_calibration_file;
  /// Number of qubits reported by the backend Target.
  size_t n_qubit;
  /// The sites (qubits) of the device.
  QMIO_QDMI_Site *sites;
  /// Directed coupling map as flat pairs of sites (2*i, 2*i+1).
  QMIO_QDMI_Site *coupling_map;
  size_t coupling_map_size;
  /// Supported operations reported by the backend Target.
  QMIO_QDMI_Operation operations[MAX_NUM_OP];
  size_t n_op;
  enum QMIO_QDMI_DEVICE_SESSION_STATUS status;
} QMIO_QDMI_Device_Session_impl_t;

struct QMIO_QDMI_Device_Job_impl_d {
  QMIO_QDMI_Device_Session session;
  QDMI_Job_Status status;
  size_t *num_shots;
  QDMI_Program_Format *format;
  char *program;
  /// Transpile flag (default 1); job parameter CUSTOM1.
  int do_transpile;
  /// Qiskit transpiler optimization level (job parameter CUSTOM2).
  /// Defaults to 0, matching the value CESGA's own benchmarking pins for
  /// reproducible, comparable transpiled depth across backends.
  int optimization_level;
  /// Transpiler seed (job parameter CUSTOM3). Routing is stochastic at any
  /// optimization_level on a sparse coupling map -- leaving this unpinned
  /// has been observed to produce meaningfully different depths (e.g. 742
  /// vs 541) for the identical circuit across separate runs. Defaults to 42.
  int seed_transpiler;
  /// Results
  double *probability_dense;
  char *probability_keys;
  double *probability_values;
  int *hist_values;
  size_t n_state;
  size_t results_size;
};

/* -------------------------------------------------------------------------- */
/* Property macros (same contract as the other devices in this suite)         */
/* -------------------------------------------------------------------------- */

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

#define CHECK_PYTHON_ERROR(value, from_python)                                 \
  {                                                                            \
    if (value == Py_None || value == NULL) {                                   \
      PyErr_Print();                                                           \
      if (!from_python) {                                                      \
        PyGILState_Release(gstate);                                            \
      }                                                                        \
      QMIO_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);                    \
      return QDMI_ERROR_FATAL;                                                 \
    }                                                                          \
  }

#define CHECK_QDMI_ERROR(value)                                                \
  {                                                                            \
    if (value != QDMI_SUCCESS) {                                               \
      return value;                                                            \
    }                                                                          \
  }

/* -------------------------------------------------------------------------- */
/* Static state                                                               */
/* -------------------------------------------------------------------------- */

static QDMI_Device_Status *QMIO_QDMI_get_device_status(void) {
  static QDMI_Device_Status device_status = QDMI_DEVICE_STATUS_OFFLINE;
  return &device_status;
}
QDMI_Device_Status QMIO_QDMI_read_device_status(void) {
  return *QMIO_QDMI_get_device_status();
}
void QMIO_QDMI_set_device_status(QDMI_Device_Status status) {
  *QMIO_QDMI_get_device_status() = status;
}

static int *isFromPython(void) {
  static int from_python = 0;
  return &from_python;
}

static PyObject **get_backend(void) {
  static PyObject *backend = NULL;
  return &backend;
}

static PyObject **get_custom_python_module(void) {
  static PyObject *custom_python_module = NULL;
  return &custom_python_module;
}

/* -------------------------------------------------------------------------- */
/* Python bootstrap                                                           */
/* -------------------------------------------------------------------------- */

static int check_env_variable(void) {
  if (getenv(SCRIPT_LOCATION) == NULL || getenv(SCRIPT_NAME) == NULL)
    return QDMI_ERROR_FATAL;
  return QDMI_SUCCESS;
}

static int initialize_python(void) {
  char *script_location = getenv(SCRIPT_LOCATION);
  char *script_name = getenv(SCRIPT_NAME);

  *isFromPython() = Py_IsInitialized();
  PyGILState_STATE gstate;
  if (!*isFromPython()) {
    Py_Initialize();
    (void)PyEval_SaveThread();
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

/* -------------------------------------------------------------------------- */
/* Device lifecycle                                                           */
/* -------------------------------------------------------------------------- */

int QMIO_QDMI_device_initialize(void) {
  int err = check_env_variable();
  CHECK_QDMI_ERROR(err)
  err = initialize_python();
  CHECK_QDMI_ERROR(err)
  QMIO_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  return QDMI_SUCCESS;
}

int QMIO_QDMI_device_finalize(void) {
  QMIO_QDMI_set_device_status(QDMI_DEVICE_STATUS_OFFLINE);
  if (*get_custom_python_module() != NULL) {
    Py_DECREF(*get_custom_python_module());
    *get_custom_python_module() = NULL;
  }
  if (*get_backend() != NULL) {
    Py_DECREF(*get_backend());
    *get_backend() = NULL;
  }
  if (Py_IsInitialized() && !_Py_IsFinalizing() && !*isFromPython()) {
    (void)PyGILState_Ensure();
    Py_Finalize();
  }
  return QDMI_SUCCESS;
}

/* -------------------------------------------------------------------------- */
/* Session interface                                                          */
/* -------------------------------------------------------------------------- */

int QMIO_QDMI_device_session_alloc(QMIO_QDMI_Device_Session *session) {
  if (session == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;
  *session = (QMIO_QDMI_Device_Session)calloc(
      1, sizeof(QMIO_QDMI_Device_Session_impl_t));
  if (*session == NULL)
    return QDMI_ERROR_OUTOFMEM;
  (*session)->status = ALLOCATED;
  return QDMI_SUCCESS;
}

/**
 * @brief Set a session parameter.
 * @details QMIO has no host/URL and no session-level credentials to set:
 * authentication happens once at SSH login to the CESGA login node, before
 * the process embedding this device is ever launched (via sbatch) into the
 * qpu partition. Consequently QDMI_DEVICE_SESSION_PARAMETER_USERNAME/
 * PASSWORD/BASEURL are accepted-and-ignored rather than rejected, so a
 * generic QDMI client that always sets them doesn't fail. The QMIO-specific
 * parameters are CUSTOM1 (calibration file path) and CUSTOM2 (use FakeQmio).
 * The calibration *directory* is not a device parameter: the qmiotools
 * library discovers it from the $QMIO_CALIBRATIONS environment variable.
 */
int QMIO_QDMI_device_session_set_parameter(
    QMIO_QDMI_Device_Session session,
    const QDMI_Device_Session_Parameter param, const size_t size,
    const void *value) {
  if (session == NULL || (value != NULL && size == 0) ||
      (param >= QDMI_DEVICE_SESSION_PARAMETER_MAX &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (session->status != ALLOCATED)
    return QDMI_ERROR_BADSTATE;

  switch (param) {
  case QDMI_DEVICE_SESSION_PARAMETER_USERNAME:
  case QDMI_DEVICE_SESSION_PARAMETER_PASSWORD:
  case QDMI_DEVICE_SESSION_PARAMETER_BASEURL:
    /* Accepted and ignored -- see function docstring. */
    return QDMI_SUCCESS;
  case QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1:
    /* Explicit calibration file path. Empty/unset -> library auto-discovers
     * the newest snapshot from $QMIO_CALIBRATIONS. */
    session->calibration_file = (char *)malloc(size);
    strncpy(session->calibration_file, (const char *)value, size - 1);
    session->calibration_file[size - 1] = '\0';
    return QDMI_SUCCESS;
  case QDMI_DEVICE_SESSION_PARAMETER_CUSTOM2:
    /* Nonzero -> use FakeQmio (AerSimulator + QMIO noise model) instead of
     * the real QPU. Still needs calibration data but no qpu allocation. */
    memcpy(&session->use_fake, value, sizeof(int));
    return QDMI_SUCCESS;
  default:
    break;
  }
  return QDMI_ERROR_NOTSUPPORTED;
}

/**
 * @brief Initializes the session: creates the backend and pulls metadata.
 * @details Calls `create_backend` and `get_metadata` from the auxiliary
 * module. The qubit count, operations, and coupling map are populated
 * dynamically from the backend's Qiskit Target, so calibration updates on
 * QMIO are reflected without recompiling this device.
 */
int QMIO_QDMI_device_session_init(QMIO_QDMI_Device_Session session) {
  if (session == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  switch (QMIO_QDMI_read_device_status()) {
  case QDMI_DEVICE_STATUS_ERROR:
  case QDMI_DEVICE_STATUS_OFFLINE:
  case QDMI_DEVICE_STATUS_MAINTENANCE:
    return QDMI_ERROR_FATAL;
  default:
    break;
  }

  PyGILState_STATE gstate = PyGILState_Ensure();

  /* create_backend(calibration_file, use_fake) -> [backend, resolved_path] */
  PyObject *pFunc = PyObject_GetAttrString(*get_custom_python_module(),
                                           CREATE_BACKEND_FUNCTION_NAME);
  CHECK_PYTHON_ERROR(pFunc, *isFromPython())
  PyObject *pArgs = PyTuple_Pack(
      2,
      PyUnicode_FromString(session->calibration_file != NULL
                               ? session->calibration_file
                               : ""),
      PyLong_FromLong(session->use_fake));
  PyObject *pCreateResult = PyObject_CallObject(pFunc, pArgs);
  CHECK_PYTHON_ERROR(pCreateResult, *isFromPython())

  PyObject *pBackend = PyList_GetItem(pCreateResult, 0); /* borrowed */
  Py_INCREF(pBackend);
  *get_backend() = pBackend;

  const char *resolved =
      PyUnicode_AsUTF8(PyList_GetItem(pCreateResult, 1));
  session->resolved_calibration_file = strdup(resolved);

  /* get_metadata(backend) -> [n_qubits, "ops", "a-b;c-d"] */
  PyObject *pMeta = PyObject_GetAttrString(*get_custom_python_module(),
                                           GET_METADATA_FUNCTION_NAME);
  CHECK_PYTHON_ERROR(pMeta, *isFromPython())
  PyObject *pMetaArgs = PyTuple_Pack(1, pBackend);
  PyObject *pResult = PyObject_CallObject(pMeta, pMetaArgs);
  CHECK_PYTHON_ERROR(pResult, *isFromPython())

  session->n_qubit = (size_t)PyLong_AsLong(PyList_GetItem(pResult, 0));

  session->sites = malloc(sizeof(QMIO_QDMI_Site) * session->n_qubit);
  if (session->sites == NULL) {
    PyGILState_Release(gstate);
    return QDMI_ERROR_OUTOFMEM;
  }
  for (size_t i = 0; i < session->n_qubit; ++i) {
    session->sites[i] = malloc(sizeof(struct QMIO_QDMI_Site_impl_d));
    session->sites[i]->index = i;
  }

  /* Operations from Target.operation_names. QMIO's native set (verified
   * against qmiotools QmioBackend Target construction) is:
   *   sx, x       -- 1 qubit, 0 params
   *   rz(theta)   -- 1 qubit, 1 param
   *   ecr         -- 2 qubits, 0 params (the only native 2-qubit gate)
   *   measure     -- 1 qubit, 0 params
   *   delay(t)    -- 1 qubit, 1 param
   * FakeQmio (AerSimulator) may additionally expose synthesized gates; the
   * checks below stay general so unknown 2-qubit names still resolve. */
  const char *ops_csv =
      PyUnicode_AsUTF8(PyList_GetItem(pResult, 1));
  char *ops_copy = strdup(ops_csv);
  session->n_op = 0;
  for (char *tok = strtok(ops_copy, ","); tok != NULL && session->n_op < MAX_NUM_OP;
       tok = strtok(NULL, ",")) {
    QMIO_QDMI_Operation_impl_t *op =
        malloc(sizeof(QMIO_QDMI_Operation_impl_t));
    op->name = strdup(tok);
    op->n_qubit = (strcmp(tok, "ecr") == 0 || strcmp(tok, "cz") == 0 ||
                   strcmp(tok, "cx") == 0 || strcmp(tok, "rzx") == 0)
                      ? 2
                      : 1;
    op->n_param = (strcmp(tok, "rz") == 0 || strcmp(tok, "rx") == 0 ||
                   strcmp(tok, "ry") == 0 || strcmp(tok, "delay") == 0 ||
                   strcmp(tok, "rzx") == 0)
                      ? 1
                      : 0;
    session->operations[session->n_op++] = op;
  }
  free(ops_copy);

  /* Coupling map: "0-1;1-2;..." (directed edges) */
  const char *edges_csv = PyUnicode_AsUTF8(PyList_GetItem(pResult, 2));
  char *edges_copy = strdup(edges_csv);
  size_t max_edges = strlen(edges_csv) / 3 + 2;
  session->coupling_map = malloc(sizeof(QMIO_QDMI_Site) * max_edges * 2);
  size_t index = 0;
  for (char *tok = strtok(edges_copy, ";"); tok != NULL;
       tok = strtok(NULL, ";")) {
    int a = -1;
    int b = -1;
    if (sscanf(tok, "%d-%d", &a, &b) == 2 && a >= 0 && b >= 0 &&
        (size_t)a < session->n_qubit && (size_t)b < session->n_qubit) {
      session->coupling_map[index++] = session->sites[a];
      session->coupling_map[index++] = session->sites[b];
    }
  }
  session->coupling_map_size = index;
  free(edges_copy);

  /* get_site_properties(backend) -> ["t1,t2,freq", ...] per qubit.
   * Populates per-site T1/T2/frequency from the Target's qubit_properties
   * (QMIO loads these from the calibration file). A QDMI FOMAC uses T1/T2
   * for calibration-aware qubit mapping. Best-effort: on failure, sites
   * keep NaN placeholders and the device still works for execution. */
  for (size_t i = 0; i < session->n_qubit; ++i) {
    session->sites[i]->t1 = (double)(0.0 / 0.0);
    session->sites[i]->t2 = (double)(0.0 / 0.0);
    session->sites[i]->frequency = (double)(0.0 / 0.0);
  }
  PyObject *pSiteFunc = PyObject_GetAttrString(
      *get_custom_python_module(), GET_SITE_PROPERTIES_FUNCTION_NAME);
  if (pSiteFunc != NULL) {
    PyObject *pSiteArgs = PyTuple_Pack(1, pBackend);
    PyObject *pSiteResult = PyObject_CallObject(pSiteFunc, pSiteArgs);
    if (pSiteResult != NULL && pSiteResult != Py_None) {
      Py_ssize_t n = PyList_Size(pSiteResult);
      for (Py_ssize_t i = 0; i < n && (size_t)i < session->n_qubit; ++i) {
        const char *row = PyUnicode_AsUTF8(PyList_GetItem(pSiteResult, i));
        double t1 = 0.0;
        double t2 = 0.0;
        double freq = 0.0;
        if (row != NULL && sscanf(row, "%lf,%lf,%lf", &t1, &t2, &freq) == 3) {
          session->sites[i]->t1 = t1;
          session->sites[i]->t2 = t2;
          session->sites[i]->frequency = freq;
        }
      }
    }
    Py_XDECREF(pSiteResult);
    Py_XDECREF(pSiteArgs);
    Py_XDECREF(pSiteFunc);
  } else {
    PyErr_Clear();
  }

  Py_XDECREF(pResult);
  Py_XDECREF(pMetaArgs);
  Py_XDECREF(pCreateResult);
  Py_XDECREF(pArgs);
  PyGILState_Release(gstate);

  session->status = INITIALIZED;
  return QDMI_SUCCESS;
}

void QMIO_QDMI_device_session_free(QMIO_QDMI_Device_Session session) {
  if (session == NULL)
    return;
  if (session->sites != NULL) {
    for (size_t i = 0; i < session->n_qubit; ++i)
      free(session->sites[i]);
    free(session->sites);
  }
  free(session->coupling_map);
  for (size_t i = 0; i < session->n_op; ++i) {
    free(session->operations[i]->name);
    free(session->operations[i]);
  }
  free(session->calibration_file);
  free(session->resolved_calibration_file);
  free(session);
}

/* -------------------------------------------------------------------------- */
/* Job interface                                                              */
/* -------------------------------------------------------------------------- */

int QMIO_QDMI_device_session_create_device_job(QMIO_QDMI_Device_Session session,
                                               QMIO_QDMI_Device_Job *job) {
  if (session == NULL || job == NULL || session->status != INITIALIZED)
    return QDMI_ERROR_INVALIDARGUMENT;
  *job =
      (QMIO_QDMI_Device_Job)calloc(1, sizeof(struct QMIO_QDMI_Device_Job_impl_d));
  if (*job == NULL)
    return QDMI_ERROR_OUTOFMEM;
  (*job)->session = session;
  (*job)->status = QDMI_JOB_STATUS_CREATED;
  (*job)->do_transpile = 1;
  (*job)->optimization_level = 0; /* matches CESGA's own benchmarking default */
  (*job)->seed_transpiler = 42;
  return QDMI_SUCCESS;
}

int QMIO_QDMI_device_job_set_parameter(QMIO_QDMI_Device_Job job,
                                       const QDMI_Device_Job_Parameter param,
                                       const size_t size, const void *value) {
  if (job == NULL || value == NULL || size == 0 ||
      (param >= QDMI_DEVICE_JOB_PARAMETER_MAX &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM1 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM2 &&
       param != QDMI_DEVICE_JOB_PARAMETER_CUSTOM3)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  if (job->status != QDMI_JOB_STATUS_CREATED)
    return QDMI_ERROR_BADSTATE;

  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAMFORMAT) {
    QDMI_Program_Format format = *(const QDMI_Program_Format *)value;
    if (format != QDMI_PROGRAM_FORMAT_QASM2 &&
        format != QDMI_PROGRAM_FORMAT_QASM3)
      return QDMI_ERROR_NOTSUPPORTED;
    job->format = malloc(sizeof(QDMI_Program_Format));
    memcpy(job->format, value, sizeof(QDMI_Program_Format));
    return QDMI_SUCCESS;
  }
  if (param == QDMI_DEVICE_JOB_PARAMETER_PROGRAM) {
    job->program = malloc(size);
    strncpy(job->program, (const char *)value, size - 1);
    job->program[size - 1] = '\0';
    return QDMI_SUCCESS;
  }
  if (param == QDMI_DEVICE_JOB_PARAMETER_SHOTSNUM) {
    job->num_shots = malloc(sizeof(size_t));
    memcpy(job->num_shots, value, sizeof(size_t));
    return QDMI_SUCCESS;
  }
  if (param == QDMI_DEVICE_JOB_PARAMETER_CUSTOM1) {
    /* Nonzero (default) -> transpile against the Target before running. */
    memcpy(&job->do_transpile, value, sizeof(int));
    return QDMI_SUCCESS;
  }
  if (param == QDMI_DEVICE_JOB_PARAMETER_CUSTOM2) {
    /* Qiskit transpiler optimization_level (default 0). */
    memcpy(&job->optimization_level, value, sizeof(int));
    return QDMI_SUCCESS;
  }
  if (param == QDMI_DEVICE_JOB_PARAMETER_CUSTOM3) {
    /* Transpiler seed (default 42). Always set explicitly for reproducible
     * routing -- see the struct field docstring. */
    memcpy(&job->seed_transpiler, value, sizeof(int));
    return QDMI_SUCCESS;
  }
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_job_query_property(QMIO_QDMI_Device_Job job,
                                        const QDMI_Device_Job_Property prop,
                                        const size_t size, void *value,
                                        size_t *size_ret) {
  (void)job;
  (void)prop;
  (void)size;
  (void)value;
  (void)size_ret;
  return QDMI_ERROR_NOTIMPLEMENTED;
}

/**
 * @brief Submits a job to QMIO (blocking).
 * @details `QmioBackend.run()` is a plain synchronous Qiskit BackendV2 call:
 * it blocks until execution completes and returns a Result. This device
 * (and the process embedding it) is assumed to already be running inside a
 * Slurm allocation of the `qpu` partition -- the ZMQ round-trip to the
 * Quantum Control Node is entirely internal to qmiotools and invisible
 * here. Consequently, after this function returns successfully, the job
 * status is QDMI_JOB_STATUS_DONE (same execution model as the Qaptiva
 * device).
 */
int QMIO_QDMI_device_job_submit(QMIO_QDMI_Device_Job job) {
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;
  if (job->status != QDMI_JOB_STATUS_CREATED || job->program == NULL ||
      job->num_shots == NULL || job->format == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;

  job->status = QDMI_JOB_STATUS_RUNNING;
  QMIO_QDMI_set_device_status(QDMI_DEVICE_STATUS_BUSY);

  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject *pFunc = PyObject_GetAttrString(*get_custom_python_module(),
                                           SUBMIT_JOB_FUNCTION_NAME);
  CHECK_PYTHON_ERROR(pFunc, *isFromPython())

  long fmt = (*(job->format) == QDMI_PROGRAM_FORMAT_QASM3) ? 3 : 2;
  PyObject *pArgs =
      PyTuple_Pack(7, *get_backend(), PyUnicode_FromString(job->program),
                   PyLong_FromLong(fmt),
                   PyLong_FromSize_t(*(job->num_shots)),
                   PyLong_FromLong(job->do_transpile),
                   PyLong_FromLong(job->optimization_level),
                   PyLong_FromLong(job->seed_transpiler));
  CHECK_PYTHON_ERROR(pArgs, *isFromPython())

  PyObject *pResult = PyObject_CallObject(pFunc, pArgs);
  if (pResult == NULL || pResult == Py_None) {
    PyErr_Print();
    PyGILState_Release(gstate);
    job->status = QDMI_JOB_STATUS_FAILED;
    QMIO_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
    return QDMI_SUCCESS; /* failure is reported via job status */
  }

  /* Result layout: ["s1,s2,...", p1, p2, ...] */
  Py_ssize_t list_size = PyList_Size(pResult);
  size_t n_results = (size_t)list_size - 1;

  const char *keys = PyUnicode_AsUTF8(PyList_GetItem(pResult, 0));
  job->probability_keys = strdup(keys);

  job->probability_values = malloc(sizeof(double) * n_results);
  job->hist_values = malloc(sizeof(int) * n_results);
  job->results_size = n_results;

  /* Dense vector sized by the first bitstring's length. */
  char *first_key = strdup(keys);
  char *comma = strchr(first_key, ',');
  if (comma != NULL)
    *comma = '\0';
  job->n_state = (size_t)1 << strlen(first_key);
  job->probability_dense = calloc(job->n_state, sizeof(double));

  char *keys_copy = strdup(keys);
  char *tok = strtok(keys_copy, ",");
  for (size_t i = 0; i < n_results; ++i) {
    double prob = PyFloat_AsDouble(PyList_GetItem(pResult, (Py_ssize_t)i + 1));
    job->probability_values[i] = prob;
    job->hist_values[i] =
        (int)(prob * (double)(*(job->num_shots)) + 0.5);
    if (tok != NULL) {
      long dense_index = strtol(tok, NULL, 2);
      if (dense_index >= 0 && (size_t)dense_index < job->n_state)
        job->probability_dense[dense_index] = prob;
      tok = strtok(NULL, ",");
    }
  }
  free(keys_copy);
  free(first_key);

  Py_XDECREF(pResult);
  Py_XDECREF(pArgs);
  PyGILState_Release(gstate);

  job->status = QDMI_JOB_STATUS_DONE;
  QMIO_QDMI_set_device_status(QDMI_DEVICE_STATUS_IDLE);
  return QDMI_SUCCESS;
}

int QMIO_QDMI_device_job_cancel(QMIO_QDMI_Device_Job job) {
  (void)job;
  /* Submission is blocking; there is no window in which to cancel. */
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_job_check(QMIO_QDMI_Device_Job job,
                               QDMI_Job_Status *status) {
  if (job == NULL || status == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;
  *status = job->status;
  return QDMI_SUCCESS;
}

int QMIO_QDMI_device_job_wait(QMIO_QDMI_Device_Job job, size_t timeout) {
  (void)timeout;
  if (job == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;
  /* Submission is blocking; by the time wait can be called the job is in a
   * terminal state. */
  if (job->status == QDMI_JOB_STATUS_DONE ||
      job->status == QDMI_JOB_STATUS_FAILED)
    return QDMI_SUCCESS;
  return QDMI_ERROR_BADSTATE;
}

int QMIO_QDMI_device_job_get_results(QMIO_QDMI_Device_Job job,
                                     QDMI_Job_Result result, size_t size,
                                     void *data, size_t *size_ret) {
  if (job == NULL || job->status != QDMI_JOB_STATUS_DONE)
    return QDMI_ERROR_INVALIDARGUMENT;

  ADD_STRING_PROPERTY(QDMI_JOB_RESULT_HIST_KEYS, job->probability_keys, result,
                      size, data, size_ret)
  ADD_LIST_PROPERTY(QDMI_JOB_RESULT_HIST_VALUES, int, job->hist_values,
                    job->results_size, result, size, data, size_ret)
  ADD_STRING_PROPERTY(QDMI_JOB_RESULT_PROBABILITIES_SPARSE_KEYS,
                      job->probability_keys, result, size, data, size_ret)
  ADD_LIST_PROPERTY(QDMI_JOB_RESULT_PROBABILITIES_SPARSE_VALUES, double,
                    job->probability_values, job->results_size, result, size,
                    data, size_ret)
  ADD_LIST_PROPERTY(QDMI_JOB_RESULT_PROBABILITIES_DENSE, double,
                    job->probability_dense, job->n_state, result, size, data,
                    size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

void QMIO_QDMI_device_job_free(QMIO_QDMI_Device_Job job) {
  if (job == NULL)
    return;
  free(job->program);
  free(job->format);
  free(job->num_shots);
  free(job->probability_keys);
  free(job->probability_values);
  free(job->probability_dense);
  free(job->hist_values);
  free(job);
}

/* -------------------------------------------------------------------------- */
/* Query interface                                                            */
/* -------------------------------------------------------------------------- */

int QMIO_QDMI_device_session_query_device_property(
    QMIO_QDMI_Device_Session session, const QDMI_Device_Property prop,
    const size_t size, void *value, size_t *size_ret) {
  if ((prop >= QDMI_DEVICE_PROPERTY_MAX &&
       prop != QDMI_DEVICE_PROPERTY_CUSTOM1) ||
      (value == NULL && size_ret == NULL)) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }

  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_NAME, "CESGA QMIO", prop, size,
                      value, size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_VERSION, "0.1.0", prop, size, value,
                      size_ret)
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_LIBRARYVERSION, "1.1.0", prop, size,
                      value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_STATUS, QDMI_Device_Status,
                            QMIO_QDMI_read_device_status(), prop, size, value,
                            size_ret)
  /* Duration reporting convention for T1/T2 (and any other durations): raw
   * values are in nanoseconds, so the unit is "ns" and the scale factor is
   * 1.0. A client reads physical_duration = raw * 1.0 [ns]. */
  ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_DURATIONUNIT, "ns", prop, size,
                      value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_DURATIONSCALEFACTOR, double,
                            1.0, prop, size, value, size_ret)

  if (session == NULL || session->status != INITIALIZED)
    return QDMI_ERROR_INVALIDARGUMENT;

  /* Provenance: which calibration snapshot was actually loaded. Matters
   * because calibration drift can change which qubits are excluded,
   * shifting routing distance and depth between runs. */
  if (prop == QDMI_DEVICE_PROPERTY_CUSTOM1) {
    const char *resolved = session->resolved_calibration_file != NULL
                               ? session->resolved_calibration_file
                               : "";
    ADD_STRING_PROPERTY(QDMI_DEVICE_PROPERTY_CUSTOM1, resolved, prop, size,
                        value, size_ret)
  }

  ADD_SINGLE_VALUE_PROPERTY(QDMI_DEVICE_PROPERTY_QUBITSNUM, size_t,
                            session->n_qubit, prop, size, value, size_ret)
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_SITES, QMIO_QDMI_Site, session->sites,
                    session->n_qubit, prop, size, value, size_ret)
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_COUPLINGMAP, QMIO_QDMI_Site,
                    session->coupling_map, session->coupling_map_size, prop,
                    size, value, size_ret)
  ADD_LIST_PROPERTY(QDMI_DEVICE_PROPERTY_OPERATIONS, QMIO_QDMI_Operation,
                    session->operations, session->n_op, prop, size, value,
                    size_ret)

  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_session_query_site_property(
    QMIO_QDMI_Device_Session session, QMIO_QDMI_Site site,
    const QDMI_Site_Property prop, const size_t size, void *value,
    size_t *size_ret) {
  if (session == NULL || site == NULL || (value != NULL && size == 0) ||
      prop >= QDMI_SITE_PROPERTY_MAX) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_INDEX, size_t, site->index,
                            prop, size, value, size_ret)

  /* T1/T2 come from the QMIO calibration file (in seconds) via the backend
   * Target. QDMI expects raw uint64 values to be scaled by
   * QDMI_DEVICE_PROPERTY_DURATIONSCALEFACTOR into
   * QDMI_DEVICE_PROPERTY_DURATIONUNIT. We report raw values already in
   * nanoseconds (unit "ns", scale factor 1.0 -- see query_device_property),
   * so a NaN (excluded qubit) maps to raw 0. */
  if (prop == QDMI_SITE_PROPERTY_T1) {
    uint64_t t1_ns =
        (site->t1 == site->t1) ? (uint64_t)(site->t1 * 1e9 + 0.5) : 0;
    ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_T1, uint64_t, t1_ns, prop,
                              size, value, size_ret)
  }
  if (prop == QDMI_SITE_PROPERTY_T2) {
    uint64_t t2_ns =
        (site->t2 == site->t2) ? (uint64_t)(site->t2 * 1e9 + 0.5) : 0;
    ADD_SINGLE_VALUE_PROPERTY(QDMI_SITE_PROPERTY_T2, uint64_t, t2_ns, prop,
                              size, value, size_ret)
  }
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_session_query_operation_property(
    QMIO_QDMI_Device_Session session, QMIO_QDMI_Operation operation,
    const size_t num_sites, const QMIO_QDMI_Site *sites,
    const size_t num_params, const double *params,
    const QDMI_Operation_Property prop, const size_t size, void *value,
    size_t *size_ret) {
  if (session == NULL || operation == NULL ||
      (sites != NULL && num_sites == 0) ||
      (params != NULL && num_params == 0) || (value != NULL && size == 0) ||
      prop >= QDMI_OPERATION_PROPERTY_MAX) {
    return QDMI_ERROR_INVALIDARGUMENT;
  }
  ADD_STRING_PROPERTY(QDMI_OPERATION_PROPERTY_NAME, operation->name, prop,
                      size, value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_QUBITSNUM, size_t,
                            operation->n_qubit, prop, size, value, size_ret)
  ADD_SINGLE_VALUE_PROPERTY(QDMI_OPERATION_PROPERTY_PARAMETERSNUM, size_t,
                            operation->n_param, prop, size, value, size_ret)
  if (operation->n_qubit == 2) {
    ADD_LIST_PROPERTY(QDMI_OPERATION_PROPERTY_SITES, QMIO_QDMI_Site,
                      session->coupling_map, session->coupling_map_size, prop,
                      size, value, size_ret)
  } else {
    ADD_LIST_PROPERTY(QDMI_OPERATION_PROPERTY_SITES, QMIO_QDMI_Site,
                      session->sites, session->n_qubit, prop, size, value,
                      size_ret)
  }
  return QDMI_ERROR_NOTSUPPORTED;
}

/* -------------------------------------------------------------------------- */
/* Telemetry interface (not supported by this device)                         */
/* -------------------------------------------------------------------------- */

int QMIO_QDMI_device_session_query_telemetrysensor_property(
    QMIO_QDMI_Device_Session session, QMIO_QDMI_TelemetrySensor sensor,
    const QDMI_TelemetrySensor_Property prop, const size_t size, void *value,
    size_t *size_ret) {
  (void)session;
  (void)sensor;
  (void)prop;
  (void)size;
  (void)value;
  (void)size_ret;
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_session_create_device_telemetrysensor_query(
    QMIO_QDMI_Device_Session session,
    QMIO_QDMI_Device_TelemetrySensor_Query *query) {
  (void)session;
  (void)query;
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_telemetrysensor_query_set_parameter(
    QMIO_QDMI_Device_TelemetrySensor_Query query,
    const QDMI_Device_TelemetrySensor_Query_Parameter param, const size_t size,
    const void *value) {
  (void)query;
  (void)param;
  (void)size;
  (void)value;
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_telemetrysensor_query_submit(
    QMIO_QDMI_Device_TelemetrySensor_Query query) {
  (void)query;
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_telemetrysensor_query_cancel(
    QMIO_QDMI_Device_TelemetrySensor_Query query) {
  (void)query;
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_telemetrysensor_query_check_status(
    QMIO_QDMI_Device_TelemetrySensor_Query query,
    QDMI_TelemetrySensor_Query_Status *status) {
  (void)query;
  (void)status;
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_telemetrysensor_query_wait(
    QMIO_QDMI_Device_TelemetrySensor_Query query, size_t timeout) {
  (void)query;
  (void)timeout;
  return QDMI_ERROR_NOTSUPPORTED;
}

int QMIO_QDMI_device_telemetrysensor_query_get_results(
    QMIO_QDMI_Device_TelemetrySensor_Query query,
    QDMI_TelemetrySensor_Query_Result result, const size_t size, void *data,
    size_t *size_ret) {
  (void)query;
  (void)result;
  (void)size;
  (void)data;
  (void)size_ret;
  return QDMI_ERROR_NOTSUPPORTED;
}

void QMIO_QDMI_device_telemetrysensor_query_free(
    QMIO_QDMI_Device_TelemetrySensor_Query query) {
  (void)query;
}
