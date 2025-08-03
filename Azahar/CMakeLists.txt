cmake_minimum_required(VERSION 3.14)
project(libretro_azahar_launcher)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- JSON (header-only) ---
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
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

# Disable unnecessary protocols
set(CURL_DISABLE_FTP ON CACHE BOOL "Disable FTP support" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "Disable LDAP support" FORCE)
set(CURL_DISABLE_DICT ON CACHE BOOL "Disable DICT support" FORCE)
set(CURL_DISABLE_TELNET ON CACHE BOOL "Disable TELNET support" FORCE)
set(CURL_DISABLE_RTSP ON CACHE BOOL "Disable RTSP support" FORCE)
set(CURL_DISABLE_SMTP ON CACHE BOOL "Disable SMTP support" FORCE)
set(CURL_DISABLE_IMAP ON CACHE BOOL "Disable IMAP support" FORCE)
set(CURL_DISABLE_POP3 ON CACHE BOOL "Disable POP3 support" FORCE)
set(CURL_DISABLE_MQTT ON CACHE BOOL "Disable MQTT support" FORCE)
set(CURL_DISABLE_SMB ON CACHE BOOL "Disable SMB support" FORCE)
set(CURL_DISABLE_LDAPS ON CACHE BOOL "Disable LDAPS support" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "Disable libpsl usage" FORCE)

# Platform-specific TLS backend
if(WIN32)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "Use Windows native SSL backend" FORCE)
else()
    # For Linux/macOS, use OpenSSL (must be installed on system or provided)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    set(CMAKE_USE_OPENSSL ON)
endif()

FetchContent_MakeAvailable(curl)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/core.cpp
)

add_library(libretro_azahar_launcher SHARED ${SOURCES})

target_compile_definitions(libretro_azahar_launcher PRIVATE
    CORE_AZAHAR_EXPORTS
    CURL_STATICLIB
)

target_link_libraries(libretro_azahar_launcher PRIVATE
    nlohmann_json
    CURL::libcurl
)
