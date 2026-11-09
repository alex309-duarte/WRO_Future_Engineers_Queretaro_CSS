// Obstacle Challenge mission program (WRO Future Engineers, design v7, Raspberry Pi 5).
// Perception: Camera Module 3 Wide frames go through a YOLOv8n model on the Hailo-8
// accelerator, which detects the red/green traffic signs (pillars) and the magenta
// parking-lot markers; the Slamtec RPLidar S2L (360 deg, 10 Hz) gives wall distances
// and wall orientation (Select_Wall / Slope / Distance_To_Wall).
// Actuation: the Hiwonder ROS Robot Controller board (drive motor, HX-35H steering
// servo, on-board IMU for heading) through the SPIKE-compatible API of spike.h.
// main() starts the lidar reader thread and the camera -> Hailo -> postprocess
// pipeline, then the mission thread (Obstacle_Challenge_Thread), which waits for the
// start button, detects the driving direction, leaves the parking lot, drives 3 laps
// (4 sections per lap; red pillars are passed on the right, green on the left) and
// finally parallel-parks.
// This file is object_detection.cpp of the design-v6 program
// (object_detection_original_h10, the national-final mission with parking) with the
// LEGO SPIKE hub replaced by the Hiwonder layer already proven on the track by the
// Open Challenge (src/ondevice). Changes versus that original are marked
// "Hiwonder:"; the mission logic is unchanged. See README.md and ORIGEN.md.
#include "toolbox.hpp"
#include "hailo_infer.hpp"
#include "utils.hpp"

#include "spike.h"
#include "reto2_config.h"
#include "rasp_gpio.h"
#include "Oradar_S2L.h"
#include "common_var.h"

#include <cstdlib>
#include <string>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <unistd.h>

// Most relevant pillar detection of the latest camera frame, written by
// postprocess_callback (post-process thread) and read by the mission thread.
// middle_point_x/y are the box centre in normalised image coordinates (0..1),
// area is the normalised box area (0..1), confidence is the YOLO score.
struct traffic_lights_struct{
    float  middle_point_x;
    float  middle_point_y;
    Color_traffic_light light_color; /* red = 2, green = 1, xparking (parking marker) = 3, none = 0 */
    float area;
    float confidence;
};

static traffic_lights_struct traffic_lights;

// Set from args.no_display in main() so postprocess_callback (which doesn't receive
// CommandLineArgs) can skip opening the lidar visualization window too.
static bool g_no_display = false;
// Enabled only by the separate terminal-run demo binary/service.
static bool g_fusion_demo_mode = false;

// writer: lidar reader thread (fills the 360-value buffer).
// Main_Actions: mission thread (Obstacle_Challenge_Thread).
pthread_t writer;
pthread_t Main_Actions;
static bool g_main_actions_started = false;
// Hiwonder: WRO_SOLO_PERCEPCION=1 starts camera, Hailo and lidar without opening
// the board or creating the mission (to test perception without moving anything).
static bool g_solo_percepcion = false;


// segment_id -> persistent color slot for the most recently detected walls, refreshed
// every new lidar scan in postprocess_callback. Global so other code can inspect which
// color slot a given wall segment currently holds.
std::unordered_map<int, int> wall_rank;

// Forward declarations. Mission steps are defined in the second half of this file.
void signal_handler(int signum);
void *Obstacle_Challenge_Thread(void *arg);
static void *Obstacle_Challenge_Impl(void *arg);
void Follow_cubes(int vel, float referencia_2, float area);
float Slope(const direction side);
float Distance_To_Wall(const direction side);
void Print_Slope(const direction side);
void Follow_Wall(const direction side);
float calculte_angle_section_start_clockwise(Color_traffic_light traffic_light_color, int cube_number_per_section);
float calculte_angle_section_start_counterclockwise(Color_traffic_light traffic_light_color, int cube_number_per_section);
static float Fit_Line_Orientation(const std::vector<cv::Point2f> &pts, const std::vector<int> &indices);
void avoid_cube_start_section(Color_traffic_light traffic_light_color, Cube_number cube_number_per_section);

// Clockwise mission steps. Each one returns the colour of the last pillar seen or
// passed, so the next step can plan its path:
//   esquivar_cubos_1 / esquivar_cubos_2 - pass the first / second pillar of a section
//   Corner_Case     - drive into the corner, check for a pillar there, turn 90 deg
//   esquivar_cubos_middle - pass a single pillar in the middle position of a section
//   Desicion        - re-zero the heading on the wall and choose which of the above to run
//   estacionamiento_clockwise - leave the parking lot at the start, read the first pillar
// `parking = true` marks the section that contains the parking lot.
Color_traffic_light esquivar_cubos_1(bool parking = false);
Color_traffic_light esquivar_cubos_2( Color_traffic_light past_cube,  bool parking = false);
void calculte_angle_section_start_clockwise_chr(Color_traffic_light traffic_light_color, Cube_number_chr cube_number_per_section, float *angle, float *hypotenuse, bool parking = false);
Color_traffic_light Corner_Case(Color_traffic_light past_cube, bool *middle_cube, bool parking = false);
Color_traffic_light esquivar_cubos_middle(bool parking = false);
Color_traffic_light Desicion(Color_traffic_light past_cube, bool middle_cube, bool parking = false);
Color_traffic_light estacionamiento_clockwise(void);
void Advance_For_distance_Especial(int speed, int distance, int reference);

// Counterparts for driving the lap counterclockwise. Turns and heading correction
// use the lidar on the RIGHT side (Slope(right), diStanceToRightWallmm), the
// opposite of the clockwise versions, which use the left side. Inside
// calculte_angle_section_start_counterclockwise_chr, green and red use the formula
// that the clockwise version uses for the opposite colour: now that the right wall
// is measured instead of the left one, the colour whose passing side matches the
// measured side ("direct" formula) is red instead of green. The rule "green is
// passed on the left, red on the right" (direction_to_turn) is a fixed traffic
// rule and is NOT inverted with the driving direction.
void calculte_angle_section_start_counterclockwise_chr(Color_traffic_light traffic_light_color, Cube_number_chr cube_number_per_section, float *angle, float *hypotenuse, bool parking = false);
Color_traffic_light esquivar_cubos_1_counterclockwise(bool parking = false);
Color_traffic_light esquivar_cubos_2_counterclockwise(Color_traffic_light past_cube, bool parking = false);
Color_traffic_light Corner_Case_counterclockwise(Color_traffic_light past_cube, bool *middle_cube, bool parking = false);
Color_traffic_light esquivar_cubos_middle_counterclockwise(bool parking = false);
Color_traffic_light Desicion_counterclockwise(Color_traffic_light past_cube, bool middle_cube, bool parking = false);
Color_traffic_light estacionamiento_counterclockwise(void);

// Holds the window points and the chosen wall's indices into them, shared by Slope()
// and Distance_To_Wall() so both report on exactly the same wall-selection result
// instead of independently re-running (and potentially disagreeing on) the selection.
// Defined up here (rather than next to Select_Wall's definition) so postprocess_callback
// can call Select_Wall directly to draw each side's wall independently.
struct WallSelection {
    bool found = false;
    std::vector<cv::Point2f> pts;
    std::vector<int> chosen_indices;
    std::vector<int> used_angles; // lidar angles (0-359) backing chosen_indices
};
static WallSelection Select_Wall(const direction side);

// Returns the index of the largest value in arr[0..n-1] (first one on ties), or -1
// if n <= 0. Used to pick the detection box with the largest area.
int find_max_index(float arr[], int n) {
    if (n <= 0) return -1; // Handle empty array case

    int max_index = 0;
    float max_value = arr[0];

    for (int i = 1; i < n; i++) {
        if (arr[i] > max_value) {
            max_value = arr[i];
            max_index = i;
        }
    }
    return max_index;
}

// One counter shared by Save_Lidar_Debug_Snapshot and Save_Wall_Detection_Snapshot so a
// single call site can log the raw scan and the algorithm's wall-detection outcome for
// that same scan under a matching snapshot_id, and the two can be joined later.
static int Next_Debug_Snapshot_Id() {
    static int next_id = 0;
    return next_id++;
}

// Wall-clock time as "YYYY-MM-DD HH:MM:SS.mmm" for the debug CSV logs.
static std::string Debug_Timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    char time_buf[32];
    struct tm tm_info;
    localtime_r(&ts.tv_sec, &tm_info);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    char full_buf[48];
    snprintf(full_buf, sizeof(full_buf), "%s.%03ld", time_buf, ts.tv_nsec / 1000000);
    return full_buf;
}

// Appends one full 360-value lidar scan to a debug CSV (snapshot_id, timestamp,
// angle_deg, range_mm), one row per angle, so a snapshot from a specific call site
// (e.g. right after the settling usleep in
// calculte_angle_section_start_counterclockwise_chr) can be inspected later when
// tracking down misreads like front-wall outliers. snapshot_id increments once per
// call (not per row), so all 360 rows from the same call can be grouped/filtered
// without having to match on the timestamp string.
static void Save_Lidar_Debug_Snapshot(int snapshot_id, const float lidar_buffer[360], const char *tag) {
    const char *path_env = std::getenv("WRO_LIDAR_DEBUG_LOG");
    const std::string path = path_env ? path_env : "/home/maker/lidar_debug_log.csv";

    FILE *f = fopen(path.c_str(), "a");
    if (!f) return;

    const std::string time_str = Debug_Timestamp();
    for (int angle = 0; angle < 360; angle++) {
        fprintf(f, "%d,%s,%s,%d,%.1f\n",
                snapshot_id, time_str.c_str(), tag, angle, lidar_buffer[angle]);
    }
    fclose(f);
}

// Appends this same scan's Select_Wall() outcome for all four sides (found, distance,
// orientation, point count) to a companion debug CSV, tagged with the same
// snapshot_id as Save_Lidar_Debug_Snapshot's raw dump - so the algorithm's conclusion
// can be compared side-by-side against the raw points that produced it, e.g. to see
// whether a front-wall outlier point actually got pulled into the chosen wall segment.
static void Save_Wall_Detection_Snapshot(int snapshot_id, const char *tag) {
    const char *path_env = std::getenv("WRO_WALL_DEBUG_LOG");
    const std::string path = path_env ? path_env : "/home/maker/lidar_wall_debug_log.csv";

    FILE *f = fopen(path.c_str(), "a");
    if (!f) return;

    const std::string time_str = Debug_Timestamp();
    struct SideInfo { direction side; const char *name; };
    static const SideInfo sides[] = {
        { front,  "front"  },
        { right,  "right"  },
        { left,   "left"   },
        { behind, "behind" },
    };
    for (const auto &s : sides) {
        WallSelection sel = Select_Wall(s.side);
        if (!sel.found) {
            fprintf(f, "%d,%s,%s,%s,0,,,0\n", snapshot_id, time_str.c_str(), tag, s.name);
            continue;
        }
        float orientation_deg = Fit_Line_Orientation(sel.pts, sel.chosen_indices);
        float orientation_rad = Oradar_S2L_Degrees_To_Radians(orientation_deg);
        cv::Point2f dir(cos(orientation_rad), sin(orientation_rad));
        cv::Point2f centroid(0, 0);
        for (int idx : sel.chosen_indices) centroid += sel.pts[idx];
        centroid *= (1.0f / (float)sel.chosen_indices.size());
        float distance_mm = std::fabs(centroid.x * dir.y - centroid.y * dir.x);
        fprintf(f, "%d,%s,%s,%s,1,%.1f,%.2f,%zu\n",
                snapshot_id, time_str.c_str(), tag, s.name,
                distance_mm, orientation_deg, sel.chosen_indices.size());
    }
    fclose(f);
}

// Recursively splits a contiguous wall segment at its point of maximum perpendicular
// deviation from the straight line joining its endpoints (Iterative End-Point Fit /
// split-and-merge). This catches corners that a gap-based breakpoint detector misses:
// two walls meeting at a corner have no range discontinuity between consecutive points,
// only a change in orientation, so a pure distance-gap check sees them as one wall.
void split_segment_at_corners(const std::vector<cv::Point2f> &points_mm,
                               const std::vector<int> &indices,
                               float threshold_mm,
                               int corner_margin_points,
                               std::vector<std::vector<int>> &out_segments)
{
    if (indices.size() < 3) {
        out_segments.push_back(indices);
        return;
    }

    const cv::Point2f &p1 = points_mm[indices.front()];
    const cv::Point2f &p2 = points_mm[indices.back()];
    cv::Point2f line_dir = p2 - p1;
    float line_len = (float)cv::norm(line_dir);

    float max_dist = 0;
    size_t split_at = 0;
    for (size_t k = 1; k + 1 < indices.size(); k++) {
        const cv::Point2f &p = points_mm[indices[k]];
        float dist = (line_len < 1e-3f)
            ? (float)cv::norm(p - p1)
            : std::fabs(line_dir.cross(p - p1)) / line_len;
        if (dist > max_dist) {
            max_dist = dist;
            split_at = k;
        }
    }

    if (max_dist > threshold_mm && split_at > 0) {
        // Drop a few points straddling the corner vertex: lidar returns right at a
        // corner are noisy/transitional and otherwise tend to form their own tiny
        // spurious "wall" instead of cleanly belonging to either side.
        size_t left_end = (split_at >= (size_t)corner_margin_points) ? split_at - corner_margin_points : 0;
        size_t right_start = std::min(split_at + (size_t)corner_margin_points, indices.size() - 1);
        std::vector<int> left(indices.begin(), indices.begin() + left_end + 1);
        std::vector<int> right(indices.begin() + right_start, indices.end());
        split_segment_at_corners(points_mm, left, threshold_mm, corner_margin_points, out_segments);
        split_segment_at_corners(points_mm, right, threshold_mm, corner_margin_points, out_segments);
    } else {
        out_segments.push_back(indices);
    }
}

using namespace hailo_utils;
using Clock = std::chrono::steady_clock;

namespace fs = std::filesystem;

/////////// Constants ///////////
constexpr size_t MAX_QUEUE_SIZE = 60;

std::shared_ptr<BoundedTSQueue<std::pair<std::vector<cv::Mat>, std::vector<cv::Mat>>>> preprocessed_batch_queue =
    std::make_shared<BoundedTSQueue<std::pair<std::vector<cv::Mat>, std::vector<cv::Mat>>>>(MAX_QUEUE_SIZE);
 
std::shared_ptr<BoundedTSQueue<InferenceResult>> results_queue =
    std::make_shared<BoundedTSQueue<InferenceResult>>(MAX_QUEUE_SIZE);

// Task-specific preprocessing callback (runs in the preprocess thread): converts each
// camera frame to a contiguous RGB image of the model input size
// (target_width x target_height px) expected by the YOLOv8n network on the Hailo-8.
void preprocess_callback(const std::vector<cv::Mat>& org_frames,
                         std::vector<cv::Mat>& preprocessed_frames,
                         uint32_t target_width, uint32_t target_height)
{
    preprocessed_frames.clear();
    preprocessed_frames.reserve(org_frames.size());

    for (const auto &src_bgr : org_frames) {
        // Skip invalid frames but keep vector alignment (optional: push empty)
        if (src_bgr.empty()) {
            preprocessed_frames.emplace_back();
            continue;
        }
        cv::Mat rgb;
        // 1) Convert to RGB
        if (src_bgr.channels() == 3) {
            cv::cvtColor(src_bgr, rgb, cv::COLOR_BGR2RGB);
        } else if (src_bgr.channels() == 4) {
            // If someone passed BGRA, drop alpha
            cv::cvtColor(src_bgr, rgb, cv::COLOR_BGRA2RGB);
        } else if (src_bgr.channels() == 1) {
            // If grayscale sneaks in, promote to 3 channels
            cv::cvtColor(src_bgr, rgb, cv::COLOR_GRAY2RGB);
        } else {
            // Fallback: force 3 channels by duplicating/merging
            std::vector<cv::Mat> ch(3, src_bgr);
            cv::merge(ch, rgb);
            cv::cvtColor(rgb, rgb, cv::COLOR_BGR2RGB); // ensure RGB order
        }
        // 2) Resize to target
        if (rgb.cols != static_cast<int>(target_width) || rgb.rows != static_cast<int>(target_height)) {
            cv::resize(rgb, rgb, cv::Size(static_cast<int>(target_width),
                                          static_cast<int>(target_height)),
                       0.0, 0.0, cv::INTER_AREA);
        }
        // 3) Ensure contiguous buffer
        if (!rgb.isContinuous()) {
            rgb = rgb.clone();
        }
        // 4) Push to output vector
        preprocessed_frames.push_back(std::move(rgb));
    }
}

// Task-specific postprocessing callback (runs in the post-process thread once per
// inference result):
//  1. parses the Hailo NMS output (3 classes: green, red, parking marker) and draws
//     the boxes on the frame;
//  2. when a new lidar scan has arrived, segments it into walls (outlier removal,
//     adaptive breakpoints, corner splitting, frame-to-frame tracking) and draws the
//     "Lidar Coordinates" debug window, highlighting the walls Select_Wall() picks;
//  3. stores the largest confident red/green box in the global traffic_lights, which
//     the mission thread reads.
void postprocess_callback(
    cv::Mat &frame_to_draw,
    const std::vector<std::pair<uint8_t*, hailo_vstream_info_t>> &output_data_and_infos,
    const VisualizationParams &vis)
{
    const size_t class_count = 3;
    float traffic_light_area[10] = {0};

    auto bboxes = parse_nms_data(output_data_and_infos[0].first, class_count);

    draw_bounding_boxes(frame_to_draw, bboxes, vis);

    // Show lidar points in their own window (mm scaled down to pixels), redrawn only when a new scan arrives
    static unsigned long last_lidar_seq = 0;
    unsigned long current_lidar_seq = Oradar_S2L_Get_Scan_Seq();
    if (current_lidar_seq != last_lidar_seq) {
        last_lidar_seq = current_lidar_seq;

        const int canvas_size = 1080;
        const float lidar_scale = 0.15f; // pixels per mm
        const int grid_spacing_mm = 500;
        const cv::Point origin(canvas_size / 2, canvas_size / 2);
        cv::Mat lidar_canvas(canvas_size, canvas_size, CV_8UC3, cv::Scalar(255, 255, 255));
        const cv::Scalar grid_color(180, 180, 180);
        const cv::Scalar axis_color(60, 60, 60);

        // Parallel grid lines every grid_spacing_mm, labeled in mm
        int max_offset_mm = (int)((canvas_size / 2) / lidar_scale);
        for (int offset_mm = grid_spacing_mm; offset_mm <= max_offset_mm; offset_mm += grid_spacing_mm) {
            int offset_px = (int)(offset_mm * lidar_scale);

            cv::line(lidar_canvas, cv::Point(origin.x + offset_px, 0), cv::Point(origin.x + offset_px, canvas_size), grid_color, 1);
            cv::line(lidar_canvas, cv::Point(origin.x - offset_px, 0), cv::Point(origin.x - offset_px, canvas_size), grid_color, 1);
            cv::line(lidar_canvas, cv::Point(0, origin.y + offset_px), cv::Point(canvas_size, origin.y + offset_px), grid_color, 1);
            cv::line(lidar_canvas, cv::Point(0, origin.y - offset_px), cv::Point(canvas_size, origin.y - offset_px), grid_color, 1);

            cv::putText(lidar_canvas, std::to_string(offset_mm), cv::Point(origin.x + offset_px + 2, origin.y - 2),
                        cv::FONT_HERSHEY_PLAIN, 0.8, grid_color, 1);
            cv::putText(lidar_canvas, std::to_string(offset_mm), cv::Point(origin.x - offset_px + 2, origin.y - 2),
                        cv::FONT_HERSHEY_PLAIN, 0.8, grid_color, 1);
        }

        // Crosshair axes drawn last so they stand out over the grid
        cv::line(lidar_canvas, cv::Point(origin.x, 0), cv::Point(origin.x, canvas_size), axis_color, 1);
        cv::line(lidar_canvas, cv::Point(0, origin.y), cv::Point(canvas_size, origin.y), axis_color, 1);

        // Mark the 0 degree direction (lidar angle index 0, along +x) distinctly from the plain axes
        const cv::Scalar zero_deg_color(0, 0, 255); // red
        cv::line(lidar_canvas, origin, cv::Point(canvas_size, origin.y), zero_deg_color, 2);
        cv::putText(lidar_canvas, "0deg", cv::Point(canvas_size - 60, origin.y - 8),
                    cv::FONT_HERSHEY_PLAIN, 1.0, zero_deg_color, 1);

        // Gather valid points (range + angle + cartesian mm) for ROR + breakpoint detection
        float lidar_buffer[360];
        Oradar_S2L_Get_Buffer(&lidar_buffer[0]);
        std::vector<cv::Point2f> points_mm;
        std::vector<float> point_range_mm;
        std::vector<float> point_angle_deg;
        points_mm.reserve(360);
        point_range_mm.reserve(360);
        point_angle_deg.reserve(360);
        for (int angle = 0; angle < 360; angle++) {
            if (lidar_buffer[angle] <= 0) continue;
            float x_mm = lidar_buffer[angle] * cos(Oradar_S2L_Degrees_To_Radians((float)angle));
            float y_mm = lidar_buffer[angle] * sin(Oradar_S2L_Degrees_To_Radians((float)angle));
            points_mm.emplace_back(x_mm, y_mm);
            point_range_mm.push_back(lidar_buffer[angle]);
            point_angle_deg.push_back((float)angle);
        }

        // Radius Outlier Removal (ROR): a point is an outlier if it has fewer than
        // ror_min_neighbors other points within ror_radius_mm of it.
        const float ror_radius_mm = 100.0f;
        const int   ror_min_neighbors = 10;
        std::vector<bool> is_outlier(points_mm.size(), false);
        for (size_t i = 0; i < points_mm.size(); i++) {
            int neighbor_count = 0;
            for (size_t j = 0; j < points_mm.size(); j++) {
                if (i == j) continue;
                if (cv::norm(points_mm[i] - points_mm[j]) <= ror_radius_mm) {
                    neighbor_count++;
                    if (neighbor_count >= ror_min_neighbors) break;
                }
            }
            is_outlier[i] = (neighbor_count < ror_min_neighbors);
        }

        // Adaptive Breakpoint Detector (Borges & Aldon): walk the ordered inlier points
        // and start a new wall segment whenever the gap to the previous point exceeds
        // the spacing expected for two returns off the same flat surface.
        // A smaller lambda shrinks sin(lambda-delta_phi) in D_max's denominator below,
        // which makes D_max *larger* - i.e. more tolerant of gaps, less likely to split.
        // At 5 deg, D_max for a ~450 mm-range point (1 deg step) is ~142 mm: so generous
        // that a real object sitting near a wall would not be split off as its own
        // segment. At the 10 deg used here it is ~80 mm.
        const float bpd_lambda_deg = 10.0f;  // min incidence angle assumed for a flat surface
        const float bpd_sigma_r_mm = 10.0f;  // range measurement noise estimate
        const float lambda_rad = Oradar_S2L_Degrees_To_Radians(bpd_lambda_deg);

        std::vector<int> segment_id(points_mm.size(), -1);
        int next_segment_id = 0;
        int prev_inlier_idx = -1;
        for (size_t i = 0; i < points_mm.size(); i++) {
            if (is_outlier[i]) continue;
            if (prev_inlier_idx >= 0) {
                float delta_phi_rad = Oradar_S2L_Degrees_To_Radians(point_angle_deg[i] - point_angle_deg[prev_inlier_idx]);
                float r_i = point_range_mm[prev_inlier_idx];
                float d_max = (delta_phi_rad >= lambda_rad)
                    ? -1.0f
                    : r_i * (sin(delta_phi_rad) / sin(lambda_rad - delta_phi_rad)) + 3.0f * bpd_sigma_r_mm;
                float dist = cv::norm(points_mm[i] - points_mm[prev_inlier_idx]);
                if (d_max < 0 || dist > d_max) {
                    next_segment_id++;
                }
            }
            segment_id[i] = next_segment_id;
            prev_inlier_idx = (int)i;
        }

        // Merge the wrap-around (angle 359 is adjacent to angle 0 on a 2D scan) if the
        // first and last detected segments are actually the same continuous wall.
        if (next_segment_id > 0) {
            int first_inlier = -1, last_inlier = -1;
            for (size_t i = 0; i < points_mm.size(); i++) {
                if (!is_outlier[i]) { first_inlier = (int)i; break; }
            }
            for (int i = (int)points_mm.size() - 1; i >= 0; i--) {
                if (!is_outlier[i]) { last_inlier = i; break; }
            }
            if (first_inlier >= 0 && last_inlier >= 0 && first_inlier != last_inlier) {
                float delta_phi_rad = Oradar_S2L_Degrees_To_Radians(360.0f - point_angle_deg[last_inlier] + point_angle_deg[first_inlier]);
                float r_i = point_range_mm[last_inlier];
                float d_max = (delta_phi_rad >= lambda_rad)
                    ? -1.0f
                    : r_i * (sin(delta_phi_rad) / sin(lambda_rad - delta_phi_rad)) + 3.0f * bpd_sigma_r_mm;
                float dist = cv::norm(points_mm[first_inlier] - points_mm[last_inlier]);
                if (d_max >= 0 && dist <= d_max) {
                    int old_id = segment_id[last_inlier];
                    int new_id = segment_id[first_inlier];
                    for (size_t i = 0; i < segment_id.size(); i++) {
                        if (segment_id[i] == old_id) segment_id[i] = new_id;
                    }
                }
            }
        }

        // Split each gap-based segment further at corners (orientation changes with no
        // range gap), so two walls meeting at a corner end up as distinct segments.
        const float corner_split_threshold_mm = 40.0f;
        const int   corner_margin_points = 2; // points dropped on each side of a detected corner
        {
            std::unordered_map<int, std::vector<int>> segments_by_id;
            for (size_t i = 0; i < segment_id.size(); i++) {
                if (segment_id[i] >= 0) {
                    segments_by_id[segment_id[i]].push_back((int)i);
                    segment_id[i] = -1; // reset; points dropped near a corner stay unclassified (clutter)
                }
            }
            std::vector<std::vector<int>> split_segments;
            for (auto &kv : segments_by_id) {
                split_segment_at_corners(points_mm, kv.second, corner_split_threshold_mm, corner_margin_points, split_segments);
            }
            for (size_t s = 0; s < split_segments.size(); s++) {
                for (int idx : split_segments[s]) {
                    segment_id[idx] = (int)s;
                }
            }
        }

        // Keep only the max_walls largest segments as actual walls; smaller segments
        // are treated as clutter/noise rather than a distinct wall.
        const int max_walls = 8;
        std::unordered_map<int, int> segment_counts;
        for (size_t i = 0; i < segment_id.size(); i++) {
            if (segment_id[i] >= 0) segment_counts[segment_id[i]]++;
        }
        std::vector<std::pair<int, int>> ranked_segments(segment_counts.begin(), segment_counts.end());
        std::sort(ranked_segments.begin(), ranked_segments.end(),
                  [](const auto &a, const auto &b) { return a.second > b.second; });
        if ((int)ranked_segments.size() > max_walls) ranked_segments.resize(max_walls);

        // Centroid (mm) of each candidate wall, used to track its identity across frames
        std::vector<cv::Point2f> candidate_centroid(ranked_segments.size(), cv::Point2f(0, 0));
        for (size_t c = 0; c < ranked_segments.size(); c++) {
            int seg = ranked_segments[c].first;
            cv::Point2f sum(0, 0);
            int count = 0;
            for (size_t i = 0; i < segment_id.size(); i++) {
                if (segment_id[i] == seg) { sum += points_mm[i]; count++; }
            }
            candidate_centroid[c] = sum * (1.0f / std::max(count, 1));
        }

        // Match candidates to persistent color slots by nearest centroid, so the same
        // physical wall keeps the same color across frames instead of flickering when
        // the size-ranking of two similarly sized walls swaps from one scan to the next.
        struct WallTrack { bool valid; cv::Point2f centroid; };
        static std::vector<WallTrack> wall_tracks(max_walls, WallTrack{false, cv::Point2f(0, 0)});
        const float wall_track_max_match_mm = 400.0f;

        std::vector<int> candidate_color(ranked_segments.size(), -1);
        std::vector<bool> track_used(max_walls, false);
        for (;;) {
            float best_dist = wall_track_max_match_mm;
            int best_c = -1, best_t = -1;
            for (size_t c = 0; c < ranked_segments.size(); c++) {
                if (candidate_color[c] != -1) continue;
                for (int t = 0; t < max_walls; t++) {
                    if (track_used[t] || !wall_tracks[t].valid) continue;
                    float d = (float)cv::norm(candidate_centroid[c] - wall_tracks[t].centroid);
                    if (d < best_dist) { best_dist = d; best_c = (int)c; best_t = t; }
                }
            }
            if (best_c < 0) break;
            candidate_color[best_c] = best_t;
            track_used[best_t] = true;
        }
        // Any unmatched candidate (new wall, or moved further than wall_track_max_match_mm) takes a free slot
        for (size_t c = 0; c < ranked_segments.size(); c++) {
            if (candidate_color[c] != -1) continue;
            for (int t = 0; t < max_walls; t++) {
                if (!track_used[t]) { candidate_color[c] = t; track_used[t] = true; break; }
            }
        }
        // Refresh tracks: assigned slots take the new centroid, unused slots go invalid
        for (int t = 0; t < max_walls; t++) wall_tracks[t].valid = false;
        for (size_t c = 0; c < ranked_segments.size(); c++) {
            int t = candidate_color[c];
            if (t >= 0) {
                wall_tracks[t].valid = true;
                wall_tracks[t].centroid = candidate_centroid[c];
            }
        }

        wall_rank.clear(); // segment_id -> persistent color slot (global, see declaration)
        for (size_t c = 0; c < ranked_segments.size(); c++) {
            wall_rank[ranked_segments[c].first] = candidate_color[c];
        }

        // Distinct colors used to render each of the top max_walls detected walls
        static const std::vector<cv::Scalar> segment_palette = {
            cv::Scalar(0, 255, 0),     // green
            cv::Scalar(0, 140, 255),   // orange
            cv::Scalar(0, 0, 255),     // red
            cv::Scalar(255, 0, 255),   // magenta
            cv::Scalar(0, 255, 255),   // yellow
            cv::Scalar(255, 0, 128),   // purple
            cv::Scalar(128, 128, 0),   // teal
            cv::Scalar(0, 100, 0),     // dark green
        };
        const cv::Scalar clutter_color(128, 128, 128); // inlier point not part of a top max_walls wall

        // Independently select and highlight the front, right, and left walls via
        // Select_Wall(), regardless of whichever side Distance_To_Wall()/Slope() were
        // last called with elsewhere in the robot logic - so the lidar frame always
        // shows all three sides at once instead of just whichever one was queried.
        struct SideHighlight { direction side; const char *label; cv::Scalar color; };
        static const std::vector<SideHighlight> sides_to_highlight = {
            { front,  "front",  cv::Scalar(0, 0, 255) },    // red
            { right,  "right",  cv::Scalar(255, 0, 0) },    // blue
            { left,   "left",   cv::Scalar(0, 200, 0) },    // green
            { behind, "behind", cv::Scalar(0, 200, 200) },  // yellow-ish
        };

        bool highlight_mask[360] = {false};
        cv::Scalar highlight_color_by_angle[360];

        for (const auto &sh : sides_to_highlight) {
            WallSelection sel = Select_Wall(sh.side);
            if (!sel.found) continue;

            for (int a : sel.used_angles) {
                if (a >= 0 && a < 360) {
                    highlight_mask[a] = true;
                    highlight_color_by_angle[a] = sh.color;
                }
            }

            // Perpendicular distance from the robot (origin) to this side's fitted wall
            // line, not just the centroid's range, since the centroid can sit off to one
            // side of the perpendicular foot.
            cv::Point2f centroid(0, 0);
            for (int idx : sel.chosen_indices) centroid += sel.pts[idx];
            centroid *= (1.0f / (float)sel.chosen_indices.size());
            float orientation_deg = Fit_Line_Orientation(sel.pts, sel.chosen_indices);
            float orientation_rad = Oradar_S2L_Degrees_To_Radians(orientation_deg);
            cv::Point2f dir(cos(orientation_rad), sin(orientation_rad));
            float distance_mm = std::fabs(centroid.x * dir.y - centroid.y * dir.x);

            cv::Point text_pos = origin + cv::Point((int)(centroid.x * lidar_scale) + 10,
                                                      (int)(centroid.y * lidar_scale) - 10);
            char dist_text[48];
            snprintf(dist_text, sizeof(dist_text), "%s %.0fmm", sh.label, distance_mm);
            cv::putText(lidar_canvas, dist_text, text_pos, cv::FONT_HERSHEY_SIMPLEX, 0.5, sh.color, 2);
        }

        for (size_t i = 0; i < points_mm.size(); i++) {
            cv::Point p = origin + cv::Point((int)(points_mm[i].x * lidar_scale), (int)(points_mm[i].y * lidar_scale));
            if (p.x >= 0 && p.x < lidar_canvas.cols && p.y >= 0 && p.y < lidar_canvas.rows) {
                cv::Scalar color;
                if (is_outlier[i]) {
                    color = cv::Scalar(255, 0, 0); // blue
                } else {
                    auto it = wall_rank.find(segment_id[i]);
                    color = (it != wall_rank.end()) ? segment_palette[it->second] : clutter_color;
                }

                int wrapped_angle = (int)point_angle_deg[i];
                bool highlighted = (wrapped_angle >= 0 && wrapped_angle < 360) && highlight_mask[wrapped_angle];
                if (highlighted) {
                    cv::Scalar hcolor = highlight_color_by_angle[wrapped_angle];
                    cv::circle(lidar_canvas, p, 3, hcolor, -1);
                    cv::circle(lidar_canvas, p, 3, cv::Scalar(0, 0, 0), 1); // black outline so it stands out beyond just color
                } else {
                    cv::circle(lidar_canvas, p, 0.5, color, -1);
                }
            }
        }

        if (!g_no_display) {
            cv::imshow("Lidar Coordinates", lidar_canvas);
        }
    }
    int index = 0;
    for (const auto &named_bbox : bboxes){
        traffic_light_area[index] = (named_bbox.bbox.x_max - named_bbox.bbox.x_min) * (named_bbox.bbox.y_max - named_bbox.bbox.y_min);
        if((named_bbox.bbox.score < 0.75) || ((Color_traffic_light)named_bbox.class_id == light_xparking ) )
        { // discard boxes with score below 0.75 and parking markers (area 0) so they are not picked as the pillar
            traffic_light_area[index] = 0;
        }
        index++;
    }
    int max_index = find_max_index(traffic_light_area, 10);
    traffic_lights.light_color = none;
    traffic_lights.confidence = 0.0f;
    if (bboxes.size() > 0){
        const auto named_bbox = bboxes.at(max_index); // largest remaining box = nearest pillar
        
        traffic_lights.middle_point_x = (named_bbox.bbox.x_min + named_bbox.bbox.x_max) / 2.0;
        traffic_lights.middle_point_y = (named_bbox.bbox.y_min + named_bbox.bbox.y_max) / 2.0;
        traffic_lights.light_color = (Color_traffic_light)named_bbox.class_id;
        traffic_lights.confidence = named_bbox.bbox.score;
        traffic_lights.area = traffic_light_area[max_index];
        //printf(" color = %d, area = %f, middle point = (%f, %f), confidence = %f", traffic_lights.light_color, traffic_lights.area, traffic_lights.middle_point_x, traffic_lights.middle_point_y, traffic_lights.confidence);
        //printf("\n");
    }
    else{
        //printf("color: %d\n",traffic_lights.light_color);
    }
}


// Hiwonder: waits up to timeout_ms (ms) for the camera to report a pillar and returns
// its colour (none on timeout or shutdown). Replaces the original
// "while(cube == none)" loop, which declared ANOTHER variable inside and never
// exited if no pillar was visible at that moment.
static Color_traffic_light Esperar_Color_De_Cubo(int timeout_ms){
    Color_traffic_light cube = traffic_lights.light_color;
    for(int t = 0; (cube == none) && (t < timeout_ms) && (terminating_main == 0); t++){
        usleep(1000);
        cube = traffic_lights.light_color;
    }
    if(cube == none){
        printf("no se vio ningun cubo en %d ms\n", timeout_ms);
    }
    return cube;
}

// Hiwonder: the .yaml files sit next to the executable (CMake copies them there), so
// they are found regardless of the directory the program is launched from.
// Returns <executable dir>/nombre, or nombre itself if /proc/self/exe cannot be read.
static std::string Ruta_Junto_Al_Ejecutable(const char *nombre){
    std::error_code ec;
    const fs::path exe = fs::read_symlink("/proc/self/exe", ec);
    if(ec) return nombre;
    return (exe.parent_path() / nombre).string();
}

// Hiwonder: orderly shutdown. Stops and joins the mission thread, leaves the board
// motors in coast and closes its serial port, releases the GPIO, and joins the lidar
// thread BEFORE freeing the driver it is still reading from (otherwise
// use-after-free).
static void Apagar_Todo(void){
    terminating_main = 1;
    if(g_main_actions_started){
        pthread_join(Main_Actions, NULL);
        g_main_actions_started = false;
    }
    if(spike_compat::Spike_Is_Attached()){
        try { Spike_Coast_Motors(); } catch (...) {}
        Spike_Close_Serial();
    }
    Rasp_Gpio_Clean();
    Oradar_S2L_Set_Terminating();
    pthread_join(writer, NULL);
    Oradar_S2L_Close();
}

// Program entry. Options: --fusion-demo (camera + lidar demo instead of the mission)
// plus the standard Hailo example arguments (network, input, batch size, display...).
// Env: WRO_SOLO_PERCEPCION=1 (perception only), WRO_VISUALIZATION_CONFIG (yaml path).
// Order: lidar thread -> GPIO (start button) -> board init -> wait for the first lidar
// scan -> Hailo pipeline threads -> mission thread. Returns a hailo_status code.
int main(int argc, char** argv)
{

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--fusion-demo") g_fusion_demo_mode = true;
    }

    signal(SIGINT, signal_handler); /* Set interrupt for ctrl+C */
    {
        const char *solo = std::getenv("WRO_SOLO_PERCEPCION");
        g_solo_percepcion = (solo != nullptr && std::string(solo) == "1");
    }

    Oradar_S2L_Init_Lidar();
    pthread_create(&writer, NULL, Oradar_S2L_Lidar_Writer_Thread, NULL);

    if(Rasp_Gpio_Init() != 0){
        Oradar_S2L_Set_Terminating();
        pthread_join(writer, NULL);
        Oradar_S2L_Close();
        return HAILO_INTERNAL_FAILURE;
    }

    // Hiwonder: board instead of the SPIKE hub, no relay and no REPL. Opens the board,
    // installs the emergency stop (Ctrl+C), checks the servo, centres the steering
    // and calibrates the gyro for 5 s with the robot STILL: the same start-up as the
    // Open Challenge plus the Obstacle Challenge scale factors (Spike_Reto2_Init).
    if(!g_solo_percepcion){
        if(Spike_Reto2_Init() != 0){
            Rasp_Gpio_Clean();
            Oradar_S2L_Set_Terminating();
            pthread_join(writer, NULL);
            Oradar_S2L_Close();
            return HAILO_INTERNAL_FAILURE;
        }
    }
    else{
        std::cout << "SOLO PERCEPCION: camara, Hailo y lidar; la placa no se abre y no hay mision.\n";
    }

    // Without lidar data every distance reads 0 and the advance loops treat 0 as
    // "no reading yet": the robot would drive blind. Wait (up to 50 x 100 ms = 5 s)
    // for the first complete scan; if none arrives, exit without moving.
    for(int i = 0; (i < 50) && (Oradar_S2L_Get_Scan_Seq() == 0) && (terminating_main == 0); i++){
        usleep(100000);
    }
    if(Oradar_S2L_Get_Scan_Seq() == 0){
        std::cerr << "ERROR: el lidar no envia datos (5 s sin barridos). No se mueve nada.\n";
        Apagar_Todo();
        return HAILO_INTERNAL_FAILURE;
    }
    {
        float lidar[360];
        Oradar_S2L_Get_Buffer(&lidar[0]);
        printf("Lidar listo: frente %.0f mm, derecha %.0f mm, izquierda %.0f mm\n",
               lidar[RP_TO_ORADAR_IDX(FRONT)], lidar[RP_TO_ORADAR_IDX(RIGHT)], lidar[RP_TO_ORADAR_IDX(LEFT)]);
    }

    try {
        const std::string APP_NAME = "object_detection";
        std::chrono::duration<double> inference_time;
        auto t_start = Clock::now();

        double org_height, org_width;
        cv::VideoCapture capture;
        size_t frame_count;
        InputType input_type;

        CommandLineArgs args = parse_command_line_arguments(argc, argv);
        post_parse_args(APP_NAME, args, argc, argv);
        g_no_display = args.no_display;
        HailoInfer model(args.net, args.batch_size);

        // Load visualization config params
        const char *visualization_config_env = std::getenv("WRO_VISUALIZATION_CONFIG");
        const std::string visualization_config = visualization_config_env
            ? visualization_config_env
            : Ruta_Junto_Al_Ejecutable("visualization_config.yaml");
        VisualizationParams vis_param = load_visualization_params(visualization_config);
        validate_visualization_params(vis_param, AppVisMode::object_detection);

        auto post_cb = std::bind(postprocess_callback,
                                 std::placeholders::_1,
                                 std::placeholders::_2,
                                 std::cref(vis_param));

        input_type = determine_input_type(args.input,
                                        std::ref(capture),
                                        std::ref(org_height),
                                        std::ref(org_width),
                                        std::ref(frame_count),
                                        std::ref(args.batch_size),
                                        std::ref(args.camera_resolution));

        auto preprocess_thread = std::async(run_preprocess,
                                            std::ref(args.input),
                                            std::ref(args.net),
                                            std::ref(model),
                                            std::ref(input_type),
                                            std::ref(capture),
                                            std::ref(args.batch_size),
                                            std::ref(args.framerate),
                                            preprocessed_batch_queue,
                                            preprocess_callback);

        ModelInputQueuesMap input_queues = {
            { model.get_infer_model()->get_input_names().at(0), preprocessed_batch_queue }
        };

        auto inference_thread = std::async(run_inference_async,
                                           std::ref(model),
                                           std::ref(inference_time),
                                           std::ref(input_queues),
                                           results_queue);

        auto output_parser_thread = std::async(run_post_process,
                                    std::ref(input_type),
                                    std::ref(org_height),
                                    std::ref(org_width),
                                    std::ref(frame_count),
                                    std::ref(capture),
                                    std::ref(args.framerate),
                                    std::ref(args.batch_size),
                                    std::ref(args.save_stream_output),
                                    std::ref(args.no_display),
                                    std::ref(args.output_dir),
                                    std::ref(args.output_resolution),
                                    results_queue,
                                    post_cb);

        // Hiwonder: the mission thread starts once camera, inference and post-process
        // are already running. The physical button is still what starts the motion.
        if(!g_solo_percepcion){
            const int rc = pthread_create(&Main_Actions, NULL, Obstacle_Challenge_Thread, NULL);
            if(rc != 0){
                throw std::runtime_error("no se pudo crear el hilo de mision");
            }
            g_main_actions_started = true;
        }

        // Blocks until the camera pipeline ends (input closed or error).
        hailo_status status = wait_and_check_threads(
            preprocess_thread,    "Preprocess",
            inference_thread,     "Inference",
            output_parser_thread, "Postprocess "
        );
        if (HAILO_SUCCESS != status) {
            Apagar_Todo();
            return status;
        }
        
        auto t_end = Clock::now();
        print_inference_statistics(inference_time, args.net, static_cast<double>(frame_count), t_end - t_start);

        Apagar_Todo();
        return HAILO_SUCCESS;
    }
    catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        Apagar_Todo();
        return HAILO_INTERNAL_FAILURE;
    }
}

// Obstacle Challenge mission (runs in the Main_Actions thread). Waits for the start
// button, then either runs the fusion demo (--fusion-demo) or the real mission:
// detect the driving direction from the side distances, leave the parking lot,
// run 12 sections (3 laps x 4 sections, Corner_Case + Desicion per section) and
// parallel-park. Moves the motors and blocks until terminating_main is set.
static void *Obstacle_Challenge_Impl(void *arg){
    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;
    float lidar_shared_buffer[360]; // local copy of the lidar scan (mm per degree index)
    Color_traffic_light cubo_temp;
    Color_traffic_light cubo_temp1;
    bool is_middle_case = false;

    //printf("distance right : %f\n", distancia_derecha);
    //printf("distance left : %f\n", distancia_izquierda);
    //printf("distance front : %f\n", distancia_frente);

    Spike_Reset_Gyro(0);
    Rasp_Gpio_Wait_For_Button();
    if (terminating_main != 0) return NULL;
    // Fusion demo (--fusion-demo): for 120 s, follow heading 0 and, when the lidar sees
    // something closer than 700 mm ahead, sidestep it on the side chosen from the
    // camera (red -> right, green -> left), the box position, or the freer side.
    // Stops early if the lidar stalls or both sides are blocked.
    if (g_fusion_demo_mode) {
        constexpr auto demo_duration = std::chrono::seconds(120);
        constexpr auto maneuver_reserve = std::chrono::seconds(12); // no new sidestep this close to the end
        constexpr float front_trigger_mm = 700.0f;   // obstacle ahead closer than this -> sidestep
        constexpr float side_clearance_mm = 450.0f;  // minimum free space needed on the passing side
        constexpr float no_return_mm = 2500.0f;      // value used when a sector has no valid return
        const auto demo_start = std::chrono::steady_clock::now();
        auto last_scan_change = demo_start;
        unsigned long last_scan_seq = Oradar_S2L_Get_Scan_Seq();
        bool stopped_early = false;
        std::printf("[FUSION DEMO] Camera + LiDAR, 120 seconds after button.\n");
        Spike_Reset_Gyro(0);

        // Nearest / mean valid range (mm) within +-half_width deg of a buffer index.
        auto sector_min = [&](int center, int half_width) {
            float nearest = no_return_mm;
            for (int offset = -half_width; offset <= half_width; ++offset) {
                const int index = (center + offset + 360) % 360;
                const float reading = lidar_shared_buffer[index];
                if (std::isfinite(reading) && reading > 0.0f)
                    nearest = std::min(nearest, reading);
            }
            return nearest;
        };
        auto sector_mean = [&](int center, int half_width) {
            float total = 0.0f;
            int count = 0;
            for (int offset = -half_width; offset <= half_width; ++offset) {
                const int index = (center + offset + 360) % 360;
                const float reading = lidar_shared_buffer[index];
                if (std::isfinite(reading) && reading > 0.0f) {
                    total += std::min(reading, no_return_mm);
                    ++count;
                }
            }
            return count ? total / count : no_return_mm;
        };

        while (terminating_main == 0 &&
               std::chrono::steady_clock::now() - demo_start < demo_duration) {
            const auto now = std::chrono::steady_clock::now();
            const unsigned long scan_seq = Oradar_S2L_Get_Scan_Seq();
            if (scan_seq != last_scan_seq) {
                last_scan_seq = scan_seq;
                last_scan_change = now;
            } else if (now - last_scan_change > std::chrono::milliseconds(750)) {
                std::printf("[FUSION DEMO] LiDAR scan stalled; braking.\n");
                stopped_early = true;
                break;
            }

            Oradar_S2L_Get_Buffer(lidar_shared_buffer);
            const float front_mm = sector_min(RP_TO_ORADAR_IDX(FRONT), 10);
            const float left_min = sector_min(RP_TO_ORADAR_IDX(LEFT), 20);
            const float right_min = sector_min(RP_TO_ORADAR_IDX(RIGHT), 20);
            const float left_mean = sector_mean(RP_TO_ORADAR_IDX(LEFT), 20);
            const float right_mean = sector_mean(RP_TO_ORADAR_IDX(RIGHT), 20);

            const bool camera_cube = traffic_lights.confidence >= 0.65f &&
                (traffic_lights.light_color == light_red ||
                 traffic_lights.light_color == light_green);
            if (front_mm <= front_trigger_mm) {
                Spike_Break_Motors();
                if (std::chrono::steady_clock::now() - demo_start >=
                    demo_duration - maneuver_reserve) break;

                direction preferred = invalid;
                if (camera_cube) {
                    // Keep the challenge's red/right and green/left passing convention.
                    preferred = traffic_lights.light_color == light_red ? right : left;
                } else if (traffic_lights.confidence >= 0.65f &&
                           traffic_lights.middle_point_x > 0.0f &&
                           traffic_lights.middle_point_x < 1.0f) {
                    // If color is uncertain, pass away from the cube's image position.
                    preferred = traffic_lights.middle_point_x < 0.5f ? right : left;
                } else {
                    preferred = left_mean >= right_mean ? left : right;
                }

                const float preferred_min = preferred == left ? left_min : right_min;
                const float alternate_min = preferred == left ? right_min : left_min;
                if (preferred_min < side_clearance_mm &&
                    alternate_min < side_clearance_mm) {
                    std::printf("[FUSION DEMO] Both sides blocked (front %.0f mm); braking early.\n",
                                front_mm);
                    stopped_early = true;
                    break;
                }
                if (preferred_min < side_clearance_mm)
                    preferred = preferred == left ? right : left;

                std::printf("[FUSION DEMO] front %.0f mm, camera %s (x=%.2f); pass %s.\n",
                    front_mm, camera_cube ?
                    (traffic_lights.light_color == light_red ? "red" : "green") : "uncertain",
                    traffic_lights.middle_point_x, preferred == left ? "left" : "right");
                // Sidestep: turn 70 deg away, advance 280 mm, turn back to heading 0.
                Spike_Turn_For_Degrees(preferred, 25, 70, 40, true);
                Spike_Reset_Gyro(0);
                Spike_Center_Vehicle_Short();

                Oradar_S2L_Get_Buffer(lidar_shared_buffer);
                if (sector_min(RP_TO_ORADAR_IDX(FRONT), 10) < 400.0f) {
                    std::printf("[FUSION DEMO] Path still blocked after turn; braking early.\n");
                    stopped_early = true;
                    break;
                }
                Spike_Advance_For_distance(25, 280, 0);
                Spike_Turn_For_Degrees(preferred == left ? right : left, 25, 70, 40, true);
                Spike_Reset_Gyro(0);
                Spike_Center_Vehicle_Short();
            } else {
                Spike_Follow_Reference(25, 0, 0);
            }
            usleep(50000);
        }

        Spike_Break_Motors();
        std::printf("[FUSION DEMO] Stopped: %s.\n",
            stopped_early ? "sensor/clearance safety stop" :
            (terminating_main ? "shutdown requested" : "120 seconds elapsed"));
    } else {
    usleep(200000); // let the gyro reset settle (200 ms)
    float slope = Slope(front);
    Spike_Reset_Gyro(0);
    Spike_Center_Vehicle_Short();

    Oradar_S2L_Get_Buffer(&lidar_shared_buffer[0]);
    distancia_frente = lidar_shared_buffer[270]; // front = index 270 in the buffer (90 is behind)
    distancia_derecha = lidar_shared_buffer[0];     // right = index 0
    distancia_izquierda = lidar_shared_buffer[180]; // left = index 180


    // Earlier test sequences, kept disabled.
    /*cubo_temp = esquivar_cubos_1();
    cubo_temp = esquivar_cubos_2(cubo_temp);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);*/
    
    //cubo_temp = Corner_Case(light_red, &is_middle_case);
    //printf("corner pillar: %d\n", cubo_temp);
    //cubo_temp = Desicion(cubo_temp, is_middle_case);
    //printf("pillar 2: %d\n", cubo_temp);
    //cubo_temp = Corner_Case(light_red, &is_middle_case);
    //slope = Slope(front);
    //Spike_Reset_Gyro(slope);
    //esquivar_cubos_middle(false);
    //cubo_temp = esquivar_cubos_1(false);
    //esquivar_cubos_2(cubo_temp, false);
    //Spike_Turn_For_Degrees(right, 60, 90, 40);
    
    // Driving direction. The robot starts in the parking lot, which lies along the
    // outer wall: if the open lane (more than 600 mm free) is on the right, the lap is
    // clockwise; if it is on the left, counterclockwise.
    if(distancia_derecha > 600){
        printf("Sentido horario\n");
        // Leave the parking lot, then 3 laps x 4 sections; the 4th section of each
        // lap (parking = true) is the one with the parking lot.
        cubo_temp = estacionamiento_clockwise();
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
        cubo_temp = Desicion(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
        cubo_temp = Desicion(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
        cubo_temp = Desicion(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case,true);
        cubo_temp = Desicion(cubo_temp, is_middle_case,true);
        printf("vuelta 1 terminada");
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
        cubo_temp = Desicion(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
        cubo_temp = Desicion(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
        cubo_temp = Desicion(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case,true);
        printf("vuelta 2 terminada");
        cubo_temp = Desicion(cubo_temp, is_middle_case,true);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
        cubo_temp = Desicion(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
        cubo_temp = Desicion(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
        cubo_temp = Desicion(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case(cubo_temp, &is_middle_case,true);
        cubo_temp = Desicion(cubo_temp, is_middle_case,true);
        // Final parking (clockwise). First reposition according to the last pillar
        // passed so the robot ends up beside the parking lot.
        if(cubo_temp == light_red){ /* red: go to 950 mm from the front wall, turn left 90 deg, advance to 260 mm, turn back right */ 
            Oradar_S2L_Advance_Until_Distance(50, 0, 950, Hold);
            Spike_Turn_For_Degrees(left, 60,90,45, true);
            Spike_Center_Vehicle();
            Oradar_S2L_Advance_Until_Distance(45, 90, 260, Hold);
            Spike_Reset_Gyro(0);
            usleep(100000);
            Spike_Turn_For_Degrees(right, 50, 90, 45, true);
            Spike_Center_Vehicle();
        }else if(cubo_temp == light_green){/* green: turn 10 deg left, creep at heading 10 until index 190 (left, 10 deg forward) reads <= 180 mm */
            Spike_Turn_For_Degrees(left, 60,10,45, true);
            Spike_Center_Vehicle();
            float lidar_buffer[360];
            Oradar_S2L_Get_Buffer(&lidar_buffer[0]);
            float distance_to_left = lidar_buffer[190];
            while(distance_to_left > 180){
                Spike_Forward(45, 10);
                Oradar_S2L_Get_Buffer(&lidar_buffer[0]);
                distance_to_left = lidar_buffer[190];
                usleep(1000);
            }
            Spike_Reset_Gyro(0);
            usleep(100000);
            Spike_Turn_For_Degrees(right, 50,4,30, true);
            Spike_Center_Vehicle_Short();
        }
        // Square up on the front wall, approach it to 150 mm, then reverse 2250 hub
        // degrees along the lot and back into it with a left/right reverse S-turn.
        float slope_final = Slope(front);
        printf("===============------------==================== reset angle before %f\n", slope_final);
        Spike_Reset_Gyro(slope_final);
        usleep(100000);
        float distance_to_left = 1000;
        distance_to_left = Distance_To_Wall(left);
        if(cubo_temp == light_red)
            Oradar_S2L_Advance_Until_Distance(45, (slope_final + 2), 150, Hold);
        else{
            Oradar_S2L_Advance_Until_Distance(45, (slope_final), 150, Hold);
        }
        Spike_Reset_Gyro(0);
        usleep(100000);
        Spike_Advance_For_Degrees(-50,2250,slope_final);
        Spike_Turn_For_Degrees(right, -50,50,45, true);
        Spike_Center_Vehicle();
        Spike_Reset_Gyro(0);
        usleep(100000);
        Spike_Turn_For_Degrees(left, -50, 105, 45, true);
        Spike_Center_Vehicle();
        usleep(200000);
        // Reverse into the lot at the -105 deg heading, then straighten the wheels
        // with a short reverse right turn and a small forward left correction.
        Spike_Advance_For_Degrees(-50,400,-105);
        Spike_Break_Motors();
        Spike_Reset_Gyro(0);
        usleep(100000); 
        Spike_Turn_For_Degrees(right, -50,28,45, true);
        Spike_Hold_Motors();
        Spike_Reset_Gyro(0);
        usleep(100000);
        Spike_Turn_For_Degrees(left, 40, -15, 45, true);
        Spike_Center_Vehicle();

    }

    else if (distancia_izquierda > 600){
        printf("Sentido antihorario\n");
        // Leave the parking lot, then 3 laps x 4 sections (mirror of the clockwise
        // branch). If the pillar read on leaving the lot was red, back up 200 hub
        // degrees at heading 90 deg after the first corner.
        cubo_temp1 = estacionamiento_counterclockwise();
        cubo_temp = Corner_Case_counterclockwise(cubo_temp1, &is_middle_case);
        if(cubo_temp1 == light_red){
            Spike_Advance_For_Degrees(-80,200,90);
            Spike_Break_Motors();
        }
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case,true);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case,true);
        printf("vuelta 1 terminada");
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case,true);
        printf("vuelta 2 terminada");
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case,true);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case);
        cubo_temp = Corner_Case_counterclockwise(cubo_temp, &is_middle_case,true);
        cubo_temp = Desicion_counterclockwise(cubo_temp, is_middle_case,true);
        printf("acabaron las vueltas, etacionamiento\n");
        usleep(100000);
        // Final parking (counterclockwise). A green last pillar needs the same
        // repositioning as red in the clockwise branch, mirrored.
        if(cubo_temp == light_green){
            Oradar_S2L_Advance_Until_Distance(50, 0, 950, Hold);
            Spike_Turn_For_Degrees(right, 60,90,45, true);
            Spike_Center_Vehicle();
            Oradar_S2L_Advance_Until_Distance(45, -90, 260, Hold);
            Spike_Reset_Gyro(0);
            usleep(100000);
            Spike_Turn_For_Degrees(left, 50, 90, 45, true);
            Spike_Center_Vehicle();
        }
        
        float slope_final = Slope(front);
        /*float slope_final = Slope(right);
        printf("reset angle before %f\n", slope);
        if(slope_final > 0){
            slope_final = slope_final - 90;
        }else{
            slope_final = slope_final + 90;
        }
        if(slope_final < 3){
            Spike_Reset_Gyro(slope_final);
        }else{
            Spike_Reset_Gyro(0);
        }*/
        usleep(100000);
        // Creep forward at heading -10 deg until the right-side reading
        // lidar_shared_buffer[0] is <= 160 mm, approach the front wall to 200 mm,
        // then reverse 740 hub degrees and back into the lot with a reverse S-turn.
        float distance_to_right = 1000;
        distance_to_right = lidar_shared_buffer[0];;
        while(distance_to_right > 160){
            Spike_Forward(45, -10);
            distance_to_right = lidar_shared_buffer[0];;
            usleep(1000);
        }
        Oradar_S2L_Advance_Until_Distance(50, 0, 200, Hold);
        Spike_Reset_Gyro(0);
        usleep(100000);
        Spike_Advance_For_Degrees(-50,740,0);
        Spike_Turn_For_Degrees(left, -50,52,45, true);
        Spike_Center_Vehicle();
        Spike_Reset_Gyro(0);
        usleep(100000);
        Spike_Turn_For_Degrees(right, -50, 100, 45, true);
        Spike_Center_Vehicle();
        usleep(200000);
        Spike_Advance_For_Degrees(-50,390,90);
        Spike_Break_Motors();
        Spike_Reset_Gyro(0);
        usleep(100000); 
        Spike_Turn_For_Degrees(left, -50,28,45, true);
        Spike_Hold_Motors();
        Spike_Reset_Gyro(0);
        usleep(100000);
        Spike_Turn_For_Degrees(right, 40, -15, 45, true);
        Spike_Center_Vehicle();

        

    }

    }

    // Mission finished: idle until shutdown is requested (Ctrl+C / kill).
    while(terminating_main == 0){
        /*Oradar_S2L_Get_Buffer(&lidar_shared_buffer[0] );
        distancia_izquierda = lidar_shared_buffer[90];
        printf("distance left : %f\n", distancia_izquierda);*/
        usleep(1000);
    }

    // Hiwonder: there is no REPL to interrupt; main() shuts down lidar, GPIO and board.
    Spike_Coast_Motors();
    return NULL;
}

// Hiwonder: the board layer throws std::exception if the board stops responding or a
// command is invalid; here the robot is stopped (motors coast) and shutdown is
// requested instead of ending the process with the motor still running.
// pthread entry point of the mission thread.
void *Obstacle_Challenge_Thread(void *arg){
    try {
        return Obstacle_Challenge_Impl(arg);
    } catch (const std::exception &e) {
        std::cerr << "ERROR en la mision Hiwonder: " << e.what() << "\n";
        terminating_main = 1;
        try { Spike_Coast_Motors(); } catch (...) {}
        return NULL;
    }
}

// Drives towards the detected pillar: each loop runs one PD steering step that moves
// the pillar's image x position (0..1) towards referencia_2. Stops when the box area
// reaches `area` (normalised 0..1) or the detection confidence drops to 0.65 or below,
// then coasts the motors. vel = drive speed (%).
void Follow_cubes(int vel, float referencia_2, float area){

    while((terminating_main == 0) && (traffic_lights.area < area) && (traffic_lights.confidence > 0.65) ){
        Spike_Follow_Reference(vel, traffic_lights.middle_point_x, referencia_2);
        usleep(1000);
        //printf("area : %f\n", traffic_lights.area);
    }
    //printf("area : %f , confidence: %f, terminating : %d\n", traffic_lights.area, traffic_lights.confidence, terminating_main );
    Spike_Coast_Motors();

}

// Total-least-squares (PCA) line orientation, in degrees, through the given indices of pts.
static float Fit_Line_Orientation(const std::vector<cv::Point2f> &pts, const std::vector<int> &indices) {
    double mean_x = 0, mean_y = 0;
    for (int idx : indices) { mean_x += pts[idx].x; mean_y += pts[idx].y; }
    mean_x /= indices.size();
    mean_y /= indices.size();

    double sxx = 0, syy = 0, sxy = 0;
    for (int idx : indices) {
        double dx = pts[idx].x - mean_x;
        double dy = pts[idx].y - mean_y;
        sxx += dx * dx;
        syy += dy * dy;
        sxy += dx * dy;
    }
    return Oradar_S2L_Radians_To_Degrees(0.5 * atan2(2.0 * sxy, sxx - syy));
}

// RMS perpendicular distance of the indexed points to their own best-fit line: a direct
// measure of how "noisy" the segment is (low = points line up cleanly, high = scattered).
static float Line_Fit_Residual_Mm(const std::vector<cv::Point2f> &pts, const std::vector<int> &indices) {
    if (indices.size() < 2) return 0.0f;
    float theta_rad = Oradar_S2L_Degrees_To_Radians(Fit_Line_Orientation(pts, indices));
    double nx = -sin(theta_rad), ny = cos(theta_rad); // unit normal to the fitted line

    double mean_x = 0, mean_y = 0;
    for (int idx : indices) { mean_x += pts[idx].x; mean_y += pts[idx].y; }
    mean_x /= indices.size();
    mean_y /= indices.size();

    double sum_sq = 0;
    for (int idx : indices) {
        double r = (pts[idx].x - mean_x) * nx + (pts[idx].y - mean_y) * ny;
        sum_sq += r * r;
    }
    return (float)sqrt(sum_sq / indices.size());
}

// Selects the wall seen on one side of the robot from the current LiDAR scan. The best
// wall segment is the one that passes the quality gates (enough points, enough length,
// orientation matching that side, low line-fit residual) with the largest extent, using
// the residual as a tiebreaker; the runner-up is merged into it only when the two are
// collinear (one wall split by a dropout). A noisy point, a stray outlier or a corner
// artifact in the window therefore does not skew the result. The function fetches the
// LiDAR buffer itself and runs wall detection (despike, outlier removal, gap + corner
// segmentation) over a window of `point` degrees centred on that side's reference angle.
//   side   : right, left, front or behind (anything else returns found = false).
//   return : WallSelection with the window points (mm, robot frame), the indices of the
//            chosen wall and their LiDAR angles; found = false when no wall qualifies.
//   side effect: updates the per-side wall tracker (mutex-protected, see below).

static WallSelection Select_Wall(const direction side) {
    WallSelection result;

    // The window's center angle and width are fully determined by which side of the
    // robot we're looking at - callers just say which side, not raw lidar angles.
    int middle_point = 0;
    // The LiDAR buffer's raw zero-reference sits 90deg off from the robot's actual front
    // (confirmed empirically: testing "right" lit up the wall the robot is squarely
    // facing). front/right are swapped from the naive 0/270 guess; left=180 and
    // behind=90 were already correct from an earlier empirical fix.
    switch (side) {
        case right:  middle_point = 0;   break;
        case left:   middle_point = 180; break;
        case front:  middle_point = 270; break;
        case behind: middle_point = 90;  break;
        default:
            //printf("Pendiente: invalid side\n");
            return result;
    }
    // The 4 directions are only 90deg apart (right=0, behind=90, left=180, front=270),
    // so the window must stay well under 90deg wide or adjacent sides start reading
    // each other's walls. Keeping a margin here costs some long-wall coverage near a
    // direction boundary, but a "right" reading that's actually looking left is worse.
    const int point = 70;

    float lidar_buffer[360];
    Oradar_S2L_Get_Buffer(&lidar_buffer[0]);

    // Points right at/near middle_point are excluded: that's often a corner or a seam
    // between two walls, and including it tends to pull in a small noisy cluster that
    // can otherwise sneak past the wall-quality gate below. Kept small (not 0) so a real
    // seam still gets a point dropped at the exact boundary; wider than this and a
    // continuous flat wall gets visibly split in two for no benefit (confirmed via
    // Lidar Coordinates_screenshot_41: at ~1650mm range a 3deg exclusion opened a
    // ~200mm gap in an otherwise unbroken front wall).
    const int middle_exclusion_deg = 1;
    int middle_point_wrapped = ((middle_point % 360) + 360) % 360;

    // Anything closer than min_range_mm is almost certainly the robot's own chassis/
    // mount, not an environmental wall. Those returns are trivially "clean" (tiny
    // residual, since a rigid nearby surface fits a line almost perfectly), so without
    // this filter they out-score real walls every time instead of just being noise.
    const float min_range_mm = 150.0f;

    // ROR (below) needs neighbors on both sides of every point to judge it fairly. A
    // real wall point sitting right at the edge of the requested window has neighbors
    // that exist just *outside* the window - if we never collect those, the edge point
    // looks falsely isolated and ROR drops it, truncating real walls right at the
    // window boundary. So we collect a padded range purely to support ROR's neighbor
    // counting, then trim back down to the actually-requested window afterwards.
    const int edge_pad_deg = 15;

    // The window is centered on middle_point - half the points fall before it, half
    // after - instead of entirely before it. A one-sided window can structurally never
    // see a wall that happens to sit just past the reference angle on the other side,
    // no matter how wide it's made.
    const int half_point = point / 2;

    std::vector<cv::Point2f> pts;
    std::vector<int> pt_angle;
    std::vector<bool> pt_is_core;
    pts.reserve(point + 2 * edge_pad_deg);
    pt_angle.reserve(point + 2 * edge_pad_deg);
    pt_is_core.reserve(point + 2 * edge_pad_deg);
    for (int i = -edge_pad_deg; i < point + edge_pad_deg; i++) {
        int raw_angle = middle_point - half_point + i;
        int wrapped_angle = ((raw_angle % 360) + 360) % 360;

        int angular_diff = std::abs(wrapped_angle - middle_point_wrapped);
        if (angular_diff > 180) angular_diff = 360 - angular_diff;
        if (angular_diff <= middle_exclusion_deg) continue;

        float d = lidar_buffer[wrapped_angle];
        if (d > min_range_mm) {
            pts.emplace_back(d * cos(Oradar_S2L_Degrees_To_Radians((float)wrapped_angle)),
                              d * sin(Oradar_S2L_Degrees_To_Radians((float)wrapped_angle)));
            pt_angle.push_back(wrapped_angle);
            pt_is_core.push_back(i >= 0 && i < point);
        }
    }

    if (pts.size() < 2) {
        //printf("Pendiente: not enough valid lidar points (%zu)\n", pts.size());
        return result;
    }

    // Despike: a short run (1-2 points) whose range jumps sharply away from and then
    // straight back to a level both flanking neighbors agree on is a multipath/
    // reflective-surface glitch, not a real object or gap. Such a run gets its range
    // replaced with a linear interpolation between its flanking neighbors, rather than
    // just deleted: deleting it still leaves an angular gap between the two flanks wide
    // enough (at typical wall ranges, a 2-3deg gap alone can exceed gap_threshold_mm) to
    // trip the gap-based segmenter below anyway, so the wall still gets chopped in two.
    // Interpolating keeps the point density continuous through the glitch instead.
    // Confirmed via a logged front-wall scan (lidar_debug_log.csv snapshot_id 0): angles
    // 251-252 dipped to ~1959/1979mm between neighbors reading ~2386mm and ~2360mm,
    // splitting the front wall into a 5-point and a 22-point fragment; interpolating
    // instead of deleting merges them back into one 29-point run.
    const float despike_neighbor_agree_mm = 60.0f;  // how tightly the two flanks must agree with each other
    const float despike_jump_mm = 150.0f;           // how far the run must diverge from that flank level
    // 3, not 2: a second logged front-wall scan (lidar_debug_log.csv, run starting
    // 2026-09-16 19:26, snapshot_id 3) hit a 3-point glitch (angles 261-263, dipping to
    // ~1430mm between ~2450mm neighbors) that a 2-point run misses entirely. Combined
    // with the always-present middle_exclusion_deg gap at the window's center, that left
    // every fragment under min_wall_points and front came back "not found" outright.
    // Bridging up to 3 points recovers a clean 13-point/2.6mm-residual segment there.
    const int   despike_max_run = 3;                // longest glitch run considered, in points
    {
        for (size_t i = 1; i < pts.size(); i++) {
            for (int run_len = 1; run_len <= despike_max_run; run_len++) {
                if (i + (size_t)run_len >= pts.size()) break; // need a flank point on both sides
                float before_r = (float)cv::norm(pts[i - 1]);
                float after_r = (float)cv::norm(pts[i + run_len]);
                if (std::fabs(before_r - after_r) > despike_neighbor_agree_mm) continue; // flanks disagree - not a clean glitch
                bool all_diverge = true;
                for (int k = 0; k < run_len; k++) {
                    float r = (float)cv::norm(pts[i + k]);
                    if (std::fabs(r - before_r) < despike_jump_mm || std::fabs(r - after_r) < despike_jump_mm) {
                        all_diverge = false;
                        break;
                    }
                }
                if (all_diverge) {
                    for (int k = 0; k < run_len; k++) {
                        float frac = (float)(k + 1) / (float)(run_len + 1);
                        float interp_r = before_r + frac * (after_r - before_r);
                        float angle_deg = (float)pt_angle[i + k];
                        float angle_rad = Oradar_S2L_Degrees_To_Radians(angle_deg);
                        pts[i + k] = cv::Point2f(interp_r * cos(angle_rad), interp_r * sin(angle_rad));
                    }
                    break;
                }
            }
        }
    }

    // Radius Outlier Removal: drop points with too few neighbors nearby. Without this,
    // a single stray noisy point can trigger a false gap/corner split below and shatter
    // a real, long wall into several small fragments - while a small isolated cluster
    // (with no room for a stray point to sneak in) stays intact and wins by comparison.
    // Neighbor counting uses the padded set (above), but only core (in-window) points
    // are kept afterwards - the padding points were only there to give edge points a
    // fair neighbor count.
    const float ror_radius_mm = 300.0f;
    const int   ror_min_neighbors = 3;
    {
        std::vector<cv::Point2f> filtered_pts;
        std::vector<int> filtered_angle;
        filtered_pts.reserve(pts.size());
        filtered_angle.reserve(pts.size());
        for (size_t i = 0; i < pts.size(); i++) {
            if (!pt_is_core[i]) continue;
            int neighbor_count = 0;
            for (size_t j = 0; j < pts.size(); j++) {
                if (i == j) continue;
                if ((float)cv::norm(pts[i] - pts[j]) <= ror_radius_mm) {
                    neighbor_count++;
                    if (neighbor_count >= ror_min_neighbors) break;
                }
            }
            if (neighbor_count >= ror_min_neighbors) {
                filtered_pts.push_back(pts[i]);
                filtered_angle.push_back(pt_angle[i]);
            }
        }
        pts = std::move(filtered_pts);
        pt_angle = std::move(filtered_angle);
    }

    if (pts.size() < 2) {
        //printf("Pendiente: not enough valid lidar points after outlier removal (%zu)\n", pts.size());
        return result;
    }

    // Gap-based segmentation: a new wall starts wherever consecutive points jump apart
    const float gap_threshold_mm = 75.0f;
    std::vector<std::vector<int>> segments;
    std::vector<int> current_segment = {0};
    for (size_t i = 1; i < pts.size(); i++) {
        if ((float)cv::norm(pts[i] - pts[i - 1]) > gap_threshold_mm) {
            segments.push_back(current_segment);
            current_segment.clear();
        }
        current_segment.push_back((int)i);
    }
    segments.push_back(current_segment);

    // Further split each segment at corners (orientation changes with no range gap)
    const float corner_split_threshold_mm = 40.0f;
    const int   corner_margin_points = 2;
    std::vector<std::vector<int>> wall_segments;
    for (auto &seg : segments) {
        split_segment_at_corners(pts, seg, corner_split_threshold_mm, corner_margin_points, wall_segments);
    }

    // Score each wall: it must be linear enough (residual below max_wall_residual_mm)
    // and have enough points to count as a real wall at all - a noisy, scattered
    // segment must never outrank a smaller but cleanly-fit one just by having a few
    // more points. Among walls that qualify, prefer more physical extent (a longer,
    // more reliable read of the same surface), then the cleanest fit.
    //
    // min_wall_points must be high enough that residual is actually meaningful: a 2-3
    // point segment is geometrically almost a perfect line no matter what (2 points
    // define a line with zero residual by definition), so tiny fragments are
    // structurally biased toward looking "clean" even though they prove nothing. A
    // real wall spanning many points will accumulate genuine sensor noise and can
    // legitimately have a higher (but still meaningful) residual than a 2-point fluke.
    const float max_wall_residual_mm = 15.0f;
    const int   min_wall_points = 6;
    // A compact object (a pillar, a box, the corner of an obstacle) can rack up enough
    // points within ROR's radius to pass min_wall_points and still read as "clean" -
    // residual alone can't tell a wall from a blob. A real wall has to actually extend
    // a meaningful distance; requiring that directly excludes objects no matter how
    // many points or how low a residual they happen to have.
    const float min_wall_extent_mm = 200.0f;
    // A wall genuinely facing "side" must run roughly perpendicular to that side's
    // viewing axis (e.g. a real front wall runs left-right, not front-back). Without
    // this check, a close/long side wall can leak into an adjacent direction's window:
    // it's close enough that its far end still subtends an angle near that other
    // direction's reference angle, even though the wall itself clearly belongs to the
    // side it's parallel to. Confirmed via Lidar Coordinates_screenshot_ (front reported
    // 487mm while the highlighted points were plainly a continuation of the right wall,
    // same orientation as the "right" pick, ~90deg off from a genuine front wall).
    const float expected_orientation_deg = std::fmod((float)middle_point + 90.0f, 180.0f);
    const float max_side_orientation_diff_deg = 50.0f;
    struct SegmentScore {
        size_t seg_idx; float residual; bool qualifies;
        cv::Point2f centroid; float orientation_deg; float extent;
    };
    std::vector<SegmentScore> scores;
    for (size_t s = 0; s < wall_segments.size(); s++) {
        const auto &seg = wall_segments[s];
        if ((int)seg.size() < min_wall_points) continue; // too small to mean anything
        float extent = (float)cv::norm(pts[seg.back()] - pts[seg.front()]);
        if (extent < min_wall_extent_mm) continue; // too compact to be a wall, not just noisy
        float residual = Line_Fit_Residual_Mm(pts, seg);
        bool qualifies = (residual <= max_wall_residual_mm);
        cv::Point2f centroid(0, 0);
        for (int idx : seg) centroid += pts[idx];
        centroid *= (1.0f / (float)seg.size());
        float orientation_deg = Fit_Line_Orientation(pts, seg);
        float orientation_diff = std::fmod(std::fabs(orientation_deg - expected_orientation_deg), 180.0f);
        if (orientation_diff > 90.0f) orientation_diff = 180.0f - orientation_diff;
        if (orientation_diff > max_side_orientation_diff_deg) continue; // wrong-facing wall, not this side's
        scores.push_back({s, residual, qualifies, centroid, orientation_deg, extent});
    }
    std::sort(scores.begin(), scores.end(), [](const SegmentScore &a, const SegmentScore &b) {
        if (a.qualifies != b.qualifies) return a.qualifies; // qualifying walls always rank first
        // Both qualifying or both not: prefer more physical extent (a longer, more
        // reliable read of the surface) first, and only use residual as a tiebreaker.
        // Point count is deliberately NOT used here: within a fixed angular window, a
        // nearby wall is sampled far more densely than a distant one purely because
        // it's close (more lidar points per degree at short range), so raw count
        // systematically favors whatever's nearest regardless of which one is actually
        // the better/longer wall read. Confirmed via Select_Wall front-window debug
        // logs: a ~600-700mm side wall consistently out-counted a genuine ~1700mm front
        // wall (34 vs 22 points) despite the front wall's residual being 2-3x cleaner -
        // extent correctly favors the front wall instead (it subtends a wider arc at
        // range, so its mm-extent across the window is larger despite fewer points).
        if (a.extent != b.extent) return a.extent > b.extent;
        return a.residual < b.residual;
    });

    if (scores.empty()) {
        //printf("Error: no wall candidates found.\n");
        return result;
    }

    // Per-side wall tracking: once a wall has been chosen for a given side, prefer to
    // keep tracking that same physical wall (by centroid + orientation) across calls,
    // rather than always snapping to whichever segment scores best in the fixed
    // angular window this frame. A few degrees of robot rotation can shift the window
    // enough that an adjacent wall briefly outscores the one actually being followed,
    // which otherwise causes a visible flip mid-turn.
    struct WallTrackState { bool valid = false; cv::Point2f centroid; float orientation_deg = 0; };
    // Guarded by wall_track_mutex: Select_Wall is now called both from the robot-control
    // thread (Obstacle_Challenge_Thread) and independently from the lidar visualization
    // in postprocess_callback (which queries front/right/left every scan regardless of
    // what the control thread is doing), so this shared map needs real locking.
    static std::unordered_map<int, WallTrackState> wall_track_by_side;
    static pthread_mutex_t wall_track_mutex = PTHREAD_MUTEX_INITIALIZER;
    const float wall_track_max_match_mm = 300.0f;
    const float wall_track_max_angle_diff_deg = 20.0f;
    // A tracked match is only honored if it's still reasonably competitive with this
    // frame's best-scoring candidate (scores[0], since scores is sorted best-first) -
    // otherwise the tracker can never recover from a bad lock. E.g. if the window
    // briefly sweeps across a nearby wall mid-turn and locks onto it, that near wall
    // stays static and keeps re-matching itself every frame after (near-zero centroid
    // drift, well within threshold), permanently hiding the real front wall even once
    // it's back in full view with much more extent. Requiring the tracked candidate to
    // hold at least this fraction of the top candidate's extent lets an obviously
    // better wall pre-empt a stale lock instead of being ignored forever (confirmed via
    // Lidar Coordinates_screenshot_42/43: "front" got stuck reporting the left wall after
    // a 25deg turn-and-back, even though the true, longer front wall was still in-window).
    const float wall_track_min_extent_ratio = 0.7f;

    size_t best_idx = 0; // default: the best-quality candidate this frame
    {
        pthread_mutex_lock(&wall_track_mutex);
        auto track_it = wall_track_by_side.find((int)side);
        if (track_it != wall_track_by_side.end() && track_it->second.valid) {
            float best_match_dist = wall_track_max_match_mm;
            bool found_match = false;
            for (size_t i = 0; i < scores.size(); i++) {
                if (!scores[i].qualifies) continue;
                float dist = (float)cv::norm(scores[i].centroid - track_it->second.centroid);
                float angle_diff = std::fabs(scores[i].orientation_deg - track_it->second.orientation_deg);
                if (angle_diff > 90.0f) angle_diff = 180.0f - angle_diff;
                if (dist < best_match_dist && angle_diff <= wall_track_max_angle_diff_deg) {
                    best_match_dist = dist;
                    best_idx = i;
                    found_match = true;
                }
            }
            if (!found_match ||
                scores[best_idx].extent < wall_track_min_extent_ratio * scores[0].extent) {
                best_idx = 0; // stale/inferior lock - a clearly better wall is available, re-acquire fresh
            }
        }
        pthread_mutex_unlock(&wall_track_mutex);
    }

    // Only merge the chosen wall with another candidate if they're actually consistent
    // with being the same flat surface (just split by a sensor dropout). Orientation
    // alone isn't enough proof of that: two genuinely separate walls running parallel
    // to each other (very common in a rectangular arena - e.g. a near side wall and a
    // further, unrelated wall glimpsed at the edge of the same window) share an
    // orientation but sit at very different perpendicular offsets. Two halves of one
    // real dropout-split wall are collinear, not just parallel, so also require the
    // merge candidate's centroid to sit close to the chosen wall's own fitted line -
    // confirmed via Lidar Coordinates_screenshot_45: "right" merged a 470mm-range wall
    // with a second, ~1900mm-range wall that only happened to share its orientation,
    // and reported one meaningless blended distance for what were really two walls.
    const float max_merge_angle_diff_deg = 12.0f;
    const float max_merge_perp_offset_mm = 150.0f;
    std::vector<int> chosen_indices = wall_segments[scores[best_idx].seg_idx];
    {
        size_t merge_idx = (best_idx == 0) ? 1 : 0;
        if (merge_idx < scores.size() && merge_idx != best_idx) {
            float orientation_diff = std::fabs(scores[best_idx].orientation_deg - scores[merge_idx].orientation_deg);
            if (orientation_diff > 90.0f) orientation_diff = 180.0f - orientation_diff;
            float theta_rad = Oradar_S2L_Degrees_To_Radians(scores[best_idx].orientation_deg);
            float nx = -sin(theta_rad), ny = cos(theta_rad); // unit normal to the chosen wall's line
            cv::Point2f delta = scores[merge_idx].centroid - scores[best_idx].centroid;
            float perp_offset_mm = std::fabs(delta.x * nx + delta.y * ny);
            if (orientation_diff <= max_merge_angle_diff_deg && perp_offset_mm <= max_merge_perp_offset_mm) {
                const auto &merge_seg = wall_segments[scores[merge_idx].seg_idx];
                chosen_indices.insert(chosen_indices.end(), merge_seg.begin(), merge_seg.end());
            }
        }
    }

    std::vector<int> used_angles;
    used_angles.reserve(chosen_indices.size());
    for (int idx : chosen_indices) used_angles.push_back(pt_angle[idx]);

    cv::Point2f final_centroid(0, 0);
    for (int idx : chosen_indices) final_centroid += pts[idx];
    final_centroid *= (1.0f / (float)chosen_indices.size());
    {
        pthread_mutex_lock(&wall_track_mutex);
        wall_track_by_side[(int)side] = WallTrackState{true, final_centroid, Fit_Line_Orientation(pts, chosen_indices)};
        pthread_mutex_unlock(&wall_track_mutex);
    }

    result.found = true;
    result.pts = std::move(pts);
    result.chosen_indices = std::move(chosen_indices);
    result.used_angles = std::move(used_angles);
    return result;
}

// Orientation (deg, -90..90, in the LiDAR buffer's x/y frame) of the wall selected on
// `side` by Select_Wall. A wall parallel to the robot's travel reads close to +-90 deg.
// Returns 0 when no wall is found.
float Slope(const direction side) {
    WallSelection sel = Select_Wall(side);
    if (!sel.found) return 0;
    return Fit_Line_Orientation(sel.pts, sel.chosen_indices);
}

// Perpendicular distance from the robot (origin) to the selected wall's fitted line -
// not just the centroid's range, since the centroid can sit off to one side of the
// perpendicular foot. Mirrors the highlighted-wall distance shown in the visualization.
// Returns the distance in mm, or 0 when no wall is found on that side.
float Distance_To_Wall(const direction side) {
    WallSelection sel = Select_Wall(side);
    if (!sel.found) return 0;

    float orientation_deg = Fit_Line_Orientation(sel.pts, sel.chosen_indices);
    float orientation_rad = Oradar_S2L_Degrees_To_Radians(orientation_deg);
    cv::Point2f dir(cos(orientation_rad), sin(orientation_rad));

    cv::Point2f centroid(0, 0);
    for (int idx : sel.chosen_indices) centroid += sel.pts[idx];
    centroid *= (1.0f / (float)sel.chosen_indices.size());

    return std::fabs(centroid.x * dir.y - centroid.y * dir.x);
}

/**
 * function name: Print_Slope
 * description: Debug helper: computes the slope (deg) of the wall selected on `side`;
 *              the printf that reported it is commented out.
 * return: void
 * parameters: direction side
 * side: which side of the robot to look at - right, left, front, or behind
 */
void Print_Slope(const direction side){
    float pendiente = Slope(side);
    //printf("Pendiente: %f\n", pendiente);
}

/**
 * function name: Follow_Wall
 * description: Drives at speed 60 while the PD steering loop pushes the wall orientation
 *              returned by Slope(side) towards 0 deg. Blocks until terminating_main is
 *              set (Ctrl+C / shutdown).
 * return: void
 * parameters: direction side
 * side: which side of the robot to look at - right, left, front, or behind
 */
void Follow_Wall(const direction side){
    while(terminating_main == 0){
        float pendiente = Slope(side);
        //printf("Pendiente: %f\n", pendiente);

        Spike_Follow_Reference(60, 0 ,pendiente);
    }
}

// Older start-of-section geometry (used by avoid_cube_start_section; the current mission
// uses the *_chr variants below). Returns atan(longitudinal / lateral) in deg, where
// longitudinal = front-wall distance - cube_number_per_section (Cube_number value, mm)
// and lateral = distance to a lane 185 mm from the left wall (green pillar) or from the
// right wall of the 1000 mm wide section (red pillar). Returns 0 for any other colour.
float calculte_angle_section_start_clockwise(Color_traffic_light traffic_light_color, Cube_number cube_number_per_section){
    float angle = 0;
    float angle_rad = 0;
    float distanceToFrontWallmm = 0;
    float diStanceToLeftWallmm = 0;
    float distanceToBackWallmm = 0;

    distanceToFrontWallmm = Distance_To_Wall(front);
    diStanceToLeftWallmm = Distance_To_Wall(left);

    //printf(" co/ca = %f\n",((distanceToFrontWallmm - 2000)/(diStanceToLeftWallmm - 185)));
    if(traffic_light_color == light_green){
        angle_rad = atan(((distanceToFrontWallmm - cube_number_per_section)/(diStanceToLeftWallmm - 185)));
    }else if(traffic_light_color == light_red){
        angle_rad = atan(((distanceToFrontWallmm - cube_number_per_section)/(1000 - diStanceToLeftWallmm - 185)));
    }else{
        return 0;
    }

    angle = Oradar_S2L_Radians_To_Degrees(angle_rad);
    //printf("distancia frente: %f, distancia izquierda: %f, angle rad: %f, angle deg: %f\n", distanceToFrontWallmm, diStanceToLeftWallmm, angle_rad, angle);
    return angle;
}

// Clockwise run: computes the heading change and straight-line distance needed to go
// from the robot's current position to the lane that passes a pillar on the correct side
// (green pillars on the left, red pillars on the right).
//   traffic_light_color     : colour of the pillar to pass (light_green / light_red).
//   cube_number_per_section : pillar row, as a distance in mm (Cube_number_chr):
//                             CUBE_first  = 1000 mm from the rear wall,
//                             CUBE_middle = 1450 mm from the rear wall,
//                             CUBE_second = 1300 mm from the front wall.
//   angle      [out]        : heading change from the section axis, deg (90 - atan(side_1/side_2)).
//   hypotenuse [out]        : straight-line distance to the target point, mm.
//   parking                 : true in the section that holds the parking lot; a green
//                             pillar then uses a lane 400 mm from the left wall instead of 185 mm.
// side_1 = longitudinal distance to the pillar row (mm); side_2 = lateral distance to the
// target lane, measured from the left wall (outer wall in a clockwise run); the red-pillar
// lane is 185 mm from the right wall of the 1000 mm wide section.
// Blocks until the LiDAR reports a front wall (or terminating_main is set).
void calculte_angle_section_start_clockwise_chr(Color_traffic_light traffic_light_color, Cube_number_chr cube_number_per_section, float *angle, float *hypotenuse, bool parking){
    float angle_rad = 0;
    float diStanceToFrontWallmm = 0;
    float diStanceToLeftWallmm = 0;
    float distanceToBackWallmm = 0;
    float lidar_shared_buffer[360];
    float distance_to_wall_front_temp = 0;
    float distance_to_wall_behind_temp = 0;
    float side_1 = 0;
    float side_2 = 0;
    usleep(100000);
    while((distance_to_wall_front_temp == 0) && (terminating_main == 0)){ /* waiting until a valid wall is detected */
        distance_to_wall_front_temp = Distance_To_Wall(front);
        distance_to_wall_behind_temp = Distance_To_Wall(behind);
    }
    Oradar_S2L_Get_Buffer(&lidar_shared_buffer[0]);
    diStanceToFrontWallmm = distance_to_wall_front_temp;
        // The rear distance is estimated from the field length (3000 mm) minus the front
        // distance, instead of the rear-wall fit, which returns 0 when the rear wall is
        // out of range or has no clean fit.
    distanceToBackWallmm = 3000 - distance_to_wall_front_temp;
    diStanceToLeftWallmm = lidar_shared_buffer[180]; // single raw ray at 180 = left

    //printf("atras: %f\n", distanceToBackWallmm);

    if(cube_number_per_section == CUBE_first){
        //printf("primer cubo\n");
        side_1 = cube_number_per_section - distanceToBackWallmm;
    
        if(traffic_light_color == light_green){
            //printf("cubo verde\n");
            if(parking == false){
                side_2 = diStanceToLeftWallmm - 185;
            }
            else{
                side_2 = diStanceToLeftWallmm - 400;
            }
            angle_rad = atan(((side_1)/(side_2)));
        }else if(traffic_light_color == light_red){
            //printf("cubo rojo\n");
            side_2 = 1000 - diStanceToLeftWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }
    }

    else if(cube_number_per_section == CUBE_second){
        //printf("segundo cubo\n");
        side_1 = diStanceToFrontWallmm - cube_number_per_section;
    
        if(traffic_light_color == light_green){
            //printf("cubo verde\n");
            side_2 = diStanceToLeftWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }else if(traffic_light_color == light_red){
            //printf("cubo rojo\n");
            side_2 = 1000 - diStanceToLeftWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }
    }

    else{
        //printf("cubo del medio\n");
        side_1 = cube_number_per_section - distanceToBackWallmm;
    
        if(traffic_light_color == light_green){
            //printf("cubo verde\n");
            if(parking == false){
                side_2 = diStanceToLeftWallmm - 185;
            }
            else{
                side_2 = diStanceToLeftWallmm - 400;
            }
            angle_rad = atan(((side_1)/(side_2)));
        }else if(traffic_light_color == light_red){
            //printf("cubo rojo\n");
            side_2 = 1000 - diStanceToLeftWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }
    }

    *hypotenuse = sqrt( pow(side_1,2) + pow(side_2,2));
    *angle = 90 - Oradar_S2L_Radians_To_Degrees(angle_rad);
    printf("distancia frente: %f, distancia atras: %f, distancia izquierda: %f, angle rad: %f\n", diStanceToFrontWallmm, distanceToBackWallmm, diStanceToLeftWallmm, angle_rad);
}

// Counter-clockwise version of calculte_angle_section_start_clockwise: same formula with
// the lateral distance measured from the right wall. Returns the angle in deg (0 for any
// other colour).
float calculte_angle_section_start_counterclockwise(Color_traffic_light traffic_light_color, Cube_number cube_number_per_section){
    float angle = 0;
    float angle_rad = 0;
    float distanceToFrontWallmm = 0;
    float diStanceToRightWallmm = 0;

    distanceToFrontWallmm = Distance_To_Wall(front);
    diStanceToRightWallmm = Distance_To_Wall(right);

    //printf(" co/ca = %f\n",((distanceToFrontWallmm - 2000)/(diStanceToLeftWallmm - 185)));
    if(traffic_light_color == light_green){
        angle_rad = atan(((distanceToFrontWallmm - cube_number_per_section)/(1000 - diStanceToRightWallmm - 185)));
    }else if(traffic_light_color == light_red){
        angle_rad = atan(((distanceToFrontWallmm - cube_number_per_section)/(diStanceToRightWallmm - 185)));
    }else{
        return 0;
    }

    angle = Oradar_S2L_Radians_To_Degrees(angle_rad);
    return angle;
}

// Older pillar-avoidance routine (the current mission uses esquivar_cubos_*): turns
// towards the passing side (green -> left, red -> right) by 90 - angle deg, drives until
// the side or front wall is closer than 250 mm, then steers back the other way.
// Blocks; moves the drive motor and the steering servo.
void avoid_cube_start_section(Color_traffic_light traffic_light_color, Cube_number cube_number_per_section){
    float angle_to_wall = 0;
    direction direction_to_turn = invalid;

    if(traffic_light_color == light_green){
        angle_to_wall = calculte_angle_section_start_clockwise(traffic_light_color, cube_number_per_section);
        direction_to_turn = left;
    }else if(traffic_light_color == light_red){
        angle_to_wall = calculte_angle_section_start_clockwise(traffic_light_color, cube_number_per_section);
        direction_to_turn = right;
    }
    
    Spike_Turn_For_Degrees(direction_to_turn, 60, 90 - angle_to_wall, 30);

    float distanceToWall = 10000;
    float distanceToWallfront = 10000;
    //usleep(5000000);
    Spike_Center_Vehicle_Short();
    //usleep(5000000);
    while((distanceToWall > 250) && (distanceToWallfront > 250)){
        distanceToWall = Distance_To_Wall(direction_to_turn);
        distanceToWallfront = Distance_To_Wall(front);
        if(direction_to_turn == left)
        {
            Spike_Follow_Reference(60,0,90 - angle_to_wall);
        }
        else
        {
            Spike_Follow_Reference(60,90 - angle_to_wall,0);
        }
        usleep(1000);
    }
    Spike_Coast_Motors();
    /* direction times -1 to reverse the turn direction when turning back after avoiding the cube */
    Spike_Small_Turn((direction_to_turn * (-1)),60,0,90 - angle_to_wall);
    Spike_Coast_Motors();
}

// Clockwise run: passes the first pillar of a section (row CUBE_first, 1000 mm from the
// rear wall) using the colour currently reported by the camera.
// Sequence: turn by angle + 5 deg towards the passing side, drive hypotenuse - 400 mm
// (hypotenuse - 200 mm for a green pillar in the parking section) holding that heading,
// then steer back to heading 0 (parallel to the section).
//   parking : true in the section that holds the parking lot.
//   return  : colour of the pillar that was passed (none if the camera saw nothing).
Color_traffic_light esquivar_cubos_1(bool parking){
    float angle_to_wall = 0;
    float hypotenuse = 0;
    Color_traffic_light cube = traffic_lights.light_color;
    direction direction_to_turn = invalid;
    /*while(cube == none){
        cube = traffic_lights.light_color;
        usleep(1000);
    }*/
    if(cube == light_green){
        printf("green cube\n");
        calculte_angle_section_start_clockwise_chr(cube, CUBE_first, &angle_to_wall, &hypotenuse, parking);
        direction_to_turn = left;
    }else if(cube == light_red){
        printf("red cube\n");
        calculte_angle_section_start_clockwise_chr(cube, CUBE_first, &angle_to_wall, &hypotenuse, parking);
        direction_to_turn = right;
    }
    printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
    //usleep(10000000);
    if(angle_to_wall > 70.0){ /* limit the angle in case of an emergency */
        angle_to_wall = 70.0;
    }
    Spike_Turn_For_Degrees(direction_to_turn, 60, abs(angle_to_wall) + 5, 40);
    Spike_Center_Vehicle_Short();
    if((parking == false) || (cube == light_red)){
        Spike_Advance_For_distance(80, (int)hypotenuse - 400, ((abs(angle_to_wall) + 5) *direction_to_turn*-1));
    }
    else{
        printf("caso verde en la seccion de parking\n");
        Spike_Advance_For_distance(80, (int)hypotenuse - 200 , (angle_to_wall*direction_to_turn*-1));
    }
    Spike_Small_Turn((direction_to_turn * -1), 60, 0, 40);
    Spike_Center_Vehicle_Short();
    
    return cube;
}

// Clockwise run: handles the second pillar of a section (row CUBE_second, 1300 mm from
// the front wall), given the colour of the pillar just passed.
// A second pillar counts as present only when the camera reports a red/green pillar (not
// a parking marker) whose normalised image x-position is below 0.5 (after a red pillar)
// or 0.90 (after a green one). Same colour as before -> the robot is already on the right
// side and drives straight to 1000 mm from the front wall (green in the parking section:
// to 1500 mm, then a short S-shift to the left). Different colour -> crosses the lane with
// a computed turn and Advance_For_distance_Especial, then steers back to heading 0.
// No second pillar -> drives straight to 1000 mm from the front wall.
//   return : colour of the second pillar if present, otherwise past_cube.
Color_traffic_light esquivar_cubos_2( Color_traffic_light past_cube, bool parking){
    float angle_to_wall = 0;
    float hypotenuse = 0;
    Color_traffic_light cube = traffic_lights.light_color;
    float middle_point_x = traffic_lights.middle_point_x;
    direction direction_to_turn = invalid;
    bool is_cube_present = false;

    /*while(cube == none){
        cube = traffic_lights.light_color;
        usleep(1000);
    }*/
    if(past_cube == light_red){
        if((middle_point_x < 0.5) && (cube != none) && ( cube != light_xparking)){
            is_cube_present = true;
            if( past_cube == cube ){
                Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
                printf("2 cubos rojos iguales\n");
            }else{
                printf("cubo rojo seguido de verde\n");
                if(cube == light_green){
                    calculte_angle_section_start_clockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = left;
                }else if(cube == light_red){
                    calculte_angle_section_start_clockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = right;
                }
                printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
                if(angle_to_wall > 70.0){ /* limit the angle in case of an emergency */
                    angle_to_wall = 70.0;
                }
                Spike_Turn_For_Degrees(direction_to_turn, 60,abs( angle_to_wall) -5 , 40);
                Spike_Center_Vehicle_Short();
                Advance_For_distance_Especial(80, (int)hypotenuse - 250, ((abs(angle_to_wall) - 5)*direction_to_turn*-1));
                Spike_Small_Turn((direction_to_turn * -1), 60, 0, 35);
                Spike_Center_Vehicle_Short();
                Spike_Coast_Motors();
            }
            
        }
        else{
            Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
            printf("no second cube\n");
        }
    }

    else if(past_cube == light_green){
        if((middle_point_x < 0.90) && ( cube != none) && ( cube != light_xparking)){
            is_cube_present = true;
            if( past_cube == cube ){
                if(parking == false){
                    Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
                }
                else{
                    Oradar_S2L_Advance_Until_Distance(80, 0, 1500, Hold);
                    Spike_Turn_For_Degrees(left, 60, 30, 40);
                    Spike_Small_Turn(right, 60, 0, 40);
                    Spike_Center_Vehicle_Short();
                }
                printf("2 cubos verdes iguales\n");
            }else{
                printf("cubo verde seguido de rojo\n");
                if(cube == light_green){
                    calculte_angle_section_start_clockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = left;
                }else if(cube == light_red){
                    //printf(" ========================== hey ==============\n");
                    calculte_angle_section_start_clockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = right;
                }
                printf("angulo: %f, hipotenusa: %f, direccion: %d\n",angle_to_wall, hypotenuse, direction_to_turn);
                if(angle_to_wall > 70.0){ /* limit the angle in case of an emergency */
                    angle_to_wall = 70.0;
                }
                //printf("yaw antes del giro derecha: %f\n", Spike_Get_Gyro());
                //usleep(10000000);
                Spike_Turn_For_Degrees(direction_to_turn, 60, abs(angle_to_wall) - 5, 40);
                Spike_Center_Vehicle_Short();
                if((parking == false)){// the parking flag matters here mainly for the red pillar in the parking section
                    Advance_For_distance_Especial(80, (int)hypotenuse - 250, (angle_to_wall*direction_to_turn*-1));
                }
                else{
                    printf("segundo cubo rojo en la seccion de parking\n");
                    Advance_For_distance_Especial(80, (int)hypotenuse - 260, ((abs(angle_to_wall) - 5)*direction_to_turn*-1));
                }
                Spike_Small_Turn((direction_to_turn * -1), 60, 0, 35);
                Spike_Center_Vehicle_Short();
                Spike_Coast_Motors();
            }
        }
        else{
            Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
            //printf("no second cube\n");
        }
    }

    if(is_cube_present == true){
        return cube;
    }
    else{
        return past_cube;
    }
}

// Clockwise run: drives to 1000 mm from the front wall, checks the camera for a pillar in
// the corner area and takes the right-hand corner turn (until |yaw| >= 86 deg).
// The colour seen sets how close to the front wall the robot gets before turning:
//   green after a red pillar -> 400 mm (600 mm when entering the parking section),
//   red after a red pillar   -> 990 mm (turn almost immediately),
//   anything else            -> 650 mm; no corner pillar, and *middle_cube is set to
//                               false after a green pillar, true otherwise.
//   middle_cube [out] : whether the next section is handled as a single middle pillar.
//   return            : colour of the corner pillar, or none when there is none.
Color_traffic_light Corner_Case(Color_traffic_light past_cube, bool *middle_cube, bool parking){
    bool is_corner_cube = true;
    printf("Case_corner\n");
    Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
    Color_traffic_light cube = traffic_lights.light_color;

    if((cube == light_green) && (past_cube == light_red)){
        printf("verde en la esquina\n");
        if(parking == false){
            printf("seccion normal\n");
            Oradar_S2L_Advance_Until_Distance(80, 0, 400, Hold); 
        }
        else{
            printf("entrando a seccion de parking\n");
            Oradar_S2L_Advance_Until_Distance(80, 0, 600, Hold);
        }

    }
    else if((cube == light_red) && (past_cube == light_red)){
        printf("rojo en la esquina\n");
        Oradar_S2L_Advance_Until_Distance(80, 0, 990, Hold);
    }
    else{
        printf("nada en la esquina\n");
        is_corner_cube = false;
        Oradar_S2L_Advance_Until_Distance(80, 0, 650, Hold);
        if(past_cube == light_green){
            *middle_cube = false;
        }
        else{
            *middle_cube = true;
        }
    }

    Spike_Turn_For_Degrees(right, 60, 86, 40,true);
    Spike_Center_Vehicle_Short();
    Spike_Coast_Motors();

    printf("=============================Case_corner_finish====================================\n");
    if(is_corner_cube == true){
        return cube;
    }
    else{
        return none;
    }
}

// Clockwise run: passes a single pillar in the middle row (CUBE_middle, 1450 mm from the
// rear wall): turn by angle + 3 deg, drive hypotenuse - 450 mm (red) or - 350 mm (green)
// holding that heading, then steer back to heading 0.
//   return : colour of the pillar that was passed.
Color_traffic_light esquivar_cubos_middle(bool parking){
    float angle_to_wall = 0;
    float hypotenuse = 0;
    Color_traffic_light cube = traffic_lights.light_color;
    direction direction_to_turn = invalid; 

    if(cube == light_green){
        printf("green cube\n");
        calculte_angle_section_start_clockwise_chr(cube, CUBE_middle, &angle_to_wall, &hypotenuse, parking);
        direction_to_turn = left;
    }
    
    else if(cube == light_red){
        printf("red cube\n");
        calculte_angle_section_start_clockwise_chr(cube, CUBE_middle, &angle_to_wall, &hypotenuse);
        direction_to_turn = right;
    }

    printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
    if(angle_to_wall > 70.0){ /* limit the angle in case of an emergency */
        angle_to_wall = 70.0;
    }
    Spike_Turn_For_Degrees(direction_to_turn, 60, abs(angle_to_wall) + 3, 40, true);
    printf("===============Turn finsih position================\n");
    //usleep(2000000);
    Spike_Center_Vehicle_Short();
    if(cube == light_red)
    {
        Spike_Advance_For_distance(80, (int)hypotenuse - 450, ((abs(angle_to_wall) + 3 )*direction_to_turn*-1));
    }
    else
    {
        Spike_Advance_For_distance(80, (int)hypotenuse - 350, ((abs(angle_to_wall) + 3 )*direction_to_turn*-1));
    }
    Spike_Small_Turn((direction_to_turn * -1), 60, 0, 40);
    Spike_Center_Vehicle_Short();
    Spike_Coast_Motors();

    return cube;
}

// Clockwise run: handles one straight section after a corner.
// First re-zeroes the gyro on the left (outer) wall: Slope(left) is close to +-90 deg when
// the robot is parallel to it, so +-90 is removed to get the misalignment, and heading 0
// becomes "parallel to the section". Then:
//   past_cube == none and middle_cube -> one middle pillar (esquivar_cubos_middle),
//   past_cube == none                 -> first and second pillars (esquivar_cubos_1 + _2),
//   otherwise (pillar at the corner)  -> only the second pillar (esquivar_cubos_2).
//   return : colour of the last pillar passed in the section.
Color_traffic_light Desicion(Color_traffic_light past_cube, bool middle_cube, bool parking){
    //usleep(2000000);
    printf("Entro a decision\n");
    Color_traffic_light cubo_temp;
    float slope = Slope(left);
    printf("reset angle before %f\n", slope);

    if(slope > 0){
        slope = slope - 90;
    }else{
        slope = slope + 90;
    }
    Spike_Reset_Gyro(slope);
    printf("reset angle %f\n", slope);
    usleep(300000);
    if(past_cube == none){
        if (middle_cube == true){
            printf("esquivando cubo del en medio\n");
            cubo_temp = esquivar_cubos_middle(parking);
            //printf("cubo del en medio salio\n");
        }
        else{
            printf("esquivando 2 cubos en la seccion\n");
            cubo_temp = esquivar_cubos_1(parking);
            cubo_temp = esquivar_cubos_2(cubo_temp, parking);
            //printf("2 cubos en la seccion salio\n");
        }
    }
    else{
        printf("esquivando segundo cubo porque habia uno en la esquina\n");
        cubo_temp = esquivar_cubos_2(past_cube, parking);
        //printf("segundo cubo porque habia uno en la esquina saliooo\n");
    }

    printf("==========================descision_finish=================================\n");

    return cubo_temp;
}

// Clockwise run: leaves the parking lot at the start of the Obstacle Challenge and passes
// the first pillar. Reverses while steering left to 16 deg, then drives forward turning
// right to 45 deg and waits up to 3 s for the camera to report a pillar.
//   red   -> turn right to 90 deg, drive at heading -90 to 480 mm from the wall ahead,
//            steer back to 0 and drive to 1050 mm from the front wall.
//   green -> steer back left in two steps, a small S-shift to the left, then drive to
//            1100 mm from the front wall.
//   return : colour seen (none if no pillar was seen within 3 s).
Color_traffic_light estacionamiento_clockwise(void){
    Spike_Turn_For_Degrees(left, -40, 16, 45, true);
    Spike_Center_Vehicle();
    Spike_Turn_For_Degrees(right, 60, 45, 40, true);
    usleep(300000);

    Color_traffic_light cube = Esperar_Color_De_Cubo(3000); // Hiwonder: the v6 wait never returned if no pillar was visible

    if(cube == light_red){
        Spike_Turn_For_Degrees(right, 60, 90, 40);
        Spike_Center_Vehicle_Short();
        Oradar_S2L_Advance_Until_Distance(80, -90, 480, Hold);
        Spike_Small_Turn(left, 60, 0, 40);
        Spike_Center_Vehicle_Short();
        Oradar_S2L_Advance_Until_Distance(80, 0, 1050, Hold);

    }
    else if(cube == light_green){
        Spike_Small_Turn(left, 60, 30, 15, true);
        Spike_Small_Turn(left, 60, 0, 40);
        Spike_Turn_For_Degrees(left, 60, 15, 40);
        Spike_Small_Turn(right, 60, 0, 40);
        Spike_Center_Vehicle_Short();
        Oradar_S2L_Advance_Until_Distance(80, 0, 1100, Hold);
    }

    return cube;
}

// ===================== Counter-clockwise counterparts =====================

// Same as calculte_angle_section_start_clockwise_chr but measuring the RIGHT wall
// instead of the left one (in a counter-clockwise run the turns and the heading
// correction use the right side, the outer wall). Green is still passed on its left and
// red on its right (fixed rule), but since the right side is now measured, the colour
// whose formula is "direct" (passing side == measured side) is red instead of green --
// so red uses side_2 = right - 185 (right - 400 in the parking section) and green uses
// the complement 1000 - right - 185.
// Parameters and outputs as in the clockwise version.
void calculte_angle_section_start_counterclockwise_chr(Color_traffic_light traffic_light_color, Cube_number_chr cube_number_per_section, float *angle, float *hypotenuse, bool parking){
    float angle_rad = 0;
    float diStanceToFrontWallmm = 0;
    float diStanceToRightWallmm = 0;
    float distanceToBackWallmm = 0;
    float lidar_shared_buffer[360];
    float distance_to_wall_front_temp = 0;
    float distance_to_wall_behind_temp = 0;
    float side_1 = 0;
    float side_2 = 0;
    usleep(100000);
    // Debug data capture used to diagnose/tune front-wall outlier filtering
    // (see Select_Wall's despike step) - re-enable to log more snapshots for comparison.
    // {
    //     const int debug_snapshot_id = Next_Debug_Snapshot_Id();
    //     const char *debug_tag = "calculte_angle_section_start_counterclockwise_chr";
    //     float lidar_debug_snapshot[360];
    //     Oradar_S2L_Get_Buffer(&lidar_debug_snapshot[0]);
    //     Save_Lidar_Debug_Snapshot(debug_snapshot_id, lidar_debug_snapshot, debug_tag);
    //     Save_Wall_Detection_Snapshot(debug_snapshot_id, debug_tag);
    // }
    while((distance_to_wall_front_temp == 0) && (terminating_main == 0)){ /* waiting until a valid wall is detected */
        distance_to_wall_front_temp = Distance_To_Wall(front);
        distance_to_wall_behind_temp = Distance_To_Wall(behind);
    }
    Oradar_S2L_Get_Buffer(&lidar_shared_buffer[0]);
    diStanceToFrontWallmm = distance_to_wall_front_temp;
        // The rear distance is estimated from the field length (3000 mm) minus the front
        // distance, instead of the rear-wall fit, which returns 0 when the rear wall is
        // out of range or has no clean fit.
    distanceToBackWallmm = 3000 - distance_to_wall_front_temp;
    diStanceToRightWallmm = lidar_shared_buffer[0]; // single raw ray at 0 = right

    if(cube_number_per_section == CUBE_first){
        side_1 = cube_number_per_section - distanceToBackWallmm;

        if(traffic_light_color == light_red){
            if(parking == false){
                side_2 = diStanceToRightWallmm - 185;
                printf("------------------ 400 ---------------------NO parking\n");
            }
            else{
                printf("------------------ 400 --------------------- parking\n");
                side_2 = diStanceToRightWallmm - 400;
            }
            angle_rad = atan(((side_1)/(side_2)));
        }else if(traffic_light_color == light_green){
            side_2 = 1000 - diStanceToRightWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }
    }

    else if(cube_number_per_section == CUBE_second){
        side_1 = diStanceToFrontWallmm - cube_number_per_section;

        if(traffic_light_color == light_red){
            if (parking == false){
                side_2 = diStanceToRightWallmm - 185;
            }
            else{
                side_2 = diStanceToRightWallmm - 400;
            }
            angle_rad = atan(((side_1)/(side_2)));
        }else if(traffic_light_color == light_green){
            side_2 = 1000 - diStanceToRightWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }
    }

    else{
        side_1 = cube_number_per_section - distanceToBackWallmm;

        if(traffic_light_color == light_red){
            if(parking == false){
                side_2 = diStanceToRightWallmm - 185;
            }
            else{
                side_2 = diStanceToRightWallmm - 400;
            }
            angle_rad = atan(((side_1)/(side_2)));
        }else if(traffic_light_color == light_green){
            side_2 = 1000 - diStanceToRightWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }
    }

    *hypotenuse = sqrt( pow(side_1,2) + pow(side_2,2));
    *angle = 90 - Oradar_S2L_Radians_To_Degrees(angle_rad);
    printf("[ccw] distancia frente: %f, distancia atras: %f, distancia derecha: %f, angle rad: %f\n", diStanceToFrontWallmm, distanceToBackWallmm, diStanceToRightWallmm, angle_rad);
    //usleep(5000000);
}

// Same structure as esquivar_cubos_1: the green -> left, red -> right rule for
// direction_to_turn does not change (it is the traffic rule); it calls the
// counter-clockwise angle/hypotenuse calculation, and the distance margin subtracted from
// the hypotenuse scales with angle / 45 deg (400 mm, or 200 mm for a red pillar in the
// parking section).
Color_traffic_light esquivar_cubos_1_counterclockwise(bool parking){
    float angle_to_wall = 0;
    float hypotenuse = 0;
    Color_traffic_light cube = traffic_lights.light_color;
    direction direction_to_turn = invalid;

    if(cube == light_green){
        printf("green cube\n");
        calculte_angle_section_start_counterclockwise_chr(cube, CUBE_first, &angle_to_wall, &hypotenuse, parking);
        direction_to_turn = left;
    }else if(cube == light_red){
        printf("red cube\n");
        calculte_angle_section_start_counterclockwise_chr(cube, CUBE_first, &angle_to_wall, &hypotenuse, parking);
        direction_to_turn = right;
    }
    printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
    if(angle_to_wall > 70.0){ /* limit the angle in case of an emergency */
        angle_to_wall = 70.0;
    }
    Spike_Turn_For_Degrees(direction_to_turn, 60, abs(angle_to_wall) + 5, 40);
    usleep(100000);
    Spike_Center_Vehicle_Short();
    if((parking == false) || (cube == light_green)){
        Spike_Advance_For_distance(80, (int)hypotenuse - (400*(angle_to_wall/45)), ((abs(angle_to_wall)) *direction_to_turn*-1));
    }
    else{
        printf("caso verde en la seccion de parking\n");
        Spike_Advance_For_distance(80, (int)hypotenuse - (200*(angle_to_wall/45)) , (angle_to_wall*direction_to_turn*-1));
    }
    Spike_Small_Turn((direction_to_turn * -1), 60, 0, 40);
    Spike_Center_Vehicle_Short();

    return cube;
}

// Counter-clockwise counterpart of esquivar_cubos_2, using the counter-clockwise
// angle/hypotenuse calculation with margins scaled by angle / 45 deg. After a red pillar
// any red/green detection counts as a second pillar (no image-position check); after a
// green one the detection must be at normalised x > 0.5. Two reds in the parking section
// end with a short S-shift to the right; a green -> red crossing in the parking section
// ends with a short advance and the same S-shift.
Color_traffic_light esquivar_cubos_2_counterclockwise( Color_traffic_light past_cube, bool parking){
    float angle_to_wall = 0;
    float hypotenuse = 0;
    Color_traffic_light cube = traffic_lights.light_color;
    float middle_point_x = traffic_lights.middle_point_x;
    direction direction_to_turn = invalid;
    bool is_cube_present = false;

    if(past_cube == light_red){
        if(/*(middle_point_x > 0.1) && *//* position check removed: the pillar was sometimes not detected there */(cube != none) && ( cube != light_xparking)){
            is_cube_present = true;
            if( past_cube == cube ){
                if(parking == false){
                    Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
                }
                else{
                    printf("                  vehicle_vuleta especial \n");
                    Oradar_S2L_Advance_Until_Distance(80, 0, 1175, Hold);
                    Spike_Turn_For_Degrees(right, 60, 30, 40);
                    Spike_Small_Turn(left, 60, 0, 40);
                    Spike_Center_Vehicle_Short();
                    
                }
                printf("2 cubos rojos iguales\n");
            }else{
                printf("cubo rojo seguido de verde\n");
                if(cube == light_green){
                    calculte_angle_section_start_counterclockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = left;
                }else if(cube == light_red){
                    calculte_angle_section_start_counterclockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = right;
                }
                printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
                if(angle_to_wall > 70.0){ /* limit the angle in case of an emergency */
                    angle_to_wall = 70.0;
                }
                Spike_Turn_For_Degrees(direction_to_turn, 60,abs( angle_to_wall)  , 40);
                Spike_Center_Vehicle_Short();
                if((parking == false)){
                    Advance_For_distance_Especial(80, (int)hypotenuse - (250*(angle_to_wall/45)), (angle_to_wall*direction_to_turn*-1));
                }
                else{
                    printf("segundo cubo rojo en la seccion de parking\n");
                    Advance_For_distance_Especial(80, (int)hypotenuse - (280*(angle_to_wall/45)), ((abs(angle_to_wall) - 5)*direction_to_turn*-1));
                }
                Spike_Small_Turn((direction_to_turn * -1), 60, 0, 35);
                Spike_Center_Vehicle_Short();
                Spike_Coast_Motors();
            }

        }
        else{
            Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
            printf("no second cube\n");
        }
    }

    else if(past_cube == light_green){
        if((middle_point_x > 0.5) && ( cube != none) && ( cube != light_xparking)){
            is_cube_present = true;
            if( past_cube == cube ){
                Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
                printf("2 cubos verdes iguales\n");
            }else{
                printf("cubo verde seguido de rojo\n");
                if(cube == light_green){
                    calculte_angle_section_start_counterclockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = left;
                }else if(cube == light_red){
                    calculte_angle_section_start_counterclockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse, parking);
                    direction_to_turn = right;
                }
                printf("angulo: %f, hipotenusa: %f, direccion: %d\n",angle_to_wall, hypotenuse, direction_to_turn);
                if(angle_to_wall > 70.0){ /* limit the angle in case of an emergency */
                    angle_to_wall = 70.0;
                }   
                Spike_Turn_For_Degrees(direction_to_turn, 60, abs(angle_to_wall) , 40);
                Spike_Center_Vehicle_Short();
                Advance_For_distance_Especial(80, (int)hypotenuse - (270*(angle_to_wall/45)), (angle_to_wall*direction_to_turn*-1));
                Spike_Small_Turn((direction_to_turn * -1), 60, 0, 35);
                Spike_Center_Vehicle_Short();
                Spike_Coast_Motors();
                if (parking == true){
                    //usleep(10000000);
                    Spike_Advance_For_Degrees(50,200,0);
                    Spike_Turn_For_Degrees(right, 60, 30, 40);
                    Spike_Small_Turn(left, 60, 0, 40);
                    Spike_Center_Vehicle_Short();
                }
            }
        }
        else{
            Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
        }
    }

    if(is_cube_present == true){
        return cube;
    }
    else{
        return past_cube;
    }
}

// Counter-clockwise counterpart of Corner_Case: the corner turn is taken to the left
// instead of the right, and the colours are swapped: red after a green pillar -> stop at
// 400 mm (600 mm entering the parking section), green after a green pillar -> 990 mm,
// otherwise 650 mm with *middle_cube = false after a red pillar, true otherwise.
// The distance thresholds are the same as in the clockwise version.
Color_traffic_light Corner_Case_counterclockwise(Color_traffic_light past_cube, bool *middle_cube, bool parking){
    bool is_corner_cube = true;
    printf("Case_corner (ccw)\n");
    Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
    Color_traffic_light cube = traffic_lights.light_color;

    if((cube == light_red) && (past_cube == light_green)){
        printf("rojo en la esquina counterclockwise\n");
        if(parking == false){
            printf("seccion normal\n");
            Oradar_S2L_Advance_Until_Distance(80, 0, 400, Hold);
        }
        else{
            printf("entrando a seccion de parking\n");
            Oradar_S2L_Advance_Until_Distance(80, 0, 600, Hold);
        }

    }
    else if((cube == light_green) && (past_cube == light_green)){
        printf("verde en la esquina ccw\n");
        Oradar_S2L_Advance_Until_Distance(80, 0, 990, Hold);
    }
    else{
        printf("nada en la esquina\n");
        is_corner_cube = false;
        Oradar_S2L_Advance_Until_Distance(80, 0, 650, Hold);
        if(past_cube == light_red){
            *middle_cube = false;
        }
        else{
            *middle_cube = true;
        }
    }

    Spike_Turn_For_Degrees(left, 60, 86, 40,true);
    Spike_Center_Vehicle_Short();
    Spike_Coast_Motors();

    printf("=============================Case_corner_finish (ccw)====================================\n");
    if(is_corner_cube == true){
        return cube;
    }
    else{
        return none;
    }
}

// Counter-clockwise counterpart of esquivar_cubos_middle: uses the counter-clockwise
// angle/hypotenuse calculation, turns by angle + 5 deg, and the margin is
// 450 mm (green) or 350 mm (red) scaled by angle / 45 deg.
Color_traffic_light esquivar_cubos_middle_counterclockwise(bool parking){
    float angle_to_wall = 0;
    float hypotenuse = 0;
    Color_traffic_light cube = traffic_lights.light_color;
    direction direction_to_turn = invalid;

    if(cube == light_green){
        printf("green cube\n");
        calculte_angle_section_start_counterclockwise_chr(cube, CUBE_middle, &angle_to_wall, &hypotenuse, parking);
        direction_to_turn = left;
    }

    else if(cube == light_red){
        printf("red cube\n");
        calculte_angle_section_start_counterclockwise_chr(cube, CUBE_middle, &angle_to_wall, &hypotenuse);
        direction_to_turn = right;
    }

    printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
    if(angle_to_wall > 70.0){ /* limit the angle in case of an emergency */
        angle_to_wall = 70.0;
    }
    Spike_Turn_For_Degrees(direction_to_turn, 60, abs(angle_to_wall) + 5, 40, true);
    printf("===============Turn finsih position (ccw)================\n");
    Spike_Center_Vehicle_Short();
    if(cube == light_green)
    {
        Spike_Advance_For_distance(80, (int)hypotenuse - (450*(angle_to_wall/45)), ((abs(angle_to_wall) + 3 )*direction_to_turn*-1));
    }
    else
    {
        Spike_Advance_For_distance(80, (int)hypotenuse - (350*(angle_to_wall/45)), ((abs(angle_to_wall) + 3 )*direction_to_turn*-1));
    }
    Spike_Small_Turn((direction_to_turn * -1), 60, 0, 40);
    Spike_Center_Vehicle_Short();
    Spike_Coast_Motors();

    return cube;
}

// Same as Desicion, except that it calls the _counterclockwise variants of the
// esquivar_cubos_* functions and uses Slope(right) instead of Slope(left) -- in a
// counter-clockwise run the heading correction uses the right (outer) LiDAR side.
Color_traffic_light Desicion_counterclockwise(Color_traffic_light past_cube, bool middle_cube, bool parking){

    printf("Entro a decision (ccw)\n");
    Color_traffic_light cubo_temp;
    float slope = Slope(right);
    printf("reset angle before %f\n", slope);
    if(slope > 0){
        slope = slope - 90;
    }else{
        slope = slope + 90;
    }
    Spike_Reset_Gyro(slope);
    printf("reset angle %f\n", slope);
    usleep(200000);
    if(past_cube == none){
        if (middle_cube == true){
            printf("esquivando cubo del en medio\n");
            cubo_temp = esquivar_cubos_middle_counterclockwise(parking);
        }
        else{
            printf("esquivando 2 cubos en la seccion\n");
            cubo_temp = esquivar_cubos_1_counterclockwise(parking);

            cubo_temp = esquivar_cubos_2_counterclockwise(cubo_temp, parking);
        }
    }
    else{
        printf("esquivando segundo cubo porque habia uno en la esquina\n");
        cubo_temp = esquivar_cubos_2_counterclockwise(past_cube, parking);
        
    }

    printf("==========================descision_finish (ccw)=================================\n");

    return cubo_temp;
}

// Counter-clockwise counterpart of estacionamiento_clockwise (leaving the parking lot
// at the start of the run). Unlike the functions above, this is a fixed sequence of
// turns tuned by hand for the physical geometry of the parking lot, not a formula
// derived from the LiDAR. The first two moves mirror the clockwise version
// (left <-> right); the colour branches are their own hand-tuned sequences:
//   red   -> steer right back to 35 deg and then to 0 deg.
//   green -> turn left to 120 deg, steer right back to 0, drive to 800 mm from the
//            front wall, then reverse 600 hub degrees.
//   none  -> same moves as the red branch.
// The colour that triggers each branch follows the fixed rule (green passed on the
// left, red on the right).
//   return : colour seen (none if no pillar was seen within 3 s).
Color_traffic_light estacionamiento_counterclockwise(void){
    Spike_Turn_For_Degrees(right, -40, 16, 45, true);
    Spike_Center_Vehicle();
    Spike_Turn_For_Degrees(left, 45, 45, 40, true);
    //(usleep(1500000);

    Color_traffic_light cube = Esperar_Color_De_Cubo(3000); // Hiwonder: the v6 wait never returned if no pillar was visible
    if(cube == light_red){
        printf("vio rojo\n");
        Spike_Small_Turn(right, 60, 35, 15,true);
        Spike_Small_Turn(right, 60, 0, 40, true);
        Spike_Center_Vehicle_Short();
        //Spike_Advance_For_Degrees(-80, 200, 90);
        //Spike_Break_Motors();
        //Spike_Turn_For_Degrees(right, 60, 15, 40, true);
        //Spike_Small_Turn(left, 60, 0, 40);
        //Oradar_S2L_Advance_Until_Distance(80, 0, 1100, Hold);

    }
    else if(cube == light_green){
        printf("vio verde\n");

        Spike_Turn_For_Degrees(left, 60, 120, 45);
        Spike_Small_Turn(right, 60, 0, 45, true);
        Spike_Center_Vehicle();
        Oradar_S2L_Advance_Until_Distance(80, 0, 800, Hold);
        Spike_Advance_For_Degrees(-80, 600, 0);

    }else{
        printf("no vio nada pero se acomoda como rojo\n");
        Spike_Small_Turn(right, 60, 35, 15,true);
        Spike_Small_Turn(right, 60, 0, 40, true);
        Spike_Center_Vehicle_Short();
        //usleep(5000000);
    }

    return cube;
}

// Hiwonder: the hub's ag() (drive a distance holding a heading) while watching the
// LiDAR. The ray at buffer index 270 - reference (270 = robot front, offset by the
// mission heading reference) stops the move when it reads 370 mm or less: the computed
// hypotenuse was too long and the robot is already close to the wall/pillar ahead.
// Difference from the v6 code: a 0 at that index (no reading) no longer counts as
// "below 370 mm" and does not abort the move.
//   speed     : drive speed in hub units (%), negative = reverse.
//   distance  : distance to drive, mm (converted to hub degrees with kMmPorGradoSpike).
//   reference : heading to hold, deg (gyro frame, positive = left).
// Blocks until the distance is covered or the LiDAR stop triggers; ends holding the motor.
void Advance_For_distance_Especial(int speed, int distance, int reference){
    const int indice = (((270 - reference) % 360) + 360) % 360;
    float lidar_shared_buffer[360];
    const bool detenido = spike_compat::Advance_For_Degrees_Until(
        speed, (int)std::lround(distance / kMmPorGradoSpike), reference,
        [&]() {
            Oradar_S2L_Get_Buffer(&lidar_shared_buffer[0]);
            const float distance_i = lidar_shared_buffer[indice];
            return (distance_i > 0.0f) && (distance_i <= 370.0f);
        });
    if(detenido){
        printf("mucha hipotenusa, se paro antes de completarla\n");
    }
    Spike_Hold_Motors();
}

// SIGINT (Ctrl+C) handler: sets terminating_main so every mission/LiDAR loop exits and
// the shutdown path runs, then restores the default handler so a second Ctrl+C kills
// the process immediately.
void signal_handler(int signum){
    printf("\nCtrl+C detceted\n");
    terminating_main = 1;
    signal(SIGINT, SIG_DFL);
}
