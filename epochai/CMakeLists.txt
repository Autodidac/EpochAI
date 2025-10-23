cmake_minimum_required(VERSION 3.20)

project(epochai_app LANGUAGES CXX)

set(SRC
    src/main.cpp
    src/tokenizer.cpp
    src/count_metrics.cpp
    src/state.cpp
    src/io_utils.cpp
    src/logger.cpp
    src/http_client.cpp
    src/app.cpp
)

add_executable(epochai ${SRC})

target_include_directories(epochai PRIVATE include)

target_compile_features(epochai PUBLIC cxx_std_23)

target_compile_definitions(epochai PRIVATE EPOCHAI_VERSION="1.0.0")

if(MSVC)
    target_compile_options(epochai PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(epochai PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS epochai RUNTIME DESTINATION bin)

