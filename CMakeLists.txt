cmake_minimum_required(VERSION 3.10)
project(core_azahar)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/core_launcher.cpp
    src/core_azahar.cpp
)

add_library(core_azahar SHARED ${SOURCES})

target_compile_definitions(core_azahar PRIVATE CORE_AZAHAR_EXPORTS)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_link_libraries(core_azahar PRIVATE libcurl)