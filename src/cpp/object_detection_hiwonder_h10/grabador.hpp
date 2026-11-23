#ifndef _GRABADOR_HPP
#define _GRABADOR_HPP

// Video recorder ("grabador") for -s / --save-stream-output that never slows
// down the caller. Encoding a frame costs about 10 ms on the Pi 5 (MJPG
// 720x720 or 800x600); doing it inside the post-process thread delayed the
// pillar detections the mission reads. Here the caller only copies the frame
// into a two-slot queue; a low-priority thread (nice 19) resizes and encodes it.
// When the encoder falls behind, new frames are DROPPED instead of making the
// caller wait (they are counted and reported on Cerrar()).
//
// MJPG in .avi: frames already written stay readable even if the process ends
// with _exit() on the emergency stop.

#include <opencv2/opencv.hpp>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

// -s records one out of every GRABAR_CADA_N camera frames (2: 15 fps from a
// 30 fps camera), for the camera video and the lidar video alike, so both stay
// the same length. Halves the encoding CPU; 1 = every frame.
constexpr int GRABAR_CADA_N = 2;

class Grabador {
public:
    // Records one out of every cada_n frames passed to Agregar(), at fps / cada_n,
    // resized to tamano. Opens the file in the recording thread; returns false
    // only if it is already open.
    bool Abrir(const std::string &ruta, double fps, cv::Size tamano, int cada_n);
    // Copies the frame (one out of every cada_n calls) into the queue, or drops
    // it if the queue is full. Never blocks on encoding. No-op if not open.
    void Agregar(const cv::Mat &frame);
    // Writes the queued frames, closes the file and joins the thread.
    void Cerrar();
    bool Abierto() const { return abierto_; }
    ~Grabador() { Cerrar(); }

private:
    void Hilo();

    static constexpr size_t MAX_COLA = 2;

    std::string ruta_;
    double fps_ = 30.0;
    cv::Size tamano_;
    int cada_n_ = 1;
    long contador_ = 0;

    std::atomic<bool> abierto_{false};
    bool parar_ = false;
    std::deque<cv::Mat> cola_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread hilo_;
    long escritos_ = 0;
    long descartados_ = 0;
};

#endif // _GRABADOR_HPP
