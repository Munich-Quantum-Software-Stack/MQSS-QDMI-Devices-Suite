include(FetchContent)

FetchContent_Declare(
    qinfo
    GIT_REPOSITORY git@github.com:Munich-Quantum-Software-Stack/qinfo.git
    GIT_TAG develop
)

FetchContent_MakeAvailable(qinfo)
