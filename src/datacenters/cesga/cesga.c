/*------------------------------------------------------------------------------
Copyright 2025 Munich Quantum Software Stack Project / CESGA

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
------------------------------------------------------------------------------*/

/** @file
 * @brief SKELETON of the CESGA datacenter QDMI Device.
 *
 * @details Role (mirroring src/datacenters/lrz/lrz.c): a *single* QDMI device
 * that fronts all quantum resources of the datacenter. The LRZ device is a
 * thin REST client of the Munich Quantum Portal: the session parameter
 * CUSTOM1 selects the hardware by name ("AQT20", "QLM", ...), session init
 * fetches qubits/connectivity/instructions from GET /resources/<hw>, jobs are
 * POSTed to /job and polled at /job/<uuid>.
 *
 * OPEN DESIGN DECISION for CESGA — pick one before implementing:
 *
 *  (A) REST portal client (LRZ-style). Requires a CESGA-side service exposing
 *      an MQP-compatible (or similar) REST API in front of QMIO/QLM/CUNQA.
 *      If CESGA deploys such a portal, this file is ~90% a copy of lrz.c with
 *      different DEVICE_HARDWARES, base URL, and auth (user+password ->
 *      QDMI_DEVICE_SESSION_PARAMETER_USERNAME/PASSWORD, exchanged for a
 *      session token at QDMI_DEVICE_SESSION_PARAMETER_AUTHURL).
 *
 *  (B) Local dispatcher. No new service: this device dlopen()s the concrete
 *      CESGA device libraries (libqmio_device.so, libqaptiva_device.so) and
 *      forwards every call to the one selected via CUSTOM1. This duplicates
 *      part of what the driver already does — only worth it if MQSS expects
 *      the datacenter to appear as exactly one QDMI device.
 *
 * The skeleton below compiles and returns NOTIMPLEMENTED everywhere except
 * the session-parameter plumbing, which already accepts username/password and
 * hardware selection so the interface contract is fixed for MQSS.
 */

#include <cesga_qdmi/device.h>
#include <cesga_qdmi/types.h>
#include <qdmi/constants.h>

#include <stdlib.h>
#include <string.h>

/// Quantum resources available at CESGA.
#define NUM_OF_HW 3
static const char *DEVICE_HARDWARES[NUM_OF_HW] = {"QMIO", "QLM", "CUNQA"};

enum CESGA_QDMI_DEVICE_SESSION_STATUS { ALLOCATED, INITIALIZED };

typedef struct CESGA_QDMI_Device_Session_impl_d {
  char *base_url;
  char *username;
  char *password;
  char *hardware_name; /* CUSTOM1 */
  enum CESGA_QDMI_DEVICE_SESSION_STATUS status;
} CESGA_QDMI_Device_Session_impl_t;

int CESGA_QDMI_device_initialize(void) { return QDMI_SUCCESS; }
int CESGA_QDMI_device_finalize(void) { return QDMI_SUCCESS; }

int CESGA_QDMI_device_session_alloc(CESGA_QDMI_Device_Session *session) {
  if (session == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;
  *session = (CESGA_QDMI_Device_Session)calloc(
      1, sizeof(CESGA_QDMI_Device_Session_impl_t));
  if (*session == NULL)
    return QDMI_ERROR_OUTOFMEM;
  (*session)->status = ALLOCATED;
  return QDMI_SUCCESS;
}

#define COPY_STRING_PARAM(field)                                               \
  {                                                                            \
    (field) = (char *)malloc(size);                                            \
    strncpy((field), (const char *)value, size - 1);                           \
    (field)[size - 1] = '\0';                                                  \
    return QDMI_SUCCESS;                                                       \
  }

int CESGA_QDMI_device_session_set_parameter(
    CESGA_QDMI_Device_Session session,
    const QDMI_Device_Session_Parameter param, const size_t size,
    const void *value) {
  if (session == NULL || value == NULL || size == 0)
    return QDMI_ERROR_INVALIDARGUMENT;
  if (session->status != ALLOCATED)
    return QDMI_ERROR_BADSTATE;

  switch (param) {
  case QDMI_DEVICE_SESSION_PARAMETER_BASEURL:
    COPY_STRING_PARAM(session->base_url)
  case QDMI_DEVICE_SESSION_PARAMETER_USERNAME:
    COPY_STRING_PARAM(session->username)
  case QDMI_DEVICE_SESSION_PARAMETER_PASSWORD:
    COPY_STRING_PARAM(session->password)
  case QDMI_DEVICE_SESSION_PARAMETER_CUSTOM1: /* hardware selection */
    COPY_STRING_PARAM(session->hardware_name)
  default:
    break;
  }
  return QDMI_ERROR_NOTSUPPORTED;
}

int CESGA_QDMI_device_session_init(CESGA_QDMI_Device_Session session) {
  if (session == NULL)
    return QDMI_ERROR_INVALIDARGUMENT;
  if (session->hardware_name != NULL) {
    int known = 0;
    for (int i = 0; i < NUM_OF_HW; ++i)
      if (strcmp(session->hardware_name, DEVICE_HARDWARES[i]) == 0)
        known = 1;
    if (!known)
      return QDMI_ERROR_INVALIDARGUMENT;
  }
  /* TODO (option A): authenticate (username/password -> token) and fetch
   * resource metadata from the CESGA portal.
   * TODO (option B): dlopen the selected device library and forward. */
  return QDMI_ERROR_NOTIMPLEMENTED;
}

void CESGA_QDMI_device_session_free(CESGA_QDMI_Device_Session session) {
  if (session == NULL)
    return;
  free(session->base_url);
  free(session->username);
  free(session->password);
  free(session->hardware_name);
  free(session);
}

/* All remaining interface functions: NOTIMPLEMENTED stubs. Copy the bodies
 * from lrz.c (option A) or forward via function pointers (option B). */
