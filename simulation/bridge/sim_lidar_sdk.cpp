// Slamtec SDK stand-in for the digital twin; see include/sl_lidar.h. The
// repository's driver (src/ondevice/Oradar_S2L.cpp) creates the driver,
// connects, starts the scan and calls grabScanDataHq() in its writer thread,
// exactly as with the real RPLidar S2L. Here each grabScanDataHq() returns one
// full revolution of the simulated S2L: its points carry the raw LiDAR angle
// (clockwise from the LiDAR's front mark, which the robot mounts at 90 deg)
// and the distance, so the driver's own mounting conversion is exercised.
#include "sl_lidar_driver.h"
#include "sim_link.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

namespace {

wro_sim::SimChannel &Channel() {
    static wro_sim::SimChannel channel(wro_sim::kPortLidar);
    return channel;
}

class SimChannelHandle : public sl::IChannel {};

}  // namespace

namespace sl {

ILidarDriver::~ILidarDriver() = default;

Result<ILidarDriver *> createLidarDriver() { return Result<ILidarDriver *>{new ILidarDriver}; }

Result<IChannel *> createSerialPortChannel(const std::string &, int) {
    return Result<IChannel *>{new SimChannelHandle};
}

sl_result ILidarDriver::connect(IChannel *) {
    try {
        Channel().Request("hello");
        return SL_RESULT_OK;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Twin LiDAR: %s\n", e.what());
        return SL_RESULT_OPERATION_FAIL;
    }
}

void ILidarDriver::disconnect() { scanning_ = false; }

sl_result ILidarDriver::getDeviceInfo(sl_lidar_response_device_info_t &info, sl_u32) {
    std::memset(&info, 0, sizeof(info));
    info.model = 0x61;  // S2 series
    info.firmware_version = 0x0102;
    info.hardware_version = 18;
    return SL_RESULT_OK;
}

sl_result ILidarDriver::setMotorSpeed(sl_u16) { return SL_RESULT_OK; }

sl_result ILidarDriver::startScan(bool, bool, sl_u32, void *) {
    scanning_ = true;
    return SL_RESULT_OK;
}

sl_result ILidarDriver::stop(sl_u32) {
    scanning_ = false;
    return SL_RESULT_OK;
}

// Reply of "scan <last seq>": uint32 seq, uint32 n, then n pairs of float32
// (raw LiDAR angle in deg, distance in mm; 0 = no return).
sl_result ILidarDriver::grabScanDataHq(sl_lidar_response_measurement_node_hq_t *nodes,
                                       size_t &count, sl_u32 timeout) {
    if (!scanning_) return SL_RESULT_OPERATION_FAIL;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
    while (std::chrono::steady_clock::now() < deadline) {
        std::string reply;
        try {
            reply = Channel().Request("scan " + std::to_string(last_seq_));
        } catch (const std::exception &e) {
            std::fprintf(stderr, "Twin LiDAR: %s\n", e.what());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        if (reply.size() < 8) return SL_RESULT_OPERATION_FAIL;
        std::uint32_t header[2];
        std::memcpy(header, reply.data(), sizeof(header));
        const std::uint32_t seq = header[0];
        const std::uint32_t n = header[1];
        if (n == 0 || seq == last_seq_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        last_seq_ = seq;
        const size_t available = std::min<size_t>(n, (reply.size() - 8) / 8);
        const size_t used = std::min(available, count);
        const char *data = reply.data() + 8;
        for (size_t i = 0; i < used; ++i) {
            float pair[2];
            std::memcpy(pair, data + i * 8, sizeof(pair));
            float angle = pair[0];
            while (angle < 0.0F) angle += 360.0F;
            while (angle >= 360.0F) angle -= 360.0F;
            nodes[i].angle_z_q14 = static_cast<sl_u16>(angle * 16384.0F / 90.0F);
            nodes[i].dist_mm_q2 = static_cast<sl_u32>(std::max(0.0F, pair[1]) * 4.0F);
            nodes[i].quality = pair[1] > 0.0F ? 15 : 0;
            nodes[i].flag = i == 0 ? 1 : 0;
        }
        count = used;
        return SL_RESULT_OK;
    }
    return SL_RESULT_OPERATION_TIMEOUT;
}

sl_result ILidarDriver::ascendScanData(sl_lidar_response_measurement_node_hq_t *nodes, size_t count) {
    std::sort(nodes, nodes + count,
              [](const sl_lidar_response_measurement_node_hq_t &a,
                 const sl_lidar_response_measurement_node_hq_t &b) {
                  return a.angle_z_q14 < b.angle_z_q14;
              });
    return SL_RESULT_OK;
}

}  // namespace sl
