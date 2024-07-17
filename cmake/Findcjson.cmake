include(FetchContent)

FetchContent_Declare(
    cjson
    GIT_REPOSITORY git@github.com:DaveGamble/cJSON.git
    GIT_TAG develop
)

FetchContent_MakeAvailable(cjson)
FetchContent_GetProperties(cjson)

set(CJON_INCLUDE_DIRS "${cjson_SOURCE_DIR}/")