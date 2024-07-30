include(FetchContent)

FetchContent_Declare(
    cjson
    GIT_REPOSITORY git@github.com:DaveGamble/cJSON.git
    GIT_TAG develop
)

FetchContent_MakeAvailable(cjson)
FetchContent_GetProperties(cjson)

# cJSON does not set the include directory on the target
target_include_directories(cjson PUBLIC $<BUILD_INTERFACE:${cjson_SOURCE_DIR}>)
