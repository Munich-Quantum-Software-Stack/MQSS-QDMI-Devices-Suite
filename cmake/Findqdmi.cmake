include(FetchContent)

FetchContent_Declare(
    qdmi
    GIT_REPOSITORY git@github.com:Munich-Quantum-Software-Stack/QDMI.git
    GIT_TAG develop
)

FetchContent_MakeAvailable(qdmi)
FetchContent_GetProperties(qdmi)

# workaround to get access to internal headers
target_include_directories(qdmi PUBLIC $<BUILD_INTERFACE:${qdmi_SOURCE_DIR}/src>)
