// Surprise rule of the WRO 2026 International Final, following the team's
// plan in README section 6.10, as a prototype in the digital twin. It is NOT
// part of the robot program yet; on the robot the same steps go where the
// detection is chosen (postprocess_callback, before traffic_lights is set).
//
// The plan: the base of every sign is 50 mm, so the height-to-base ratio of
// its detection box tells its height. About 1 -> 50 mm sign, about 2 ->
// standard 100 mm sign, about 3 -> 150 mm sign. A modified sign (50 or 150 mm)
// has the opposite colour of the original one, so "only the colour decision
// changes": the mission receives the opposite colour and its avoidance
// manoeuvres stay as they are.
//
// Here: for the nearest (largest) red or green box of each frame the ratio is
// measured in pixels of the 800 x 600 camera frame (the model input is the
// same frame stretched to 640 x 640, so normalised boxes are converted back).
// Three things were needed on top of the bare ratio; the first run with a
// one-frame decision called standard signs "modified" and the car pushed a
// green sign:
//   1. Only frames where the whole sign is seen vote: the box does not touch
//      the image border, it is above the part of the image taken by the
//      car's own front, it is 14 to 62 px wide (about 0.4 to 1.5 m), and the
//      pixels just under it are the white mat (a sign whose foot is hidden by
//      a wall looks short).
//   2. The base is not taken from the width of the box, because a sign seen
//      at an angle shows two faces and its box is up to 1.41 times wider
//      (a standard sign seen head-on and a tall one seen at 45 deg gave the
//      same ratio). The base in pixels is what 50 mm measures at the distance
//      of the sign, and the distance comes from the row where the foot of the
//      sign touches the mat (camera height and tilt are known). The ratio is
//      then the height of the sign divided by 50 mm, close to the ideal
//      1, 2 and 3 of the plan at any viewing angle. A first attempt used the
//      gyro heading to correct the width: it failed, the heading is not
//      always relative to the walls during the avoidance manoeuvres.
//   3. A sign is standard until 4 of the last 7 voting frames (at least 5
//      frames of the same sign) say otherwise; once called modified it stays
//      so while it is in view, because the mission reads the colour several
//      times during the approach.
// If the sign is modified its class is swapped in the NMS buffer before the
// repository's postprocess_callback reads it.
//
// Limits: ratio below 1.5 -> 50 mm sign, above 2.5 -> 150 mm sign. Checked in
// the twin with tools/ratio_survey.py (see README). On the robot the camera
// height, tilt and angle of view must be measured (WRO_SURPRISE_CAMERA), and
// the limits set from its own frames (WRO_SURPRISE_LIMITS): the team's first
// real frame gave box ratios of about 0.9, 1.4 and 1.7-2.0.
//
// Environment: WRO_SURPRISE_RULE=1 enables it; WRO_SURPRISE_LIMITS="1.5,2.5";
// WRO_SURPRISE_CAMERA="164,10,86" (lens height in mm, tilt down in deg,
// horizontal angle of view of the frame in deg).
#include "hailo/hailort.h"

#include <opencv2/imgproc.hpp>

#include <chrono>
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace wro_sim {

namespace {

constexpr int kClasses = 3;            // 0 green, 1 red, 2 parking marker (class id = index + 1)
constexpr int kMaxPerClass = 100;
constexpr int kVotes = 7;              // frames in the vote
constexpr int kMinFrames = 5;          // frames of the same sign before any decision
constexpr int kNeeded = 4;             // votes needed to call the sign modified
constexpr float kMinWidthPx = 14.0F;   // narrower: too far, one pixel changes the ratio
constexpr float kMaxWidthPx = 62.0F;   // wider: too close, the car's front hides the foot
constexpr float kMaxBottom = 0.76F;    // below this the image shows the car's own front
constexpr double kMatGray = 110.0;     // mean grey under the box: white mat is above

struct Limits {
    float short_max = 1.5F;      // ratio (sign height / 50 mm base) below: 50 mm sign
    float standard_max = 2.5F;   // below: 100 mm sign; above: 150 mm sign
    float cam_height_mm = 164.0F;  // lens above the mat (CAD)
    float cam_tilt_deg = 10.0F;    // optical axis below the horizontal (CAD)
    float hfov_deg = 86.0F;        // horizontal angle of view of the camera frame
    bool enabled = false;
};

const Limits &Get_Limits() {
    static Limits l = [] {
        Limits x;
        const char *on = std::getenv("WRO_SURPRISE_RULE");
        x.enabled = on != nullptr && std::string(on) == "1";
        if (const char *lim = std::getenv("WRO_SURPRISE_LIMITS")) {
            std::sscanf(lim, "%f,%f", &x.short_max, &x.standard_max);
        }
        if (const char *cam = std::getenv("WRO_SURPRISE_CAMERA")) {
            std::sscanf(cam, "%f,%f,%f", &x.cam_height_mm, &x.cam_tilt_deg, &x.hfov_deg);
        }
        if (x.enabled) {
            std::printf("Surprise rule ON (team plan, README 6.10): height/base < %.2f -> 50 mm, > %.2f -> 150 mm\n",
                        x.short_max, x.standard_max);
        }
        return x;
    }();
    return l;
}

struct Track {
    std::deque<int> heights;   // 50, 100 or 150 for the last frames
    float cx = -1, cy = -1, area = 0;
    int colour = -1;
    std::chrono::steady_clock::time_point seen;
    int decided = 100;
};
Track g_track;

int Classify(float ratio, const Limits &l) {
    if (ratio < l.short_max) return 50;
    if (ratio < l.standard_max) return 100;
    return 150;
}

}  // namespace

bool Surprise_Enabled() { return Get_Limits().enabled; }

// Called with every inference result, before postprocess_callback.
// Height of a sign in mm from its box: the foot row gives the distance along
// the floor, the top row then gives the height (pinhole camera, lens height
// and tilt known). The top of the box is the far top edge of the sign, about
// 60 mm behind its foot. Returns the forward distance in mm through dist_mm.
float Sign_Height_Mm(float y_top_px, float y_bottom_px, int frame_w, int frame_h, const Limits &l,
                     float *dist_mm) {
    const double f = (frame_w / 2.0) / std::tan(l.hfov_deg * M_PI / 360.0);
    const double tilt = l.cam_tilt_deg * M_PI / 180.0;
    const double below_bottom = tilt + std::atan((y_bottom_px - frame_h / 2.0) / f);
    const double below_top = tilt + std::atan((y_top_px - frame_h / 2.0) / f);
    if (below_bottom <= 0.02) {   // foot at or above the horizon: no distance
        *dist_mm = 0;
        return 0;
    }
    const double d = l.cam_height_mm / std::tan(below_bottom);
    *dist_mm = static_cast<float>(d);
    return static_cast<float>(l.cam_height_mm - (d + 60.0) * std::tan(below_top));
}

void Surprise_Process(cv::Mat &frame, const std::vector<std::pair<uint8_t *, hailo_vstream_info_t>> &outputs) {
    const Limits &lim = Get_Limits();
    if (!lim.enabled || outputs.empty() || frame.empty()) return;
    uint8_t *data = outputs[0].first;

    // Read the NMS-by-class buffer.
    std::vector<hailo_bbox_float32_t> cls[kClasses];
    size_t offset = 0;
    for (int c = 0; c < kClasses; ++c) {
        float32_t count;
        std::memcpy(&count, data + offset, sizeof(count));
        offset += sizeof(count);
        for (int i = 0; i < static_cast<int>(count); ++i) {
            hailo_bbox_float32_t b;
            std::memcpy(&b, data + offset, sizeof(b));
            offset += sizeof(b);
            cls[c].push_back(b);
        }
    }

    // Nearest sign = largest red or green box with the score the mission uses.
    int best_c = -1, best_i = -1;
    float best_area = 0;
    for (int c = 0; c < 2; ++c) {
        for (size_t i = 0; i < cls[c].size(); ++i) {
            const auto &b = cls[c][i];
            const float area = (b.x_max - b.x_min) * (b.y_max - b.y_min);
            if (b.score >= 0.75F && area > best_area) {
                best_area = area;
                best_c = c;
                best_i = static_cast<int>(i);
            }
        }
    }
    const auto now = std::chrono::steady_clock::now();
    if (best_c < 0) {
        if (now - g_track.seen > std::chrono::milliseconds(500)) g_track = Track{};
        return;
    }
    const auto b = cls[best_c][best_i];
    const float w_px = (b.x_max - b.x_min) * frame.cols;
    const float h_px = (b.y_max - b.y_min) * frame.rows;
    const float cx = (b.x_min + b.x_max) / 2, cy = (b.y_min + b.y_max) / 2;
    // Ratio of the plan, with the 50 mm base taken at the distance of the sign.
    float dist_mm = 0;
    const float height_mm = Sign_Height_Mm(b.y_min * frame.rows, b.y_max * frame.rows, frame.cols, frame.rows,
                                           lim, &dist_mm);
    const float ratio = height_mm / 50.0F;
    (void)h_px;
    // Does this frame show the whole sign? Otherwise it does not vote.
    const cv::Rect r(cv::Point(static_cast<int>(b.x_min * frame.cols), static_cast<int>(b.y_min * frame.rows)),
                     cv::Point(static_cast<int>(b.x_max * frame.cols), static_cast<int>(b.y_max * frame.rows)));
    bool cut = b.x_min < 0.005F || b.y_min < 0.005F || b.x_max > 0.995F || b.y_max > kMaxBottom ||
               w_px < kMinWidthPx || w_px > kMaxWidthPx || dist_mm <= 0;
    if (!cut) {
        const int strip_h = std::max(3, r.height / 8);
        const cv::Rect under = cv::Rect(r.x, r.y + r.height + 2, std::max(1, r.width), strip_h) &
                               cv::Rect(0, 0, frame.cols, frame.rows);
        if (under.area() > 0) {
            cv::Mat gray;
            cv::cvtColor(frame(under), gray, cv::COLOR_BGR2GRAY);
            if (cv::mean(gray)[0] < kMatGray) cut = true;   // foot hidden (wall, car, another object)
        }
    }

    // Same sign as before? (same colour, centre close, seen recently)
    // (same colour, seen recently, centre and size close to the last frame: the
    // next sign of the section is further away, so its box is much smaller)
    const bool same = g_track.colour == best_c && now - g_track.seen < std::chrono::milliseconds(700) &&
                      std::fabs(cx - g_track.cx) < 0.35F && std::fabs(cy - g_track.cy) < 0.30F &&
                      best_area > 0.5F * g_track.area && best_area < 2.0F * g_track.area;
    if (!same) g_track = Track{};
    g_track.colour = best_c;
    g_track.cx = cx;
    g_track.cy = cy;
    g_track.area = best_area;
    g_track.seen = now;
    if (!cut) {
        g_track.heights.push_back(Classify(ratio, lim));
        if (g_track.heights.size() > static_cast<size_t>(kVotes)) g_track.heights.pop_front();
    }
    int n50 = 0, n100 = 0, n150 = 0;
    for (int h : g_track.heights) (h == 50 ? n50 : h == 100 ? n100 : n150)++;
    // Standard until there is enough evidence; once a sign is called
    // modified it stays so while it is in view (the mission reads the colour
    // at a few moments of the approach, and the close frames, where the top
    // of the sign shows, are the least reliable).
    int decided = g_track.decided;
    if (decided == 100 && static_cast<int>(g_track.heights.size()) >= kMinFrames) {
        if (n50 >= kNeeded) decided = 50;
        if (n150 >= kNeeded) decided = 150;
    }
    if (decided != g_track.decided) {
        std::printf("[surprise] %s sign, box ratio %.2f (votes 50:%d 100:%d 150:%d) -> %d mm%s\n",
                    best_c == 1 ? "red" : "green", ratio, n50, n100, n150, decided,
                    decided == 100 ? "" : (best_c == 1 ? ": MODIFIED, passed as GREEN" : ": MODIFIED, passed as RED"));
        g_track.decided = decided;
    }

    // Label on the camera frame (drawn before the repository draws its boxes).
    char text[96];
    if (cut) {
        std::snprintf(text, sizeof(text), "(no vote) -> %d mm", decided);
    } else {
        std::snprintf(text, sizeof(text), "h/b %.2f -> %d mm", ratio, decided);
    }
    const cv::Scalar col = decided == 100 ? cv::Scalar(255, 255, 255) : cv::Scalar(0, 255, 255);
    cv::putText(frame, text, cv::Point(r.x, std::min(frame.rows - 8, r.y + r.height + 22)),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 4, cv::LINE_AA);
    cv::putText(frame, text, cv::Point(r.x, std::min(frame.rows - 8, r.y + r.height + 22)),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, col, 2, cv::LINE_AA);
    if (decided == 100) return;
    const char *as = best_c == 1 ? "MODIFIED -> pass as GREEN" : "MODIFIED -> pass as RED";
    cv::putText(frame, as, cv::Point(r.x, std::min(frame.rows - 8, r.y + r.height + 46)),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 4, cv::LINE_AA);
    cv::putText(frame, as, cv::Point(r.x, std::min(frame.rows - 8, r.y + r.height + 46)),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);

    // Swap the colour of the modified sign: move its box to the other class.
    const auto moved = cls[best_c][best_i];
    cls[best_c].erase(cls[best_c].begin() + best_i);
    cls[1 - best_c].push_back(moved);
    offset = 0;
    for (int c = 0; c < kClasses; ++c) {
        const int n = std::min<int>(static_cast<int>(cls[c].size()), kMaxPerClass);
        const float32_t count = static_cast<float32_t>(n);
        std::memcpy(data + offset, &count, sizeof(count));
        offset += sizeof(count);
        for (int i = 0; i < n; ++i) {
            std::memcpy(data + offset, &cls[c][i], sizeof(hailo_bbox_float32_t));
            offset += sizeof(hailo_bbox_float32_t);
        }
    }
}

}  // namespace wro_sim
