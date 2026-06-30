include(FetchContent)

FetchContent_Declare(
    mqss_client
    #GIT_REPOSITORY https://github.com/Munich-Quantum-Software-Stack/MQSS-Client
    #GIT_TAG develop
    SOURCE_DIR /workspaces/MQSS-Client
)

FetchContent_MakeAvailable(mqss_client)

FetchContent_GetProperties(mqss_client)
