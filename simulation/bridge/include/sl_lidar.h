// Stand-in for the Slamtec RPLidar SDK header (digital twin build only).
// The repository's LiDAR driver (src/ondevice/Oradar_S2L.cpp, RPLidar S2L
// branch) is compiled unchanged against it; sim_lidar_sdk.cpp answers its
// calls with the scans of the simulated RPLidar S2L in CoppeliaSim.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

typedef uint32_t sl_result;
typedef uint8_t sl_u8;
typedef uint16_t sl_u16;
typedef uint32_t sl_u32;

#define SL_RESULT_OK              ((sl_result)0)
#define SL_RESULT_FAIL_BIT        ((sl_result)0x80000000)
#define SL_RESULT_OPERATION_FAIL  ((sl_result)(0x8004 | SL_RESULT_FAIL_BIT))
#define SL_RESULT_OPERATION_TIMEOUT ((sl_result)(0x8002 | SL_RESULT_FAIL_BIT))
#define SL_IS_OK(x)    (((x) & SL_RESULT_FAIL_BIT) == 0)
#define SL_IS_FAIL(x)  (((x) & SL_RESULT_FAIL_BIT))

typedef struct {
    sl_u8 model;
    sl_u16 firmware_version;
    sl_u8 hardware_version;
    sl_u8 serialnum[16];
} sl_lidar_response_device_info_t;

// angle_z_q14: degrees * 16384 / 90; dist_mm_q2: millimetres * 4.
typedef struct {
    sl_u16 angle_z_q14;
    sl_u32 dist_mm_q2;
    sl_u8 quality;
    sl_u8 flag;
} sl_lidar_response_measurement_node_hq_t;

#define DEFAULT_MOTOR_SPEED (0xFFFFu)
