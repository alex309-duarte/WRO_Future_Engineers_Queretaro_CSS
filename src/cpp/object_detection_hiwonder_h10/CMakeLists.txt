cmake_minimum_required(VERSION 3.16)
project(object_detection LANGUAGES C CXX)

# ============================================================
# Settings
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Option: allow advanced users to use system/vcpkg instead of submodules
option(HAILO_USE_SYSTEM_DEPS "Use system-installed yaml-cpp/libcurl instead of submodules" OFF)

# ============================================================
# Core dependencies
# ============================================================
find_package(Threads REQUIRED)
find_package(HailoRT REQUIRED)
find_package(OpenCV REQUIRED)

message(STATUS "Found OpenCV includes: ${OpenCV_INCLUDE_DIRS}")

# ============================================================
# Third-party directory (submodules live here in this repo)
# <repo>/hailo_apps/cpp/object_detection
# <repo>/hailo_apps/cpp/external/{yaml-cpp,curl}
# ============================================================
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external")

# ============================================================
# yaml-cpp
# ============================================================
if(HAILO_USE_SYSTEM_DEPS)
    find_package(yaml-cpp REQUIRED)
    if(TARGET yaml-cpp::yaml-cpp AND NOT TARGET yaml-cpp)
        add_library(yaml-cpp ALIAS yaml-cpp::yaml-cpp)
    endif()
else()
    if(NOT EXISTS "${THIRD_PARTY_DIR}/yaml-cpp/CMakeLists.txt")
        message(FATAL_ERROR
            "Missing submodule: yaml-cpp\n"
            "Expected path:\n"
            "  ${THIRD_PARTY_DIR}/yaml-cpp\n\n"
            "Fix:\n"
            "  git submodule update --init --recursive\n"
            "Or clone with:\n"
            "  git clone --recurse-submodules <repo>\n"
        )
    endif()

    # yaml-cpp: build only the library (no tools/tests)
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    add_subdirectory(
        "${THIRD_PARTY_DIR}/yaml-cpp"
        "${CMAKE_BINARY_DIR}/_deps/yaml-cpp"
    )
endif()

if(NOT TARGET yaml-cpp)
    message(FATAL_ERROR "yaml-cpp target was not created. Check yaml-cpp submodule/build.")
endif()

# ============================================================
# libcurl
# ============================================================
if(HAILO_USE_SYSTEM_DEPS)
    find_package(CURL REQUIRED)
else()
    if(NOT EXISTS "${THIRD_PARTY_DIR}/curl/CMakeLists.txt")
        message(FATAL_ERROR
            "Missing submodule: curl\n"
            "Expected path:\n"
            "  ${THIRD_PARTY_DIR}/curl\n\n"
            "Fix:\n"
            "  git submodule update --init --recursive\n"
            "Or clone with:\n"
            "  git clone --recurse-submodules <repo>\n"
        )
    endif()

    # ---- Build minimal libcurl (deterministic, avoids extra deps) ----
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_TESTS ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    # Disable optional deps that break Windows builds (Libpsl/Brotli/Zstd/NGHTTP2/LDAP)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
    set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
    set(CURL_DISABLE_LDAPS ON CACHE BOOL "" FORCE)

    # TLS backend: native on Windows; keep Linux default simple
    if(WIN32)
        set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
        set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    endif()

    add_subdirectory(
        "${THIRD_PARTY_DIR}/curl"
        "${CMAKE_BINARY_DIR}/_deps/curl"
    )

    # curl submodule usually exports 'libcurl' target; create CURL::libcurl alias
    if(TARGET libcurl AND NOT TARGET CURL::libcurl)
        add_library(CURL::libcurl ALIAS libcurl)
    endif()
endif()

if(NOT TARGET CURL::libcurl)
    message(FATAL_ERROR "libcurl target missing (CURL::libcurl). Check curl submodule/build.")
endif()

# ============================================================
# Sources
# ============================================================
set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/object_detection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Oradar_S2L.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/rasp_gpio.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/hiwonder_runtime.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/spike_distance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/hiwonder/src/hiwonder_brick.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/hiwonder/src/spike_compat.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/utils.cpp"
)
list(APPEND SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/toolbox.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common/hailo_infer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common/resources_manager.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Copy repo-level YAML (hailo-apps/config/...) next to the built executable
set(HAILO_APPS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")  # object_detection -> hailo-apps
set(RESOURCES_YAML_SRC "${HAILO_APPS_ROOT}/config/resources_config.yaml")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${RESOURCES_YAML_SRC}"
          "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config/resources_config.yaml"
  COMMENT "Copy resources_config.yaml next to the executable"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/visualization_config.yaml"
          "$<TARGET_FILE_DIR:${PROJECT_NAME}>/visualization_config.yaml"
  COMMENT "Copy Hiwonder visualization config next to the executable"
)

# ============================================================
# Oradar SDK (MS200/MS2000)
# ============================================================
set(ORADAR_SDK_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../object_detection_original_h10/oradar_sdk")
if(NOT EXISTS "${ORADAR_SDK_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "Oradar SDK not found at ${ORADAR_SDK_DIR}. "
        "Keep this variant next to object_detection_original_h10.")
endif()
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${ORADAR_SDK_DIR}" "${CMAKE_BINARY_DIR}/_deps/oradar_sdk")

# ============================================================
# Include dirs
# ============================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/hiwonder/include
    "${CMAKE_CURRENT_SOURCE_DIR}/../common"
    "${ORADAR_SDK_DIR}"
)

# ============================================================
# Compile options
# ============================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -O3
        -fconcepts
        -Wno-ignored-qualifiers
        -Wno-extra
        -Wno-stringop-truncation
        -Wno-reorder
    )
endif()

# ============================================================
# Link libs
# ============================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    HailoRT::libhailort
    Threads::Threads
    ${OpenCV_LIBS}
    yaml-cpp
    CURL::libcurl
    oradar_sdk
    gpiod
)

# ---- Link stdc++fs only when needed (GCC < 9) ----
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
    endif()
endif()

# Prueba offline: no abre puertos ni toca actuadores y conserva sus
# comprobaciones también en Release (-DNDEBUG).
enable_testing()
add_executable(spike_compat_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/spike_compat_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/hiwonder/src/hiwonder_brick.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/hiwonder/src/spike_compat.cpp"
)
target_include_directories(spike_compat_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/hiwonder/include"
)
target_link_libraries(spike_compat_test PRIVATE Threads::Threads)
target_compile_options(spike_compat_test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME spike_compat_test COMMAND spike_compat_test)
