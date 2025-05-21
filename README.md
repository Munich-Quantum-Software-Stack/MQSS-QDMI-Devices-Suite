# QDMI Devices

This repository contains various QDMI device implementations for the different quantum devices. It
serves as a hardware abstraction layer that enables QDMI clients to communicate with the devices'
systems through a standardized interface.

## 🚀 Overview

The **Quantum Device Management Interface (QDMI)** is one of the core components of the Munich
Quantum Software Stack (MQSS)—a sophisticated software stack to connect end users to a wide range of
possible quantum devices. It enables the submission to and control of gate-based quantum systems and
enables software tools to automatically retrieve and adapt to changing physical characteristics and
constraints of different platforms.

The QDMI Devices provides device-specific logic for:

- Querying supported device properties (e.g., number of qubits, coupling map, environmental
  variables).
- Querying the environmental variables' data
- Creating and executing quantum jobs on the devices.

## ⚙️ Build Instructions

The devices are built using CMake as its build system.

The following commands configure the project.

```bash
cmake -S . -B build
```

After the build system is generated in the `build` directory, the devices can be built by calling.

```bash
cmake --build build
```

After the build process is complete, the device libraries can be found in the
`build\name-of-the-device` directory.

## 📄 License

QDMI is released under the Apache License v2.0 with LLVM Exceptions. See LICENSE for more
information. Any contribution to the project is assumed to be under the same license.

## 📬 Contact

The development of this project is led by
[Ercüment Kaya (LRZ/TUM CAPS)](mailto:ercuement.kaya@lrz.de) and
[Muhammad Nufail Farooqi (LRZ)](mailto:Muhammad.Farooqi@lrz.de).

Please try to use the publicly accessible GitHub channels
([issues](https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/issues),
[discussions](https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/discussions),
[pull requests](https://github.com/Munich-Quantum-Software-Stack/QDMI-Devices/pulls)) to allow for a
transparent and open discussion as much as possible.
