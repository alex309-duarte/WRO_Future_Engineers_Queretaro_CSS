// HailoInfer for the digital twin: the class the camera pipeline uses
// (src/cpp/common/hailo_infer.hpp), linked instead of hailo_infer.cpp. The
// YOLOv8n network cannot run without the Hailo-8, so this "model" is a colour
// detector for the three classes of the real model (green traffic sign, red
// traffic sign, magenta parking marker). It receives the same 640x640 RGB
// input the network gets and writes the same output: the Hailo NMS-by-class
// buffer (per class: a float count, then y_min, x_min, y_max, x_max, score,
// normalised), which the repository's parse_nms_data() and
// postprocess_callback() read unchanged.
#include "hailo_infer.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cstring>
#include <vector>

namespace {

constexpr uint32_t kInputSize = 640;
constexpr uint32_t kClasses = 3;            // 1 green, 2 red, 3 parking (class id = index + 1)
constexpr uint32_t kMaxBoxesPerClass = 100;
constexpr int kMaxBoxesTotal = 10;           // postprocess_callback keeps 10 areas

struct Box {
    int cls;  // 0 green, 1 red, 2 parking marker
    float x_min, y_min, x_max, y_max, score, area;
};

hailo_vstream_info_t MakeInfo(const char *name, uint32_t h, uint32_t w, uint32_t f) {
    hailo_vstream_info_t info{};
    std::strncpy(info.name, name, HAILO_MAX_STREAM_NAME_SIZE - 1);
    info.format.type = HAILO_FORMAT_TYPE_FLOAT32;
    info.shape.height = h;
    info.shape.width = w;
    info.shape.features = f;
    return info;
}

// Boxes of one colour mask: connected regions larger than min_area pixels.
void Regions(const cv::Mat &mask, int cls, float min_area, std::vector<Box> &out) {
    cv::Mat labels, stats, centroids;
    const int n = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8, CV_32S);
    for (int i = 1; i < n; ++i) {
        const float area = static_cast<float>(stats.at<int>(i, cv::CC_STAT_AREA));
        if (area < min_area) continue;
        const float x = static_cast<float>(stats.at<int>(i, cv::CC_STAT_LEFT));
        const float y = static_cast<float>(stats.at<int>(i, cv::CC_STAT_TOP));
        const float w = static_cast<float>(stats.at<int>(i, cv::CC_STAT_WIDTH));
        const float h = static_cast<float>(stats.at<int>(i, cv::CC_STAT_HEIGHT));
        // Fill ratio as the confidence: a solid sign face fills its box.
        const float fill = area / std::max(1.0F, w * h);
        const float score = std::min(0.99F, 0.70F + 0.30F * fill);
        out.push_back({cls, x / mask.cols, y / mask.rows, (x + w) / mask.cols, (y + h) / mask.rows,
                       score, (w * h) / (mask.cols * mask.rows)});
    }
}

std::vector<Box> Detect(const cv::Mat &rgb) {
    cv::Mat hsv;
    cv::cvtColor(rgb, hsv, cv::COLOR_RGB2HSV);  // H 0..180, S, V 0..255
    cv::Mat red_lo, red_hi, red, green, magenta;
    // Red sign RGB(238,39,55): H ~177. The orange lines of the mat (H ~12)
    // stay outside the band.
    cv::inRange(hsv, cv::Scalar(0, 120, 60), cv::Scalar(6, 255, 255), red_lo);
    cv::inRange(hsv, cv::Scalar(168, 120, 60), cv::Scalar(180, 255, 255), red_hi);
    red = red_lo | red_hi;
    // Green sign RGB(68,214,44): H ~56.
    cv::inRange(hsv, cv::Scalar(40, 100, 50), cv::Scalar(80, 255, 255), green);
    // Parking marker RGB(255,0,255): H 150.
    cv::inRange(hsv, cv::Scalar(140, 100, 60), cv::Scalar(162, 255, 255), magenta);
    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    for (cv::Mat *m : {&red, &green, &magenta}) cv::morphologyEx(*m, *m, cv::MORPH_OPEN, kernel);

    std::vector<Box> boxes;
    const float min_area = 0.00025F * rgb.cols * rgb.rows;  // ~100 px at 640x640
    Regions(green, 0, min_area, boxes);
    Regions(red, 1, min_area, boxes);
    Regions(magenta, 2, min_area, boxes);
    std::sort(boxes.begin(), boxes.end(), [](const Box &a, const Box &b) { return a.area > b.area; });
    if (static_cast<int>(boxes.size()) > kMaxBoxesTotal) boxes.resize(kMaxBoxesTotal);
    return boxes;
}

}  // namespace

HailoInfer::HailoInfer(const std::string &hef_path, size_t batch, hailo_format_type_t,
                       hailo_format_type_t)
    : batch_size(batch) {
    (void)hef_path;
    const auto in = MakeInfo("twin_colour_detector/input_layer1", kInputSize, kInputSize, 3);
    auto out = MakeInfo("twin_colour_detector/nms", 0, 0, 0);
    out.format.order = HAILO_FORMAT_ORDER_HAILO_NMS_BY_CLASS;
    out.nms_shape.number_of_classes = kClasses;
    out.nms_shape.max_bboxes_per_class = kMaxBoxesPerClass;
    infer_model = std::make_shared<hailort::InferModel>(
        std::vector<hailort::InferModel::InferStream>{hailort::InferModel::InferStream(in)},
        std::vector<hailort::InferModel::InferStream>{hailort::InferModel::InferStream(out)},
        hailort::Hef(std::vector<hailo_vstream_info_t>{in}));
    output_vstream_info_by_name[out.name] = out;
}

HailoInfer::HailoInfer(const std::string &hef_path, const std::string &, size_t batch,
                       hailo_format_type_t input_type, hailo_format_type_t output_type)
    : HailoInfer(hef_path, batch, input_type, output_type) {}

const std::vector<hailort::InferModel::InferStream> &HailoInfer::get_inputs() {
    return infer_model->inputs();
}
const std::vector<hailort::InferModel::InferStream> &HailoInfer::get_outputs() {
    return infer_model->outputs();
}
const std::shared_ptr<hailort::InferModel> HailoInfer::get_infer_model() { return infer_model; }
hailo_3d_image_shape_t HailoInfer::get_model_shape() { return infer_model->inputs()[0].shape(); }
size_t HailoInfer::get_output_vstream_infos_size() { return infer_model->outputs().size(); }

void HailoInfer::infer(
    const InputMap &inputs,
    std::function<void(const hailort::AsyncInferCompletionInfo &,
                       const std::vector<std::pair<uint8_t *, hailo_vstream_info_t>> &,
                       const std::vector<std::shared_ptr<uint8_t>> &)> callback) {
    if (inputs.empty()) return;
    const auto &frames = inputs.begin()->second;
    const hailo_vstream_info_t info = output_vstream_info_by_name.begin()->second;
    std::vector<std::pair<uint8_t *, hailo_vstream_info_t>> outputs;
    std::vector<std::shared_ptr<uint8_t>> guards;
    const size_t bytes = kClasses * (sizeof(float32_t) + kMaxBoxesPerClass * sizeof(hailo_bbox_float32_t));
    for (const cv::Mat &frame : frames) {
        std::shared_ptr<uint8_t> buffer(new uint8_t[bytes](), std::default_delete<uint8_t[]>());
        const std::vector<Box> boxes = frame.empty() ? std::vector<Box>{} : Detect(frame);
        size_t offset = 0;
        for (uint32_t cls = 0; cls < kClasses; ++cls) {
            std::vector<hailo_bbox_float32_t> of_class;
            for (const Box &b : boxes) {
                if (b.cls == static_cast<int>(cls)) of_class.push_back({b.y_min, b.x_min, b.y_max, b.x_max, b.score});
            }
            const float32_t count = static_cast<float32_t>(of_class.size());
            std::memcpy(buffer.get() + offset, &count, sizeof(count));
            offset += sizeof(count);
            for (const auto &bb : of_class) {
                std::memcpy(buffer.get() + offset, &bb, sizeof(bb));
                offset += sizeof(bb);
            }
        }
        outputs.emplace_back(buffer.get(), info);
        guards.push_back(buffer);
    }
    hailort::AsyncInferCompletionInfo completion;
    callback(completion, outputs, guards);
}

void HailoInfer::wait_for_last_job() {}
