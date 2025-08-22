cmake_minimum_required(VERSION 3.31)

# Require user to specify the CORE and the system name
if(NOT DEFINED CORE)
    message(FATAL_ERROR "specify -DCORE=name_of_emulator (e.g., -DCORE=azahar)")
endif()

if(NOT DEFINED SYSTEM_NAME)
    message(FATAL_ERROR "specify -DSYSTEM_NAME=platform_name (e.g., -DSYSTEM_NAME=Nintendo - Nintendo 3DS)")
ENDIF()

set(VALID_CORES azahar duckstation mgba melonds pcsx2 xemu xenia)
list(FIND VALID_CORES "${CORE}" CORE_INDEX)

if(CORE_INDEX EQUAL -1)
    message(FATAL_ERROR "Invalid CORE '${CORE}'. Accepted values: ${VALID_CORES}")
endif()

project(${CORE})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Allow compilation of static libraries on Unix-like systems
if(UNIX OR APPLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)
endif()

include(FetchContent)

# --- JSON (header-only) ---
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build curl as static library" FORCE)
FetchContent_MakeAvailable(nlohmann_json)


# --- CURL ---
FetchContent_Declare(
  curl
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG curl-8_15_0
)

# Common curl options
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build curl as static library" FORCE)
set(CURL_STATICLIB ON CACHE BOOL "Build curl as static library" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "Disable curl CLI" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable curl tests" FORCE)
set(CURL_DISABLE_TESTS ON CACHE BOOL "Disable curl-specific tests" FORCE)

# Disable unnecessary protocols
set(CURL_USE_FTP OFF CACHE BOOL "Disable FTP support" FORCE)
set(CURL_USE_LDAP OFF CACHE BOOL "Disable LDAP support" FORCE)
set(CURL_USE_DICT OFF CACHE BOOL "Disable DICT support" FORCE)
set(CURL_USE_TELNET OFF CACHE BOOL "Disable TELNET support" FORCE)
set(CURL_USE_RTSP OFF CACHE BOOL "Disable RTSP support" FORCE)
set(CURL_USE_SMTP OFF CACHE BOOL "Disable SMTP support" FORCE)
set(CURL_USE_IMAP OFF CACHE BOOL "Disable IMAP support" FORCE)
set(CURL_USE_POP3 OFF CACHE BOOL "Disable POP3 support" FORCE)
set(CURL_USE_MQTT OFF CACHE BOOL "Disable MQTT support" FORCE)
set(CURL_USE_SMB OFF CACHE BOOL "Disable SMB support" FORCE)
set(CURL_USE_LDAPS OFF CACHE BOOL "Disable LDAPS support" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "Disable libpsl usage" FORCE)

# Platform-specific TLS backend
if(WIN32)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "Use Windows native SSL backend" FORCE)
else()
    find_package(GnuTLS QUIET)
    if(GnuTLS_FOUND)
        message(STATUS "Using GnuTLS for TLS")
        set(GNUTLS_USE_STATIC_LIBS TRUE)
        set(CMAKE_USE_GNUTLS ON CACHE BOOL "" FORCE)
        set(USE_GNUTLS ON CACHE BOOL "" FORCE)
    else()
        find_package(OpenSSL QUIET)
        if(OpenSSL_FOUND)
             message(STATUS "Using OpenSSL for TLS")
            set(OPENSSL_USE_STATIC_LIBS TRUE)
            set(CMAKE_USE_OPENSSL ON CACHE BOOL "" FORCE)
            set(USE_OPENSSL ON CACHE BOOL "" FORCE)
        else()
            message(FATAL_ERROR "No supported TLS backend (OpenSSL or GnuTLS) found!")
        endif()
    endif()
endif()

FetchContent_MakeAvailable(curl)

# Source files
set(SOURCES src/core.cpp)

# Define dynamic target name
set(TARGET_NAME ${CORE})

add_library(${TARGET_NAME} SHARED ${SOURCES})

# Define string macros for the core and system. Both are exported in a string.
target_compile_definitions(${TARGET_NAME} PRIVATE CORE=\"${CORE}\" SYSTEM_NAME=\"${SYSTEM_NAME}\")

target_include_directories(${TARGET_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link external dependencies
target_link_libraries(${TARGET_NAME} PRIVATE
    nlohmann_json
    CURL::libcurl
)

# Force correct output name
set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "" OUTPUT_NAME "${CORE}")
