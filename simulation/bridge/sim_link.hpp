// Link between the robot program and the digital twin in CoppeliaSim.
//
// The scene script (scene/twin_car.lua) opens one ZeroMQ REP socket per
// device of the robot:
//   23101  Hiwonder board  (drive motor, steering servo, IMU, start button, LED)
//   23102  RPLidar S2L     (one message per full revolution)
//   23103  Camera Module 3 Wide (one RGB frame per request)
// Each device of the stand-in layer owns a SimChannel to its port and sends
// short text requests; the replies are text (board) or binary (LiDAR, camera).
//
// The scene runs on Windows and the program in WSL, so by default the host
// is the WSL default gateway (the Windows side). WRO_SIM_HOST overrides it.
#pragma once

#include <mutex>
#include <string>

namespace wro_sim {

constexpr int kPortBoard = 23101;
constexpr int kPortLidar = 23102;
constexpr int kPortCamera = 23103;

// Host of the CoppeliaSim scene: WRO_SIM_HOST, else the default gateway (the
// Windows host when running in WSL), else 127.0.0.1.
std::string Sim_Host();

// One request/reply channel (ZeroMQ REQ). Thread-safe: requests from several
// threads are serialised. On a timeout the socket is rebuilt and the call
// throws std::runtime_error.
class SimChannel {
public:
    explicit SimChannel(int port, int timeout_ms = 1500);
    ~SimChannel();
    SimChannel(const SimChannel &) = delete;
    SimChannel &operator=(const SimChannel &) = delete;

    std::string Request(const std::string &message);

private:
    void Open();
    void Close();

    std::mutex mutex_;
    std::string endpoint_;
    int timeout_ms_;
    void *socket_ = nullptr;
};

// Process-wide ZeroMQ context.
void *Zmq_Context();

}  // namespace wro_sim
