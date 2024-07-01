include(FetchContent)

FetchContent_Declare(
    qdmi
    GIT_REPOSITORY git@github.com:Munich-Quantum-Software-Stack/QDMI.git
    GIT_TAG develop
)

FetchContent_MakeAvailable(qdmi)
FetchContent_GetProperties(qdmi)

set(QDMI_INCLUDE_DIRS "${qdmi_SOURCE_DIR}/include")
set(QDMI_SRC_DIRS "${qdmi_SOURCE_DIR}/src")