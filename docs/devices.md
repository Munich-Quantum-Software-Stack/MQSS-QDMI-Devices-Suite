# Devices

<!-- IMPORTANT: Keep the line above as the first line. -->

<!----------------------------------------------------------------------------
Copyright 2024 Munich Quantum Software Stack Project

Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
"License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

https://github.com/Munich-Quantum-Software-Stack/QDMI/blob/develop/LICENSE

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.

SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-------------------------------------------------------------------------- -->

<!-- This file is a static page and included in the ./CMakeLists.txt file. -->

This page contains the implementations of the QDMI devices.

\tableofcontents

## Implemented Devices {#device}

Currently, there are two QDMI device implementations: Qaptiva and DCDB. While Qaptiva is implemented using C with Python components, DCDB is implemented using C++.

Qaptiva is a quantum computing emulator that has 38 qubits. On the other hand, DCDB is a database of the LRZ that monitors HPC environments.

While Qaptiva is capable of executing quantum circuits, DCDB allows the QDMI Client to query the surroundings of quantum and HPC devices located at LRZ.

### The Properties of Devices {#device-properties}

Every implemented devices has different proterties, i.e. `QDMI_DEVICE_PROPERTY_NAME`, the number of `QDMI_Site` or `QDMI_Environment`.

The properties might be of composite type such as `QDMI_Environment` or `QDMI_Site` or primitive type such as `int`, `char*` (string).
i.e `char*` (string), `int`, `QDMI_Site`.

The `QDMI_Client` can query the device to get the proterties using `QDMI_device_query_device_property`.
If the device does not the property it would return `QDMI_ERROR_NOTSUPPORTED`. Below you can find the respective implementation in DCDB and Qaptiva.

<!-- prettier-ignore-start -->
<div class="tabbed">
- <b class="tab-title">QLM</b>
  \dontinclude qlm.c
  \skip int QLM_QDMI_device_session_query_device_property
  \until QDMI_DEVICE_PROPERTY_LIBRARYVERSION
  \until size_ret)
  \until QDMI_ERROR_NOTSUPPORTED
- <b class="tab-title">DCDB</b>
  \dontinclude dcdb.cpp
  \skip int DCDB_QDMI_device_session_query_device_property
  \until QDMI_DEVICE_PROPERTY_LIBRARYVERSION
  \until size_ret)
  \until QDMI_ERROR_NOTSUPPORTED
</div>
<!-- prettier-ignore-end -->

Both implementations use an auxiliary macro to add the string properties to the device. For an
explanation of the macros, see the QDMI Documentation on <a href="https://munich-quantum-software-stack.github.io/QDMI/md_docs_2examples.html#device-macros">Auxiliary Macros</a>.


### The Sites of the devices {#device-sites}

TODO

### The Environment of the devices {#device-environment}

TODO

### Submiting a job {#device-job}

TODO

### Submiting an environment query {#device-environment-query}

TODO
