cmake_minimum_required(VERSION 3.16)
project(wro_twin_obstacle LANGUAGES C CXX)

# Obstacle Challenge program of the repository, built for the CoppeliaSim
# digital twin. The mission, the LiDAR driver, the board layer (spike_compat,
# spike_hiwonder), the start-button driver and the camera pipeline are the
# repository's files, compiled unchanged from src/. Only the hardware at the
# bottom is replaced by the stand-ins in bridge/:
#   hiwonder_brick.cpp   -> bridge/sim_brick.cpp          (board in the scene)
#   Slamtec SDK          -> bridge/sim_lidar_sdk.cpp      (LiDAR in the scene)
#   libgpiod             -> bridge/sim_gpiod.cpp          (start button + LED)
#   parada_emergencia    -> bridge/sim_emergency_stop.cpp
#   HailoRT + hailo_infer-> bridge/sim_hailo.cpp          (colour detector)
#   main()               -> bridge/sim_main.cpp           (frames from the twin camera)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(WRO_REPO "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Root of the team repository")
set(SRC "${WRO_REPO}/src")
set(OD  "${SRC}/cpp/object_detection_hiwonder_h10")
set(ONDEVICE "${SRC}/ondevice")
set(BRIDGE "${CMAKE_CURRENT_SOURCE_DIR}/bridge")

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)
find_package(CURL REQUIRED)

# LiDAR mounting, read from common_var.h as the robot build does.
file(STRINGS "${OD}/common_var.h" _montaje
     REGEX "^#define[ \t]+Lidar_Angle_Orientation_fixed_to_robot[ \t]+[0-9]+")
string(REGEX MATCH "[0-9]+$" RPLIDAR_FRONT_DEG "${_montaje}")
message(STATUS "Twin: RPLidar front of the robot = lidar angle ${RPLIDAR_FRONT_DEG}")

# The mission is the repository's file. -DWRO_MISSION_PATCH=<file.patch> builds
# it from a patched copy instead (proposed/*.patch: changes to the mission that
# are being tried in the twin before they go to the robot).
# proposed/mission_changes.patch was made on the mission of release v2.1.0; the
# mission changed after it (release v2.2.0), so the patch is applied to the
# object_detection.cpp of the git tag WRO_MISSION_PATCH_REF (read from the
# repository clone with git). An empty WRO_MISSION_PATCH_REF uses the current file.
set(WRO_MISSION_PATCH "" CACHE FILEPATH "Patch applied to a copy of object_detection.cpp")
set(WRO_MISSION_PATCH_REF "v2.1.0" CACHE STRING "Git tag whose object_detection.cpp the patch is applied to")
set(MISSION_SRC "${OD}/object_detection.cpp")
if(WRO_MISSION_PATCH)
    set(MISSION_SRC "${CMAKE_BINARY_DIR}/patched/object_detection.cpp")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/patched")
    set(_mission_base "${OD}/object_detection.cpp")
    if(WRO_MISSION_PATCH_REF)
        set(_mission_base "${CMAKE_BINARY_DIR}/patched/object_detection_${WRO_MISSION_PATCH_REF}.cpp")
        execute_process(
            COMMAND git -C "${WRO_REPO}" show "${WRO_MISSION_PATCH_REF}:src/cpp/object_detection_hiwonder_h10/object_detection.cpp"
            OUTPUT_FILE "${_mission_base}"
            RESULT_VARIABLE _git_rc)
        if(_git_rc)
            message(FATAL_ERROR "Could not read object_detection.cpp of ${WRO_MISSION_PATCH_REF} from ${WRO_REPO} "
                                "(the patch needs a git clone with tags; -DWRO_MISSION_PATCH_REF= uses the current file)")
        endif()
    endif()
    execute_process(
        COMMAND patch --binary -s -o "${MISSION_SRC}" "${_mission_base}" "${WRO_MISSION_PATCH}"
        RESULT_VARIABLE _patch_rc)
    if(_patch_rc)
        message(FATAL_ERROR "Could not apply ${WRO_MISSION_PATCH} to ${_mission_base}")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${WRO_MISSION_PATCH}" "${OD}/object_detection.cpp")
    message(STATUS "Twin: mission built with ${WRO_MISSION_PATCH} (on ${_mission_base})")
endif()

add_executable(object_detection_twin
    # Repository code (unchanged, or with WRO_MISSION_PATCH)
    ${MISSION_SRC}
    ${OD}/spike_reto2.cpp
    ${OD}/lidar_reto2.cpp
    ${OD}/rasp_gpio.cpp
    ${OD}/toolbox.cpp
    ${OD}/grabador.cpp
    ${OD}/utils/utils.cpp
    ${SRC}/cpp/common/resources_manager.cpp
    ${ONDEVICE}/Oradar_S2L.cpp
    ${ONDEVICE}/spike_hiwonder.cpp
    ${ONDEVICE}/spike_compat.cpp
    # Twin stand-ins
    ${BRIDGE}/sim_link.cpp
    ${BRIDGE}/sim_brick.cpp
    ${BRIDGE}/sim_lidar_sdk.cpp
    ${BRIDGE}/sim_gpiod.cpp
    ${BRIDGE}/sim_emergency_stop.cpp
    ${BRIDGE}/sim_hailo.cpp
    ${BRIDGE}/sim_main.cpp
    # Surprise rule: the team's plan (README 6.10) as a twin prototype
    ${BRIDGE}/surprise_rule.cpp
)

# The robot program's own main() is kept but renamed; sim_main.cpp replaces it.
set_source_files_properties(${MISSION_SRC} PROPERTIES COMPILE_DEFINITIONS "main=wro_vehicle_main")

target_compile_definitions(object_detection_twin PRIVATE
    WRO_LIDAR_RPLIDAR_S2L
    RPLIDAR_FRONT_DEG=${RPLIDAR_FRONT_DEG}
    RPLIDAR_CLOCKWISE=1)

# bridge/include first: its hailo/, sl_lidar*.h and gpiod.h stand in for the
# vendor headers.
target_include_directories(object_detection_twin PRIVATE
    ${BRIDGE}/include
    ${BRIDGE}
    ${OD}
    ${OD}/utils
    ${SRC}/cpp/common
    ${ONDEVICE}
    ${ONDEVICE}/hiwonder/include
    ${OpenCV_INCLUDE_DIRS}
    ${ZMQ_INCLUDE_DIRS}
    ${YAMLCPP_INCLUDE_DIRS})

target_link_libraries(object_detection_twin PRIVATE
    ${OpenCV_LIBS} ${ZMQ_LIBRARIES} ${YAMLCPP_LIBRARIES} CURL::libcurl Threads::Threads)

# Same config files next to the executable as the robot build.
add_custom_command(TARGET object_detection_twin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OD}/visualization_config.yaml"
            "$<TARGET_FILE_DIR:object_detection_twin>/visualization_config.yaml"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:object_detection_twin>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC}/config/resources_config.yaml"
            "$<TARGET_FILE_DIR:object_detection_twin>/config/resources_config.yaml")
