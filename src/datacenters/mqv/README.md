<!----------------------------------------------------------------------------
Copyright (c) 2026 Munich Quantum Valley

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://llvm.org/LICENSE.txt

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-------------------------------------------------------------------------- -->

# MQV QDMI Device

A QDMI 1.4 device for MQV's MQSS MQP-API. It submits OpenQASM jobs to one MQP-API resource (e.g.
`MAQCS`, `Q20`) and reports that resource's qubits, native gates, coupling map and status.

The device is based on the LRZ device in this repository, ported to QDMI 1.4. It is a separate
module because the LRZ device implements QDMI v1.3.1, and MQT Core 4.1 (used by its built-in QDMI
driver, e.g. in openQSE/QFw) only loads QDMI 1.4 devices. The QDMI symbol prefix is `MQV`.

## Build

```console
cmake -S . -B build -DBUILD_MODULE=mqv -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Dependencies: libcurl, rabbitmq-c (`librabbitmq`), Boost headers and nlohmann/json (needed by
MQSS-Client). MQSS-Client is fetched at a pinned revision (`MQV_MQSS_CLIENT_REV`) and linked
statically into `libmqv_device`; QDMI v1.4.0 is fetched as well. At runtime the device needs libcurl
and `librabbitmq`.

On Rocky Linux 10:
`dnf install gcc-c++ cmake git libcurl-devel openssl-devel boost-devel json-devel librabbitmq-devel`
(`librabbitmq-devel` and `json-devel` come from EPEL/CRB). On macOS (Homebrew):
`brew install curl rabbitmq-c boost nlohmann-json`.

CMake options:

- `MQV_QDMI_INSTALL` (OFF): install the library and its manifest.
- `MQV_QDMI_BUNDLE_RABBITMQ` (OFF): also install `librabbitmq` next to the library (RPATH
  `$ORIGIN`), for systems without rabbitmq-c.
- `MQV_QDMI_DEVICE_ID` (`mqv.default`), `MQV_QDMI_MANIFEST_RESOURCES`, `MQV_QDMI_MANIFEST_BASEURL`:
  see [Manifest](#manifest-and-device-ids).

## Tests

Testing is enabled in `tests/`, so run ctest from the test build directory:

```console
ctest --test-dir build/tests/datacenters/mqv            # unit tests; E2E tests skip
tests/datacenters/mqv/mock_mqp/run_e2e.sh build/tests/datacenters/mqv
```

The unit tests include the QDMI conformance tests. The end-to-end tests skip unless
`MQV_QDMI_E2E_BASEURL` is set. `run_e2e.sh` starts a local mock MQP-API (`mock_mqp/mqp_mock.py`,
needs [uv](https://docs.astral.sh/uv/) and `curl`) and runs the end-to-end tests five times: a
standard resource; the same with `GET resources/<name>` denied; a resource with connectivity `None`;
a MAQCS-style resource with `(a, b)` connectivity; and the same answering as MAQCS does today
(results as `[{"counts": ...}]`, jobs with `no_modify=false` cancelled). The mock's token is a fixed
test value, not a credential.

## Manifest and device IDs

The build writes `mqv-qdmi-device.qdmi.json` next to the library, in the format MQT Core's built-in
QDMI driver reads. The driver doesn't search next to device libraries: register the manifest with
`MQT_CORE_QDMI_CONFIG_FILE=<path>/mqv-qdmi-device.qdmi.json` (this replaces MQT Core's system, user
and project `qdmi.json` files, see MQT Core's QDMI configuration docs). Keep the manifest next to
the library: its `library` entry is relative to the manifest's directory.

| Device ID                                                                                                | Session set by the manifest   |
| -------------------------------------------------------------------------------------------------------- | ----------------------------- |
| `mqv.default`                                                                                            | none                          |
| `mqv.aqt20`, `mqv.maqcs`, `mqv.muniqc-atoms20`, `mqv.q20`, `mqv.q5`, `mqv.qexa20`, `mqv.qlm`, `mqv.wmi3` | `custom1` = the resource name |

The per-resource IDs are `mqv.<lowercase resource>`, from the CMake list
`MQV_QDMI_MANIFEST_RESOURCES` (default `AQT20;MAQCS;MUNIQC-Atoms20;Q20;Q5;QExa20;QLM;WMI3`). The
MQP-API host is site-specific, so the manifest has no base URL. A site build can add one with
`MQV_QDMI_MANIFEST_BASEURL`. The token never goes into the manifest.

```python
from mqt.core.qdmi import builtin_driver

device = builtin_driver.open_device(
    "mqv.maqcs", base_url="https://<mqp-api-host>/v1/", token="<MQP-API token>"
)
```

## Session parameters

Each parameter falls back to an environment variable when the client does not set it. Session
initialization does not contact the MQP-API.

| Session parameter | Meaning                                        | Environment fallback | Default |
| ----------------- | ---------------------------------------------- | -------------------- | ------- |
| `BASEURL`         | MQP-API base URL, `https://<mqp-api-host>/v1/` | `MQV_QDMI_BASEURL`   | none    |
| `TOKEN`           | MQP-API token, sent as Bearer token            | `MQV_QDMI_TOKEN`     | none    |
| `CUSTOM1`         | MQP-API resource name, e.g. `MAQCS`            | `MQV_QDMI_RESOURCE`  | none    |
| `CUSTOM2`         | job field `no_modify`, `true` or `false`       | `MQV_QDMI_NO_MODIFY` | `true`  |

Without a resource the session describes an empty device (0 qubits). If `CURL_CA_BUNDLE` is unset,
the device sets it to the system CA bundle (`/etc/pki/tls/certs/ca-bundle.crt`,
`/etc/ssl/certs/ca-certificates.crt` or `/etc/ssl/cert.pem`), because MQSS-Client otherwise disables
libcurl's default bundle for HTTPS.

## Programs and results

- A job carries exactly one program, `QDMI_PROGRAM_FORMAT_QASM2` or `QDMI_PROGRAM_FORMAT_QASM3`. The
  default shot count is 1024.
- MQSS currently routes OpenQASM only with `no_modify=true`, hence the default. The program must
  already use the resource's native gates and coupling map.
- MAQCS needs OpenQASM 3 with a declared register (`qubit[n] q;`). Physical qubits (`$0`, `$1`,
  ...), as Qiskit writes a transpiled circuit, fail.
- The device reads results with its own `GET job/<id>/result`, not through MQSS-Client. It accepts a
  histogram `{"00": n, ...}`, a list of them, or either wrapped as `{"counts": {...}}`, e.g.
  `[{"counts": {...}}]` as the MQP-API workers return it. Results are histograms only.

## Differences from the LRZ device

- QDMI 1.4 instead of v1.3.1, so MQT Core 4.1 can load it.
- No built-in base URL or resource: the LRZ device defaults to the LRZ portal and `EQE1`
  (`LRZ_QDMI_DEVICE_BASEURL`, `LRZ_QDMI_DEVICE_QPU`); this device takes them from the session or the
  `MQV_QDMI_*` variables.
- Adds `CUSTOM2` for the MQP-API job field `no_modify` (default `true`).
- Loads the resource on first use, not at session initialization.
- Ships a manifest for MQT Core's driver, with one device ID per resource.
- Reads job results itself, to accept the `[{"counts": ...}]` shape.
- Pins MQSS-Client and links it statically; no C++ exception crosses the C interface.
