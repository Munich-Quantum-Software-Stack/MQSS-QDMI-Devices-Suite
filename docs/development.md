# Development Guide

<!-- IMPORTANT: Keep the line above as the first line. -->
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

<!-- This file is a static page and included in the CMakeLists.txt file. -->

Ready to contribute to QDMI Devices or create your own? This guide will help you get started.

## Initial Setup

1. Fork the [QDMI Devices](https://github.com/Munich-Quantum-Software-Stack/QDMI_devices) repository
   on GitHub (see <https://docs.github.com/en/get-started/quickstart/fork-a-repo>).

2. Clone your fork locally

   ```sh
   git clone git@github.com:your_name_here/QDMI-Devices.git
   ```

3. Change into the project directory

   ```sh
   cd QDMI-Devices
   ```

4. Create a branch for local development

   ```sh
   git checkout -b name-of-your-bugfix-or-feature
   ```

   Now you can make your changes locally.

5. (Optional, **highly recommended**) Install [pre-commit](https://pre-commit.com/) to automatically
   run a set of checks before each commit.

<!-- prettier-ignore-start -->
   <div class="tabbed">

- <b class="tab-title">via `uv`</b> The easiest way to install pre-commit is via
  [uv](https://docs.astral.sh/uv/).

  \code{.shell} uv tool install pre-commit \endcode

- <b class="tab-title">via `brew`</b> If you use macOS, then pre-commit is in Homebrew, use

  \code{.shell} brew install pre-commit \endcode

- <b class="tab-title">via `pipx`</b> If you prefer to use [pipx](https://pypa.github.io/pipx/), you
  can install pre-commit with

  \code{.shell} pipx install pre-commit \endcode

- <b class="tab-title">via `pip`</b> If you prefer to use regular `pip` (preferably in a virtual
  environment), you can install pre-commit with

  \code{.shell} pip install pre-commit \endcode

     </div>
  <!-- prettier-ignore-end -->
     Afterwards, you can install the pre-commit hooks with

\code{.shell} pre-commit install \endcode

6. (Optional, **highly recommended**) Use Visual Studio Code Dev Containers. The required
   configurations is located under _.devcontainer_.

   6.1. Open the repository in the Visual Studio Code

   6.2 Use the
   [Dev Container](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
   plugin to **Open Folder in Container**. This should install all the dependencies and set the
   environmental variables.

## Configure and Build

Building the project requires a C compiler supporting _C11_ and a minimum CMake version of _3.19_.

QDMI Devices uses CMake as its build system. Building a project using CMake is a two-step process:

The following commands configure the project to build all devices.

```shell
cmake -S . -B build
```

After the build system is generated in the `build` directory, the devices can be built by calling.

```shell
cmake --build build
```

In case of disable a spesifc device, please use the following commanded to configure the project:

```bash
cmake --build build -DBUILD_THE_NAME_OF_THE_DEVICE=OFF
```

The possible option for `DBUILD_THE_NAME_OF_THE_DEVICE` are as follows:

- `BUILD_BACKEND_QLM` : Builds the QDMI Device for Qaptiva Device
- `BUILD_BACKEND_DCDB` : Builds the QDMI Device for DCDB

The other options are as follows:

- `BUILD_BACKEND_TESTS` : Builds the tests for the enabled devices
- `BUILD_DOCUMENTATION` : Builds the documentation for the enabled devices

## Device-Spesific Dependencies {#dependencies}

To build and run the devices following packages are required.

<!-- prettier-ignore-start -->
   <div class="tabbed">

- <b class="tab-title">QLM</b> To build the _QLM Device_, Python and
  [myQLM](https://myqlm.github.io) package is required.

- <b class="tab-title">DCDB</b> To build the _DCDB Device_, [DCDB](https://gitlab.lrz.de/dcdb/dcdb)
is required.

   </div>
<!-- prettier-ignore-end -->

## Environment Variables {#environment-variables}

To build and run the devices following environment variables needs to be set.

<!-- prettier-ignore-start -->
   <div class="tabbed">

- <b class="tab-title">QLM</b>
  - _QLM_AUXILIARY_SCRIPT_LOCATION_ : The location of the QLM auxiliary script.
  - _QLM_AUXILIARY_SCRIPT_NAME_ : The name of the QLM auxiliary script.
  - _QLM_HOST_URL_ : The host url where QLM hardware is hosted.

- <b class="tab-title">DCDB</b>
  - _DCDB_SOURCE_LOCATION_ : The location of the DCDB source.
  - _LD_LIBRARY_PATH_ : The library of the DCDB should be append.
  - _DCDB_HOST_URL_ : The host url where DCDB is hosted.

   </div>
