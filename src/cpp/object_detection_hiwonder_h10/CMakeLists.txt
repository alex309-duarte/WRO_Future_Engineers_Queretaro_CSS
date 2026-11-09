cmake_minimum_required(VERSION 3.16)
project(object_detection LANGUAGES C CXX)

# Obstacle Challenge (reto 2) build on the Hiwonder controller board: camera +
# Hailo-8 detection of traffic signs and parking markers, LiDAR and SPIKE-style
# motion API. The board layer, the calibration and the LiDAR driver are the
# Open Challenge ones (src/ondevice, main branch): they are compiled
# from there, not copied, so a calibration change in one challenge also
# applies to the other. See README.md.

# ============================================================
# Settings
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)   # the Open Challenge sources are compiled as gnu++17
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
# <repo>/src/cpp/external/{yaml-cpp,curl}
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
# Hiwonder board layer and LiDAR from the Open Challenge (src/ondevice,
# main branch); stop with a clear message if any file is missing
# ============================================================
set(ONDEVICE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../ondevice")
foreach(archivo
        spike_hiwonder.cpp parada_emergencia.cpp hiwonder_config.h Oradar_S2L.cpp
        hiwonder/src/hiwonder_brick.cpp spike_compat.cpp
        hiwonder/include/spike_compat/spike_compat.h)
    if(NOT EXISTS "${ONDEVICE_DIR}/${archivo}")
        message(FATAL_ERROR
            "Missing ${ONDEVICE_DIR}/${archivo}: the Obstacle Challenge builds the Hiwonder layer "
            "of the Open Challenge (src/ondevice). Use the main branch.")
    endif()
endforeach()

# ============================================================
# Sources
# ============================================================
set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/object_detection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/spike_reto2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lidar_reto2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/rasp_gpio.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/utils.cpp"
    "${ONDEVICE_DIR}/Oradar_S2L.cpp"
    "${ONDEVICE_DIR}/spike_hiwonder.cpp"
    "${ONDEVICE_DIR}/parada_emergencia.cpp"
    "${ONDEVICE_DIR}/hiwonder/src/hiwonder_brick.cpp"
    "${ONDEVICE_DIR}/spike_compat.cpp"
)
list(APPEND SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/toolbox.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common/hailo_infer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common/resources_manager.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Copy repo-level YAML (src/config/...) next to the built executable
set(HAILO_APPS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")  # object_detection -> src
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
# LiDAR: RPLidar S2L by default (Hailo-8 robot). A robot with an Oradar MS200
# selects it with: cmake ... -DWRO_LIDAR=ORADAR
# Both drivers (src/ondevice/Oradar_S2L.cpp) fill the same 360-entry buffer
# using the Oradar angle orientation.
# ============================================================
set(WRO_LIDAR "RPLIDAR_S2L" CACHE STRING "Robot lidar: ORADAR or RPLIDAR_S2L")
set_property(CACHE WRO_LIDAR PROPERTY STRINGS ORADAR RPLIDAR_S2L)
message(STATUS "Lidar: ${WRO_LIDAR}")

if(WRO_LIDAR STREQUAL "RPLIDAR_S2L")
    # Slamtec SDK built separately (make in its root), same commit 99478e5
    # as the src/ondevice/rplidar_sdk submodule.
    set(RPLIDAR_SDK_DIR "$ENV{HOME}/rplidar_sdk" CACHE PATH "Root of the Slamtec rplidar_sdk")
    set(RPLIDAR_LIB "${RPLIDAR_SDK_DIR}/output/Linux/Release/libsl_lidar_sdk.a")
    if(NOT EXISTS "${RPLIDAR_LIB}")
        message(FATAL_ERROR
            "${RPLIDAR_LIB} does not exist. Clone https://github.com/Slamtec/rplidar_sdk "
            "into ${RPLIDAR_SDK_DIR} and build it with make, or pass -DRPLIDAR_SDK_DIR=...")
    endif()
    # Mounting of the S2L on the robot: LiDAR angle (deg) that faces the
    # front, and rotation sense. Same values the Open Challenge ran with on
    # the track (src/ondevice/Makefile without LIDAR_FLAGS).
    set(RPLIDAR_FRONT_DEG 0 CACHE STRING "RPLidar angle that points to the front of the robot")
    set(RPLIDAR_CLOCKWISE 1 CACHE STRING "1 if the RPLidar angle increases clockwise seen from above")
    set(LIDAR_INCLUDE_DIRS "${RPLIDAR_SDK_DIR}/sdk/include" "${RPLIDAR_SDK_DIR}/sdk/src")
    set(LIDAR_LIBS "${RPLIDAR_LIB}")
    set(LIDAR_DEFINITIONS
        WRO_LIDAR_RPLIDAR_S2L
        RPLIDAR_FRONT_DEG=${RPLIDAR_FRONT_DEG}
        RPLIDAR_CLOCKWISE=${RPLIDAR_CLOCKWISE})
elseif(WRO_LIDAR STREQUAL "ORADAR")
    set(ORADAR_SDK_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../object_detection_original_h10/oradar_sdk")
    if(NOT EXISTS "${ORADAR_SDK_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "Oradar SDK not found at ${ORADAR_SDK_DIR}. "
            "Keep this variant next to object_detection_original_h10.")
    endif()
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${ORADAR_SDK_DIR}" "${CMAKE_BINARY_DIR}/_deps/oradar_sdk")
    set(LIDAR_INCLUDE_DIRS "${ORADAR_SDK_DIR}" "${ORADAR_SDK_DIR}/src")
    set(LIDAR_LIBS oradar_sdk)
    set(LIDAR_DEFINITIONS "")
else()
    message(FATAL_ERROR "WRO_LIDAR must be ORADAR or RPLIDAR_S2L, not '${WRO_LIDAR}'")
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE ${LIDAR_DEFINITIONS})

# ============================================================
# Include dirs
# ============================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    "${ONDEVICE_DIR}/hiwonder/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common"
    ${LIDAR_INCLUDE_DIRS}
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
    ${LIDAR_LIBS}
    gpiod
)

# ---- Link stdc++fs only when needed (GCC < 9) ----
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
    endif()
endif()

# Offline test of the Obstacle Challenge layer against the Open Challenge fake
# board: opens no serial ports, drives no actuators, and keeps its checks
# active in Release builds too (-DNDEBUG). Run with ctest or ./reto2_test.
enable_testing()
add_executable(reto2_test
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/reto2_test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/spike_reto2.cpp"
    "${ONDEVICE_DIR}/tests/fake_brick.cpp"
    "${ONDEVICE_DIR}/spike_hiwonder.cpp"
    "${ONDEVICE_DIR}/spike_compat.cpp"
)
target_include_directories(reto2_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${ONDEVICE_DIR}/hiwonder/include"
)
target_link_libraries(reto2_test PRIVATE Threads::Threads)
target_compile_options(reto2_test PRIVATE -Wall -Wextra)
add_test(NAME reto2_test COMMAND reto2_test)
