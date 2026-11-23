// Asynchronous video recorder: see grabador.hpp.

#include "grabador.hpp"

#include <cstdio>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

bool Grabador::Abrir(const std::string &ruta, double fps, cv::Size tamano, int cada_n) {
    // Already open, or a previous attempt failed (its thread ended without
    // opening the file): do not retry on every frame.
    if (abierto_ || hilo_.joinable()) return false;
    ruta_ = ruta;
    cada_n_ = (cada_n < 1) ? 1 : cada_n;
    fps_ = (fps > 0.0 ? fps : 30.0) / cada_n_;
    tamano_ = tamano;
    contador_ = 0;
    escritos_ = 0;
    descartados_ = 0;
    parar_ = false;
    cola_.clear();
    abierto_ = true;
    hilo_ = std::thread(&Grabador::Hilo, this);
    return true;
}

void Grabador::Agregar(const cv::Mat &frame) {
    if (!abierto_ || frame.empty()) return;
    if ((contador_++ % cada_n_) != 0) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (cola_.size() >= MAX_COLA) {
        descartados_++;
        return;
    }
    cola_.push_back(frame.clone());
    cv_.notify_one();
}

void Grabador::Cerrar() {
    if (!hilo_.joinable()) return;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        parar_ = true;
    }
    cv_.notify_one();
    hilo_.join();
    abierto_ = false;
    printf("-I- Video %s: %ld frames written, %ld dropped (encoder busy)\n",
           ruta_.c_str(), escritos_, descartados_);
}

void Grabador::Hilo() {
    // Lowest priority for this thread only, so encoding takes CPU only when the
    // mission, the lidar and the Hailo pipeline leave some free.
    setpriority(PRIO_PROCESS, static_cast<id_t>(syscall(SYS_gettid)), 19);

    cv::VideoWriter video;
    video.open(ruta_, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps_, tamano_);
    if (!video.isOpened()) {
        fprintf(stderr, "-E- Could not open the video %s: not recording it\n", ruta_.c_str());
        std::lock_guard<std::mutex> lock(mutex_);
        abierto_ = false;
        cola_.clear();
        return;
    }
    printf("-I- Saving video to: %s (%.1f fps)\n", ruta_.c_str(), fps_);

    cv::Mat reducido;
    while (true) {
        cv::Mat frame;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return parar_ || !cola_.empty(); });
            if (cola_.empty()) break; // parar_ and nothing left to write
            frame = std::move(cola_.front());
            cola_.pop_front();
        }
        if (frame.size() != tamano_) {
            cv::resize(frame, reducido, tamano_, 0.0, 0.0, cv::INTER_AREA);
            video.write(reducido);
        } else {
            video.write(frame);
        }
        escritos_++;
    }
    video.release();
}
