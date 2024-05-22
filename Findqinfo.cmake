include(FetchContent)

FetchContent_Declare(
    qinfo
    GIT_REPOSITORY git@github.com:Munich-Quantum-Software-Stack/QInfo.git
    GIT_TAG testing
)

FetchContent_MakeAvailable(qinfo)
