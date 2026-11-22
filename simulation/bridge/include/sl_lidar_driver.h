// Stand-in for the Slamtec RPLidar SDK driver interface (digital twin build
// only); see sl_lidar.h.
#pragma once

#include "sl_lidar.h"

namespace sl {

class IChannel {
public:
    virtual ~IChannel() = default;
};

template <typename T>
struct Result {
    T value;
    T &operator*() { return value; }
};

class ILidarDriver {
public:
    virtual ~ILidarDriver();
    sl_result connect(IChannel *channel);
    void disconnect();
    sl_result getDeviceInfo(sl_lidar_response_device_info_t &info, sl_u32 timeout = 1000);
    sl_result setMotorSpeed(sl_u16 speed = DEFAULT_MOTOR_SPEED);
    sl_result startScan(bool force, bool useTypicalScan, sl_u32 options = 0, void *outUsedScanMode = nullptr);
    sl_result stop(sl_u32 timeout = 1000);
    // Blocks until the simulated LiDAR finishes a new revolution, then copies
    // its points (at most count) and sets count to the number copied.
    sl_result grabScanDataHq(sl_lidar_response_measurement_node_hq_t *nodes, size_t &count,
                             sl_u32 timeout = 2000);
    sl_result ascendScanData(sl_lidar_response_measurement_node_hq_t *nodes, size_t count);
private:
    unsigned long last_seq_ = 0;
    bool scanning_ = false;
};

Result<ILidarDriver *> createLidarDriver();
Result<IChannel *> createSerialPortChannel(const std::string &device, int baudrate);

}  // namespace sl
