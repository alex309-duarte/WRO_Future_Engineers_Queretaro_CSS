// Stand-in for HailoRT's C++ inference API (digital twin build only); see
// hailort.h. Only what the repository's pipeline code names is declared.
#pragma once

#include "hailort.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace hailort {

// HailoRT's Expected<T>: the pipeline only calls release() and value().
template <typename T>
class Expected {
public:
    Expected() = default;
    explicit Expected(T v) : value_(std::move(v)) {}
    T release() { return std::move(value_); }
    T &value() { return value_; }
    bool has_value() const { return true; }
    hailo_status status() const { return HAILO_SUCCESS; }
    explicit operator bool() const { return true; }
private:
    T value_{};
};

class Hef {
public:
    Hef() = default;
    explicit Hef(std::vector<hailo_vstream_info_t> inputs) : inputs_(std::move(inputs)) {}
    Expected<std::vector<hailo_vstream_info_t>> get_input_vstream_infos() const {
        return Expected<std::vector<hailo_vstream_info_t>>(inputs_);
    }
private:
    std::vector<hailo_vstream_info_t> inputs_;
};

class InferModel {
public:
    class InferStream {
    public:
        InferStream() = default;
        explicit InferStream(const hailo_vstream_info_t &info) : info_(info) {}
        std::string name() const { return info_.name; }
        hailo_3d_image_shape_t shape() const { return info_.shape; }
        hailo_format_t format() const { return info_.format; }
        size_t get_frame_size() const {
            return static_cast<size_t>(info_.shape.height) * info_.shape.width * info_.shape.features;
        }
        const hailo_vstream_info_t &info() const { return info_; }
    private:
        hailo_vstream_info_t info_{};
    };

    InferModel() = default;
    InferModel(std::vector<InferStream> inputs, std::vector<InferStream> outputs, Hef hef)
        : inputs_(std::move(inputs)), outputs_(std::move(outputs)), hef_(std::move(hef)) {}

    const std::vector<InferStream> &inputs() const { return inputs_; }
    const std::vector<InferStream> &outputs() const { return outputs_; }
    std::vector<std::string> get_input_names() const {
        std::vector<std::string> n;
        for (const auto &s : inputs_) n.push_back(s.name());
        return n;
    }
    std::vector<std::string> get_output_names() const {
        std::vector<std::string> n;
        for (const auto &s : outputs_) n.push_back(s.name());
        return n;
    }
    const Hef &hef() const { return hef_; }

private:
    std::vector<InferStream> inputs_;
    std::vector<InferStream> outputs_;
    Hef hef_;
};

class ConfiguredInferModel {
public:
    class Bindings {};
};

class AsyncInferJob {
public:
    void wait() {}
};

struct AsyncInferCompletionInfo {
    hailo_status status = HAILO_SUCCESS;
};

class VDevice {};

}  // namespace hailort
