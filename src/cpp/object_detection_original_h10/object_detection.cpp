#include "toolbox.hpp"
#include "hailo_infer.hpp"
#include "utils.hpp"

#include "spike.h"
#include "rasp_gpio.h"
#include "Oradar_S2L.h"
#include "common_var.h"

#include <cstdlib>
#include <string>

struct traffic_lights_struct{
    float  middle_point_x;
    float  middle_point_y;
    Color_traffic_light light_color; /* red = 2,  green = 1, xparking = 3 */
    float area;
    float confidence;
};

static traffic_lights_struct traffic_lights;

// Set from args.no_display in main() so postprocess_callback (which doesn't receive
// CommandLineArgs) can skip opening the lidar visualization window too.
static bool g_no_display = false;

pthread_t writer;
pthread_t Main_Actions;


// segment_id -> persistent color slot for the most recently detected walls, refreshed
// every new lidar scan in postprocess_callback. Global so other code can inspect which
// color slot a given wall segment currently holds.
std::unordered_map<int, int> wall_rank;

void signal_handler(int signum);
void *Obstacle_Challenge_Thread(void *arg);
void Follow_cubes(int vel, float referencia_2, float area);
float Slope(const direction side);
float Distance_To_Wall(const direction side);
void Print_Slope(const direction side);
void Follow_Wall(const direction side);
float calculte_angle_section_start_clockwise(Color_traffic_light traffic_light_color, int cube_number_per_section);
float calculte_angle_section_start_counterclockwise(Color_traffic_light traffic_light_color, int cube_number_per_section);
static float Fit_Line_Orientation(const std::vector<cv::Point2f> &pts, const std::vector<int> &indices);
void avoid_cube_start_section(Color_traffic_light traffic_light_color, Cube_number cube_number_per_section);

Color_traffic_light esquivar_cubos_1(void);
Color_traffic_light esquivar_cubos_2( Color_traffic_light past_cube);
void calculte_angle_section_start_clockwise_chr(Color_traffic_light traffic_light_color, Cube_number_chr cube_number_per_section, float *angle, float *hypotenuse);
Color_traffic_light Corner_Case(Color_traffic_light past_cube, bool *middle_cube);
Color_traffic_light esquivar_cubos_middle(void);
Color_traffic_light Desicion(Color_traffic_light past_cube, bool middle_cube);

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

// Task-specific preprocessing callback
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

// Task-specific postprocessing callback
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
        // At 5deg, D_max for a ~450mm-range point was ~142mm: generous enough that a
        // real object sitting near a wall wouldn't get split off as its own segment.
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
            { front, "front", cv::Scalar(0, 0, 255) },  // red
            { right, "right", cv::Scalar(255, 0, 0) },  // blue
            { left,  "left",  cv::Scalar(0, 200, 0) },  // green
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
        if(named_bbox.bbox.score < 0.75)
        { // check the score, if the score is below 0.65 make the area 0 to discard in the following steps
            traffic_light_area[index] = 0;
        }
        index++;
    }
    int max_index = find_max_index(traffic_light_area, 10);
    traffic_lights.light_color = none;
    if (bboxes.size() > 0){
        const auto named_bbox = bboxes.at(max_index); // Assuming only one bbox for traffic light detection
        
        traffic_lights.middle_point_x = (named_bbox.bbox.x_min + named_bbox.bbox.x_max) / 2.0;
        traffic_lights.middle_point_y = (named_bbox.bbox.y_min + named_bbox.bbox.y_max) / 2.0;
        traffic_lights.light_color = (Color_traffic_light)named_bbox.class_id;
        traffic_lights.area = traffic_light_area[max_index];
        traffic_lights.confidence = named_bbox.bbox.score;
        //printf(" color = %d, area = %f, middle point = (%f, %f), confidence = %f", traffic_lights.light_color, traffic_lights.area, traffic_lights.middle_point_x, traffic_lights.middle_point_y, traffic_lights.confidence);
        //printf("\n");
    }
    else{
        //printf("color: %d\n",traffic_lights.light_color);
    }
}


int main(int argc, char** argv)
{

    signal(SIGINT, signal_handler); /* Set interrupt for ctrl+C */
    Oradar_S2L_Init_Lidar();
    pthread_create(&writer, NULL, Oradar_S2L_Lidar_Writer_Thread, NULL);

    Rasp_Gpio_Init();
    Rasp_Gpio_Power_On_Spike();

    Spike_Serial_Init();
    Spike_Interpreter();
    Spike_Initialize_Libraries();
    pthread_create(&Main_Actions, NULL, Obstacle_Challenge_Thread, NULL);
    
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
            : "/home/maker/WRO_Hailo10H_Compatible/software/cpp/object_detection_original_h10/visualization_config.yaml";
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

        hailo_status status = wait_and_check_threads(
            preprocess_thread,    "Preprocess",
            inference_thread,     "Inference",
            output_parser_thread, "Postprocess "
        );
        if (HAILO_SUCCESS != status) {
            return status;
        }
        
        auto t_end = Clock::now();
        print_inference_statistics(inference_time, args.net, static_cast<double>(frame_count), t_end - t_start);

        return HAILO_SUCCESS;
    }
    catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return HAILO_INTERNAL_FAILURE;
    }
}

void *Obstacle_Challenge_Thread(void *arg){
    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;
    float lidar_shared_buffer[360]; // Your shared buffer
    Color_traffic_light cubo_temp;
    bool is_middle_case = false;

    Oradar_S2L_Get_Buffer(&lidar_shared_buffer[0]);
    distancia_frente = lidar_shared_buffer[90];
    distancia_derecha = lidar_shared_buffer[0];
    distancia_izquierda = lidar_shared_buffer[180];

    printf("dsitancia derecha : %f\n", distancia_derecha);
    printf("dsitancia izquierda : %f\n", distancia_izquierda);
    printf("dsitancia frente : %f\n", distancia_frente);

    Spike_Reset_Gyro(0);
    Rasp_Gpio_Wait_For_Button();
    usleep(200000); //wiating for reset gyro
    float slope = Slope(front);
    Spike_Reset_Gyro(0);
    Spike_Center_Vehicle_Short();

    //float slope = Slope(front);
    //printf("pendiente: %f\n", slope);
    //Spike_Reset_Gyro(slope);
    //Oradar_S2L_Advance_Until_Distance(50,0,150,Hold);


    //float slope = Slope(front);
    //Spike_Reset_Gyro(slope);
    /*Corner_Case(light_red, &is_middle_case);
    float slope = Slope(front);
    Spike_Reset_Gyro(slope);
    usleep(200000);
    if(is_middle_case == true){
        esquivar_cubos_middle();
    }
    else{
        printf("no cubo del medio\n");
    }*/
    cubo_temp = esquivar_cubos_1();
    cubo_temp = esquivar_cubos_2(cubo_temp);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);
    cubo_temp = Corner_Case(cubo_temp, &is_middle_case);
    cubo_temp = Desicion(cubo_temp, is_middle_case);

    slope = Slope(front);
    Spike_Reset_Gyro(slope);
    usleep(200000);
    cubo_temp = esquivar_cubos_1();
    esquivar_cubos_2(cubo_temp);
    //printf("color de cubo: %d\n",cubo );
    //esquivar_cubos((Color_traffic_light)traffic_lights.light_color,CUBE_second,cubo_temp );
    //avoid_cube_start_section((Color_traffic_light)traffic_lights.light_color, cube_first););
    //alculte_angle_section_start_clockwise_chr((Color_traffic_light)traffic_lights.light_color,CUBE_first);
    //avoid_cube_start_section((Color_traffic_light)traffic_lights.light_color, cube_first);
    //usleep(200000); //wiating for reset gyro
    //Spike_Center_Vehicle_Short();
    //usleep(200000); //wiating for reset gyro
    //avoid_cube_start_section((Color_traffic_light)traffic_lights.light_color, cube_second);
    
    /*if(distancia_derecha > 600){
        printf("Sentido horario\n");
        Spike_Turn_For_Degrees(right, 60, 45, 40, true);
        Spike_Center_Vehicle_Short();

    }

    else if (distancia_izquierda > 600){
        printf("Sentido antihorario\n");
        Spike_Turn_For_Degrees(left, 60, 45, 40, true);
        Spike_Center_Vehicle_Short();
    
    }*/


//programa de christian
   /*Follow_cubes(60, 0.5, 0.06);
   usleep(2000000);
   float grados = Spike_Get_Gyro();
   usleep(2000000);
   Spike_Small_Turn(left, 60, 0, 30);
   usleep(2000000);
   Spike_Turn_For_Degrees(left, 60, grados + 30, 30);
   usleep(2000000);
   Spike_Center_Vehicle_Short();
   usleep(2000000);
   Spike_Advance_For_Degrees(60,30*abs(grados), 30);
   usleep(2000000);
   Spike_Small_Turn(right, 60, 0, 30);
   usleep(2000000);
   Spike_Center_Vehicle_Short();
   usleep(2000000);
   Follow_cubes(60, 0.5, 0.06);  
   usleep(2000000);
   grados = Spike_Get_Gyro();
   usleep(2000000);
   
   Spike_Turn_For_Degrees(right, 60, (grados*-1) + 30, 30);
   Spike_Center_Vehicle_Short();
   Spike_Advance_For_Degrees(60,30*abs(grados), -30);
   Spike_Small_Turn(left, 60, 0, 30);
   Spike_Center_Vehicle_Short();*/


   

   


    while(terminating_main == 0){
        /*Oradar_S2L_Get_Buffer(&lidar_shared_buffer[0] );
        distancia_izquierda = lidar_shared_buffer[90];
        printf("dsitancia izquierda : %f\n", distancia_izquierda);*/
        usleep(1000);
    }

    /* send Ctrl + C to stop subprocess in spike brick */
    char control_c = '\003';
    char msg[10] = "";
    msg[0] = control_c;
    msg[1] = '\r';
    Spike_Send_Serial_Data(msg);
    Spike_Send_Serial_Data("\r");

    Spike_Coast_Motors();
    Oradar_S2L_Set_Terminating();
    Rasp_Gpio_Clean();
    Spike_Close_Serial();
    Oradar_S2L_Close();
    return NULL;
}

void Follow_cubes(int vel, float referencia_2, float area){

    while((terminating_main == 0) && (traffic_lights.area < area) && (traffic_lights.confidence > 0.65) ){
        Spike_Follow_Reference(vel, traffic_lights.middle_point_x, referencia_2);
        usleep(1000);
        printf("area : %f\n", traffic_lights.area);
    }
    printf("area : %f , confidencia: %f, terminating : %d\n", traffic_lights.area, traffic_lights.confidence, terminating_main );
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

// Calculates the slope using only the two best walls found around middle_point: "best"
// meaning the most points and the cleanest (lowest-residual) fit to a straight line, so
// a noisy point, a stray outlier, or a corner artifact in the window doesn't skew the
// result. The points themselves are no longer passed in - this fetches the current
// lidar buffer and runs wall detection (gap + corner segmentation) over the requested
// angular window itself, scoped to [middle_point - point, middle_point).

static WallSelection Select_Wall(const direction side) {
    WallSelection result;

    // The window's center angle and width are fully determined by which side of the
    // robot we're looking at - callers just say which side, not raw lidar angles.
    int middle_point = 0;
    // The Oradar's raw zero-reference sits 90deg off from the robot's actual front
    // (confirmed empirically: testing "right" lit up the wall the robot is squarely
    // facing). front/right are swapped from the naive 0/270 guess; left=180 and
    // behind=90 were already correct from an earlier empirical fix.
    switch (side) {
        case right:  middle_point = 0;   break;
        case left:   middle_point = 180; break;
        case front:  middle_point = 270; break;
        case behind: middle_point = 90;  break;
        default:
            printf("Pendiente: invalid side\n");
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
        printf("Pendiente: not enough valid lidar points (%zu)\n", pts.size());
        return result;
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
        printf("Pendiente: not enough valid lidar points after outlier removal (%zu)\n", pts.size());
        return result;
    }

    // Gap-based segmentation: a new wall starts wherever consecutive points jump apart
    const float gap_threshold_mm = 60.0f;
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

float Slope(const direction side) {
    WallSelection sel = Select_Wall(side);
    if (!sel.found) return 0;
    return Fit_Line_Orientation(sel.pts, sel.chosen_indices);
}

// Perpendicular distance from the robot (origin) to the selected wall's fitted line -
// not just the centroid's range, since the centroid can sit off to one side of the
// perpendicular foot. Mirrors the highlighted-wall distance shown in the visualization.
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
 * description: This function calculates the slope of the line formed by the points detected by the lidar
 * return: void
 * parameters: direction side
 * side: which side of the robot to look at - right, left, front, or behind
 */
void Print_Slope(const direction side){
    float pendiente = Slope(side);
    printf("Pendiente: %f\n", pendiente);
}

/**
 * function name: Follow_Wall
 * description: This function follows the wall maintaining an angle of 0 with respect to it
 * return: void
 * parameters: direction side
 * side: which side of the robot to look at - right, left, front, or behind
 */
void Follow_Wall(const direction side){
    while(terminating_main == 0){
        float pendiente = Slope(side);
        printf("Pendiente: %f\n", pendiente);

        Spike_Follow_Reference(60, 0 ,pendiente);
    }
}

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
    printf("distancia frente: %f, distancia izquierda: %f, angle rad: %f, angle deg: %f\n", distanceToFrontWallmm, diStanceToLeftWallmm, angle_rad, angle);
    return angle;
}

void calculte_angle_section_start_clockwise_chr(Color_traffic_light traffic_light_color, Cube_number_chr cube_number_per_section, float *angle, float *hypotenuse){
    float angle_rad = 0;
    float diStanceToFrontWallmm = 0;
    float diStanceToLeftWallmm = 0;
    float distanceToBackWallmm = 0;
    float lidar_shared_buffer[360];
    float side_1 = 0;
    float side_2 = 0;

    Oradar_S2L_Get_Buffer(&lidar_shared_buffer[0]);
    diStanceToFrontWallmm = lidar_shared_buffer[270];
    distanceToBackWallmm = lidar_shared_buffer[90];
    diStanceToLeftWallmm = lidar_shared_buffer[180];
    
    //printf("atras: %f\n", distanceToBackWallmm);

    if(cube_number_per_section == CUBE_first){
        printf("primer cubo\n");
        side_1 = cube_number_per_section - distanceToBackWallmm;
    
        if(traffic_light_color == light_green){
            printf("cubo verde\n");
            side_2 = diStanceToLeftWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }else if(traffic_light_color == light_red){
            printf("cubo rojo\n");
            side_2 = 1000 - diStanceToLeftWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }
    }

    else if(cube_number_per_section == CUBE_second){
        printf("segundo cubo");
        side_1 = diStanceToFrontWallmm - cube_number_per_section;
    
        if(traffic_light_color == light_green){
            printf("cubo verde\n");
            side_2 = diStanceToLeftWallmm - 305;
            angle_rad = atan(((side_1)/(side_2)));
        }else if(traffic_light_color == light_red){
            printf("cubo rojo\n");
            side_2 = 1000 - diStanceToLeftWallmm - 305;
            angle_rad = atan(((side_1)/(side_2)));
        }
    }

    else{
        printf("cubo del medio\n");
        side_1 = cube_number_per_section - distanceToBackWallmm;
    
        if(traffic_light_color == light_green){
            printf("cubo verde\n");
            side_2 = diStanceToLeftWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }else if(traffic_light_color == light_red){
            printf("cubo rojo\n");
            side_2 = 1000 - diStanceToLeftWallmm - 185;
            angle_rad = atan(((side_1)/(side_2)));
        }
    }

    *hypotenuse = sqrt( pow(side_1,2) + pow(side_2,2));
    *angle = 90 - Oradar_S2L_Radians_To_Degrees(angle_rad);
    printf("distancia frente: %f, distancia atras: %f, distancia izquierda: %f, angle rad: %f\n", diStanceToFrontWallmm, distanceToBackWallmm, diStanceToLeftWallmm, angle_rad);
}

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

Color_traffic_light esquivar_cubos_1(void){
    float angle_to_wall = 0;
    float hypotenuse = 0;
    Color_traffic_light cube = traffic_lights.light_color;
    direction direction_to_turn = invalid;

    if(cube == light_green){
        calculte_angle_section_start_clockwise_chr(cube, CUBE_first, &angle_to_wall, &hypotenuse);
        direction_to_turn = left;
    }else if(cube == light_red){
        calculte_angle_section_start_clockwise_chr(cube, CUBE_first, &angle_to_wall, &hypotenuse);
        direction_to_turn = right;
    }
    //printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
    Spike_Turn_For_Degrees(direction_to_turn, 60, angle_to_wall, 30);
    Spike_Center_Vehicle_Short();
    Spike_Advance_For_distance(80, (int)hypotenuse - 400, (angle_to_wall*direction_to_turn*-1));
    Spike_Small_Turn((direction_to_turn * -1), 60, 0, 30);
    Spike_Center_Vehicle_Short();
    Spike_Coast_Motors();
    
    return cube;
}

Color_traffic_light esquivar_cubos_2( Color_traffic_light past_cube){
    float angle_to_wall = 0;
    float hypotenuse = 0;
    Color_traffic_light cube = traffic_lights.light_color;
    float middle_point_x = traffic_lights.middle_point_x;
    direction direction_to_turn = invalid;
    bool is_cube_present = false;

    if(past_cube == light_red){
        if((middle_point_x < 0.5) && (cube != none) && ( cube != light_xparking)){
            is_cube_present = true;
            if( past_cube == cube ){
                Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
                printf("cubos iguales\n");
            }else{

                if(cube == light_green){
                    calculte_angle_section_start_clockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = left;
                }else if(cube == light_red){
                    calculte_angle_section_start_clockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = right;
                }
                printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
                Spike_Turn_For_Degrees(direction_to_turn, 60, angle_to_wall, 30);
                Spike_Center_Vehicle_Short();
                Spike_Advance_For_distance(80, (int)hypotenuse - 400, (angle_to_wall*direction_to_turn*-1));
                Spike_Small_Turn((direction_to_turn * -1), 60, 0, 30);
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
        if((middle_point_x < 0.83) && ( cube != none) && ( cube != light_xparking)){
            is_cube_present = true;
            if( past_cube == cube ){
                Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
                printf("cubos iguales\n");
            }else{

                if(cube == light_green){
                    calculte_angle_section_start_clockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = left;
                }else if(cube == light_red){
                    calculte_angle_section_start_clockwise_chr(cube, CUBE_second, &angle_to_wall, &hypotenuse);
                    direction_to_turn = right;
                }
                printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
                Spike_Turn_For_Degrees(direction_to_turn, 60, angle_to_wall, 30);
                Spike_Center_Vehicle_Short();
                Spike_Advance_For_distance(80, (int)hypotenuse - 400, (angle_to_wall*direction_to_turn*-1));
                Spike_Small_Turn((direction_to_turn * -1), 60, 0, 30);
                Spike_Center_Vehicle_Short();
                Spike_Coast_Motors();
            }
        }
        else{
            Oradar_S2L_Advance_Until_Distance(80, 0, 1000, Hold);
            printf("no second cube\n");
        }
    }

    if(is_cube_present == true){
        return cube;
    }
    else{
        return past_cube;
    }
}

Color_traffic_light Corner_Case(Color_traffic_light past_cube, bool *middle_cube){
    Oradar_S2L_Advance_Until_Distance(80, 0, 1100, Hold);
    Color_traffic_light cube = traffic_lights.light_color;

    if(cube == light_green){
        printf("verde en la esquina\n");
       Oradar_S2L_Advance_Until_Distance(80, 0, 400, Hold); 

    }
    else if(cube == light_red){
        printf("rojo en la esquina\n");
        Oradar_S2L_Advance_Until_Distance(80, 0, 990, Hold);
    }
    else{
        printf("nada en la esquina\n");
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

    return cube;
}

Color_traffic_light esquivar_cubos_middle(void){
    float angle_to_wall = 0;
    float hypotenuse = 0;
    Color_traffic_light cube = traffic_lights.light_color;
    direction direction_to_turn = invalid; 

    if(cube == light_green){
        calculte_angle_section_start_clockwise_chr(cube, CUBE_middle, &angle_to_wall, &hypotenuse);
        direction_to_turn = left;
    }
    
    else if(cube == light_red){
        calculte_angle_section_start_clockwise_chr(cube, CUBE_middle, &angle_to_wall, &hypotenuse);
        direction_to_turn = right;
    }

    printf("angulo: %f, hipotenusa: %f\n",angle_to_wall, hypotenuse);
    Spike_Turn_For_Degrees(direction_to_turn, 60, angle_to_wall, 30);
    Spike_Center_Vehicle_Short();
    Spike_Advance_For_distance(80, (int)hypotenuse - 420, (angle_to_wall*direction_to_turn*-1));
    Spike_Small_Turn((direction_to_turn * -1), 60, 0, 30);
    Spike_Center_Vehicle_Short();
    Spike_Coast_Motors();

    return cube;
}

Color_traffic_light Desicion(Color_traffic_light past_cube, bool middle_cube){
    Color_traffic_light cubo_temp;
    float slope = Slope(front);
    Spike_Reset_Gyro(slope);
    usleep(200000);
    if(past_cube == none){
        if (middle_cube == true){
            cubo_temp = esquivar_cubos_middle();
        }
        else{
            cubo_temp = esquivar_cubos_1();
            cubo_temp = esquivar_cubos_2(cubo_temp);
        }
    }
    else{
        cubo_temp = esquivar_cubos_2(past_cube);
    }

    return cubo_temp;
}

void signal_handler(int signum){
    printf("\nCtrl+C detceted\n");
    terminating_main = 1;
    signal(SIGINT, SIG_DFL);
}
