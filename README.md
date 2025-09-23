<!----------------------------------------------------------------------------
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
-------------------------------------------------------------------------- -->

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/Munich-Quantum-Software-Stack/QDMI/develop/docs/_static/mqss_logo_dark.svg" width="20%">
    <img src="https://raw.githubusercontent.com/Munich-Quantum-Software-Stack/QDMI/develop/docs/_static/mqss_logo.svg" width="20%">
  </picture>
</p>

# MQSS QDMI Devices Suite

<p align="center">
  <a href="https://munich-quantum-software-stack.github.io/QDMI-Devices/">
  <img style="min-width: 200px !important; width: 30%;" src="https://img.shields.io/badge/documentation-blue?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA0NDggNTEyIj48IS0tIUZvbnQgQXdlc29tZSBGcmVlIDYuNi4wIGJ5IEBmb250YXdlc29tZSAtIGh0dHBzOi8vZm9udGF3ZXNvbWUuY29tIExpY2Vuc2UgLSBodHRwczovL2ZvbnRhd2Vzb21lLmNvbS9saWNlbnNlL2ZyZWUgQ29weXJpZ2h0IDIwMjQgRm9udGljb25zLCBJbmMuLS0+PHBhdGggZmlsbD0iI2ZmZmZmZiIgZD0iTTk2IDBDNDMgMCAwIDQzIDAgOTZMMCA0MTZjMCA1MyA0MyA5NiA5NiA5NmwyODggMCAzMiAwYzE3LjcgMCAzMi0xNC4zIDMyLTMycy0xNC4zLTMyLTMyLTMybDAtNjRjMTcuNyAwIDMyLTE0LjMgMzItMzJsMC0zMjBjMC0xNy43LTE0LjMtMzItMzItMzJMMzg0IDAgOTYgMHptMCAzODRsMjU2IDAgMCA2NEw5NiA0NDhjLTE3LjcgMC0zMi0xNC4zLTMyLTMyczE0LjMtMzIgMzItMzJ6bTMyLTI0MGMwLTguOCA3LjItMTYgMTYtMTZsMTkyIDBjOC44IDAgMTYgNy4yIDE2IDE2cy03LjIgMTYtMTYgMTZsLTE5MiAwYy04LjggMC0xNi03LjItMTYtMTZ6bTE2IDQ4bDE5MiAwYzguOCAwIDE2IDcuMiAxNiAxNnMtNy4yIDE2LTE2IDE2bC0xOTIgMGMtOC44IDAtMTYtNy4yLTE2LTE2czcuMi0xNiAxNi0xNnoiLz48L3N2Zz4=" alt="Documentation" />
  </a>
</p>
<!-- [DOXYGEN MAIN] -->

The MQSS **QDMI Devices Suite** is a collection of QDMI
implementations for various quantum devices. The QDMI Devices are integrated into the Munich Quantum
Software Stack (MQSS) infrastructure to execute quantum programs and query device information to
optimize, transform, and lower the quantum programs. These devices vary from quantum emulators to
neutral atom quantum computers. The implementations serve as a hardware abstraction layer, enabling
QDMI clients to interact with the devices through QDMI drivers.

<!-- [DOXYGEN MAIN] -->

## FAQ

<!-- [DOXYGEN FAQ] -->

### What is MQSS?

**MQSS** stands for _Munich Quantum Software Stack_ and is a project of the _Munich Quantum Valley
(MQV)_ initiative. It is jointly developed by the _Leibniz Supercomputing Centre (LRZ)_, the _Chair
for Design Automation (CDA)_, and the _Chair of Computer Architecture and Parallel Systems (CAPS)_
at TUM. It provides a comprehensive compilation and runtime infrastructure for on-premise and remote
quantum devices, support for modern compilation and optimization techniques, and enables both
current and future high-level abstractions for quantum programming. This stack is designed to be
capable of deployment in a variety of scenarios via flexible configuration options. This includes
stand-alone scenarios for individual systems, cloud access to a variety of devices, as well as tight
integration into HPC environments supporting quantum acceleration. Within the MQV, a concrete
instance of the MQSS is deployed at the LRZ, serving as a single access point to all of its quantum
devices via multiple compatible access paths. This includes a web portal, command line access via
web credentials, as well as the option for hybrid access with tight integration with LRZ's HPC
systems. It facilitates the connection between end-users and quantum computing platforms by its
integration within HPC infrastructures, such as those found at the LRZ.

### What is QDMI?

**QDMI**, or _Quantum Device Management Interface_, serves as the communication interface between
software within the MQSS and the quantum hardware connected to the MQSS. The aim is to provide a
standard way to communicate with quantum resources that can be widely used by the quantum community.

### What is a QDMI Device?

**QDMI Device**, or _QDMI Backend_, is a QDMI implementation of a quantum device that enables QDMI
clients to interact with hardware though QDMI drivers. The hardware can vary from quantum emulators
to neutral atom quantum computers.

### Under which license is QDMI Devices released?

QDMI Devices is released under the Apache License v2.0 with LLVM Exceptions. See LICENSE for more
information. Any contribution to the project is assumed to be under the same license.

<!-- [DOXYGEN FAQ] -->

## 📬 Contact

The development of this project is led by QCT Department of the LRZ, a part of the Munich Quantum
Valley. You can also always reach us at
[mqss@munich-quantum-valley.de](mailto:mqss@munich-quantum-valley.de).

Please try to use the publicly accessible GitHub channels
([issues](https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/issues),
[discussions](https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/discussions),
[pull requests](https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/pulls)) to allow for a
transparent and open discussion as much as possible.
