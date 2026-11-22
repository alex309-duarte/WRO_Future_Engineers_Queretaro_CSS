// See sim_link.hpp.
#include "sim_link.hpp"

#include <zmq.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace wro_sim {

void *Zmq_Context() {
    static void *context = zmq_ctx_new();
    return context;
}

std::string Sim_Host() {
    const char *env = std::getenv("WRO_SIM_HOST");
    if (env != nullptr && *env != '\0') return env;
    // Default route in /proc/net/route: destination 00000000, gateway in
    // little-endian hex.
    std::ifstream route("/proc/net/route");
    std::string line;
    std::getline(route, line);  // header
    while (std::getline(route, line)) {
        std::istringstream in(line);
        std::string iface, dest, gateway;
        in >> iface >> dest >> gateway;
        if (dest == "00000000" && gateway != "00000000") {
            const unsigned long g = std::strtoul(gateway.c_str(), nullptr, 16);
            char text[32];
            std::snprintf(text, sizeof(text), "%lu.%lu.%lu.%lu", g & 0xFF, (g >> 8) & 0xFF,
                          (g >> 16) & 0xFF, (g >> 24) & 0xFF);
            return text;
        }
    }
    return "127.0.0.1";
}

SimChannel::SimChannel(int port, int timeout_ms) : timeout_ms_(timeout_ms) {
    endpoint_ = "tcp://" + Sim_Host() + ":" + std::to_string(port);
}

SimChannel::~SimChannel() { Close(); }

void SimChannel::Open() {
    socket_ = zmq_socket(Zmq_Context(), ZMQ_REQ);
    int linger = 0;
    zmq_setsockopt(socket_, ZMQ_LINGER, &linger, sizeof(linger));
    if (zmq_connect(socket_, endpoint_.c_str()) != 0) {
        Close();
        throw std::runtime_error("cannot connect to the twin at " + endpoint_);
    }
}

void SimChannel::Close() {
    if (socket_ != nullptr) {
        zmq_close(socket_);
        socket_ = nullptr;
    }
}

std::string SimChannel::Request(const std::string &message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (socket_ == nullptr) Open();
    if (zmq_send(socket_, message.data(), message.size(), 0) < 0) {
        Close();
        throw std::runtime_error("twin " + endpoint_ + ": send failed");
    }
    zmq_pollitem_t item{socket_, 0, ZMQ_POLLIN, 0};
    if (zmq_poll(&item, 1, timeout_ms_) <= 0 || !(item.revents & ZMQ_POLLIN)) {
        // Lazy Pirate: a REQ socket that lost its reply cannot send again.
        Close();
        throw std::runtime_error("twin " + endpoint_ + " did not answer '" + message +
                                 "' (is the scene running?)");
    }
    zmq_msg_t reply;
    zmq_msg_init(&reply);
    if (zmq_msg_recv(&reply, socket_, 0) < 0) {
        zmq_msg_close(&reply);
        Close();
        throw std::runtime_error("twin " + endpoint_ + ": receive failed");
    }
    std::string out(static_cast<const char *>(zmq_msg_data(&reply)), zmq_msg_size(&reply));
    zmq_msg_close(&reply);
    return out;
}

}  // namespace wro_sim
