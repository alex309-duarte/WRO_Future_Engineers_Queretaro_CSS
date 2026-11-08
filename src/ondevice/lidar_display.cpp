#include "lidar_display.h"
#include "Oradar_S2L.h"
#include "common_var.h"
#include "spike.h"

#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <vector>

// Dibujo y segmentacion copiados de postprocess_callback en
// object_detection_original_h10/object_detection.cpp. La pared de cada lado
// sale de Oradar_S2L_Get_Wall_Info: es la que usa este programa, no la del
// reto 2.

static volatile int display_gyro_enabled = 0;

// Divide un segmento en su punto de maxima desviacion (Iterative End-Point
// Fit): separa dos paredes que se tocan en una esquina sin hueco de distancia.
static void split_segment_at_corners(const std::vector<cv::Point2f> &points_mm,
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

// El buffer esta en la orientacion del Oradar (derecha=0, atras=90,
// izquierda=180, frente=270) y se dibuja con el indice 0 hacia +x y la y de
// la imagen hacia abajo: el frente queda arriba. Los puntos de
// Oradar_S2L_Get_Wall_Info vienen en el sistema "RP" (0=frente, 90=derecha),
// que es el mismo girado +270: (x, y) RP -> (y, -x) aqui.
static cv::Point2f rp_to_canvas_mm(float x_mm, float y_mm) {
    return cv::Point2f(y_mm, -x_mm);
}

static void draw_scan(cv::Mat &lidar_canvas, unsigned long scan_seq) {
    const int canvas_size = lidar_canvas.rows;
    const float lidar_scale = 0.15f; // pixels per mm
    const int grid_spacing_mm = 500;
    const cv::Point origin(canvas_size / 2, canvas_size / 2);
    lidar_canvas.setTo(cv::Scalar(255, 255, 255));
    const cv::Scalar grid_color(180, 180, 180);
    const cv::Scalar axis_color(60, 60, 60);

    // Cuadricula cada grid_spacing_mm, con etiquetas en mm
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

    cv::line(lidar_canvas, cv::Point(origin.x, 0), cv::Point(origin.x, canvas_size), axis_color, 1);
    cv::line(lidar_canvas, cv::Point(0, origin.y), cv::Point(canvas_size, origin.y), axis_color, 1);

    // Direccion 0 del buffer (indice 0, hacia +x)
    const cv::Scalar zero_deg_color(0, 0, 255); // red
    cv::line(lidar_canvas, origin, cv::Point(canvas_size, origin.y), zero_deg_color, 2);
    cv::putText(lidar_canvas, "0deg", cv::Point(canvas_size - 60, origin.y - 8),
                cv::FONT_HERSHEY_PLAIN, 1.0, zero_deg_color, 1);

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

    // Radius Outlier Removal: un punto con menos de ror_min_neighbors vecinos
    // a ror_radius_mm es ruido.
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

    // Adaptive Breakpoint Detector (Borges & Aldon): un hueco mayor que el
    // esperado entre dos retornos de la misma superficie empieza otra pared.
    const float bpd_lambda_deg = 10.0f;
    const float bpd_sigma_r_mm = 10.0f;
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

    // 359 y 0 son vecinos: unir el primer y el ultimo segmento si son la misma pared
    if (next_segment_id > 0) {
        int first_inlier = -1, last_inlier = -1;
        for (size_t i = 0; i < points_mm.size(); i++) {
            if (!is_outlier[i]) { first_inlier = (int)i; break; }
        }
        for (int i = (int)points_mm.size() - 1; i >= 0; i--) {
            if (!is_outlier[i]) { last_inlier = i; break; }
        }
        if (first_inlier >= 0 && last_inlier >= 0 && first_inlier != last_inlier) {
            float delta_phi_rad = Oradar_S2L_Degrees_To_Radians(375.0f - point_angle_deg[last_inlier] + point_angle_deg[first_inlier]);
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

    // Partir ademas en las esquinas (cambio de orientacion sin hueco)
    const float corner_split_threshold_mm = 40.0f;
    const int   corner_margin_points = 2;
    {
        std::unordered_map<int, std::vector<int>> segments_by_id;
        for (size_t i = 0; i < segment_id.size(); i++) {
            if (segment_id[i] >= 0) {
                segments_by_id[segment_id[i]].push_back((int)i);
                segment_id[i] = -1; // los puntos junto a una esquina quedan sin clasificar
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

    // Solo los max_walls segmentos mas grandes cuentan como paredes
    const int max_walls = 8;
    std::unordered_map<int, int> segment_counts;
    for (size_t i = 0; i < segment_id.size(); i++) {
        if (segment_id[i] >= 0) segment_counts[segment_id[i]]++;
    }
    std::vector<std::pair<int, int>> ranked_segments(segment_counts.begin(), segment_counts.end());
    std::sort(ranked_segments.begin(), ranked_segments.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    if ((int)ranked_segments.size() > max_walls) ranked_segments.resize(max_walls);

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

    // Asignar colores por centroide mas cercano al barrido anterior, para que
    // una pared no cambie de color de un barrido a otro.
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
    for (size_t c = 0; c < ranked_segments.size(); c++) {
        if (candidate_color[c] != -1) continue;
        for (int t = 0; t < max_walls; t++) {
            if (!track_used[t]) { candidate_color[c] = t; track_used[t] = true; break; }
        }
    }
    for (int t = 0; t < max_walls; t++) wall_tracks[t].valid = false;
    for (size_t c = 0; c < ranked_segments.size(); c++) {
        int t = candidate_color[c];
        if (t >= 0) {
            wall_tracks[t].valid = true;
            wall_tracks[t].centroid = candidate_centroid[c];
        }
    }

    std::unordered_map<int, int> wall_rank; // segment_id -> color
    for (size_t c = 0; c < ranked_segments.size(); c++) {
        wall_rank[ranked_segments[c].first] = candidate_color[c];
    }

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
    const cv::Scalar clutter_color(128, 128, 128);

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
            cv::circle(lidar_canvas, p, 0.5, color, -1);
        }
    }

    // La pared de cada lado segun Oradar_S2L_Wall_Slope, con su distancia
    // perpendicular y su orientacion (Wall_Slope devuelve 10 veces los grados).
    struct SideHighlight { int middle_point; const char *label; cv::Scalar color; };
    static const std::vector<SideHighlight> sides_to_highlight = {
        { FRONT, "front",  cv::Scalar(0, 0, 255) },    // red
        { RIGHT, "right",  cv::Scalar(255, 0, 0) },    // blue
        { LEFT,  "left",   cv::Scalar(0, 200, 0) },    // green
        { BACK,  "behind", cv::Scalar(0, 200, 200) },  // yellow-ish
    };

    int text_row = 0;
    for (const auto &sh : sides_to_highlight) {
        char dist_text[64];
        Oradar_S2L_Wall_Info wall = Oradar_S2L_Get_Wall_Info(sh.middle_point);
        if (wall.found) {
            for (size_t k = 0; k < wall.x_mm.size(); k++) {
                cv::Point2f q = rp_to_canvas_mm(wall.x_mm[k], wall.y_mm[k]);
                cv::Point p = origin + cv::Point((int)(q.x * lidar_scale), (int)(q.y * lidar_scale));
                cv::circle(lidar_canvas, p, 3, sh.color, -1);
                cv::circle(lidar_canvas, p, 3, cv::Scalar(0, 0, 0), 1);
            }
            cv::Point2f c = rp_to_canvas_mm(wall.centroid_x_mm, wall.centroid_y_mm);
            cv::Point text_pos = origin + cv::Point((int)(c.x * lidar_scale) + 10,
                                                    (int)(c.y * lidar_scale) - 10);
            // Izquierda/derecha: los mismos grados que Wall_Slope / 10. Frente y
            // atras corren a 90: se muestra cuanto se desvian, para que el mismo
            // giro del robot lea igual en los cuatro lados.
            float tilt_deg = wall.orientation_deg;
            if (sh.middle_point == FRONT || sh.middle_point == BACK) {
                tilt_deg -= 90.0f;
                if (tilt_deg <= -90.0f) tilt_deg += 180.0f;
            }
            snprintf(dist_text, sizeof(dist_text), "%s %.0fmm %.1fdeg", sh.label, wall.distance_mm, tilt_deg);
            cv::putText(lidar_canvas, dist_text, text_pos, cv::FONT_HERSHEY_SIMPLEX, 0.5, sh.color, 2);
        } else {
            snprintf(dist_text, sizeof(dist_text), "%s: sin pared", sh.label);
        }
        // Resumen en la esquina, para leerlo aunque la etiqueta quede fuera
        cv::putText(lidar_canvas, dist_text, cv::Point(10, 25 + 22 * text_row++),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, sh.color, 2);
    }

    // Abajo, el yaw de la placa: el mismo que usan los giros y el PD de rumbo
    // (Spike_Print_Gyro lo imprime en decigrados).
    char gyro_text[48];
    if (display_gyro_enabled) {
        try {
            snprintf(gyro_text, sizeof(gyro_text), "gyro: %.1f deg", Spike_Get_Gyro());
        } catch (const std::exception &) {
            snprintf(gyro_text, sizeof(gyro_text), "gyro: sin placa");
        }
    } else {
        snprintf(gyro_text, sizeof(gyro_text), "gyro: esperando placa");
    }
    int baseline = 0;
    const cv::Size gyro_size = cv::getTextSize(gyro_text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 2, &baseline);
    const cv::Point gyro_pos((canvas_size - gyro_size.width) / 2, canvas_size - 40);
    cv::rectangle(lidar_canvas, gyro_pos + cv::Point(-8, -gyro_size.height - 8),
                  gyro_pos + cv::Point(gyro_size.width + 8, baseline + 4), cv::Scalar(255, 255, 255), -1);
    cv::putText(lidar_canvas, gyro_text, gyro_pos, cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);

    char seq_text[48];
    snprintf(seq_text, sizeof(seq_text), "barrido %lu, %zu puntos", scan_seq, points_mm.size());
    cv::putText(lidar_canvas, seq_text, cv::Point(10, canvas_size - 12),
                cv::FONT_HERSHEY_PLAIN, 1.2, axis_color, 1);
}

bool Lidar_Display_Available(void) {
    const char *display = std::getenv("DISPLAY");
    if (display == nullptr || *display == '\0') {
        printf("Aviso: sin DISPLAY no se abre la ventana del lidar; sigue sin ella "
               "(en VNC: export DISPLAY=:0, o usa --no-display).\n");
        return false;
    }
    return true;
}

// Un fallo de la ventana (p. ej. sin permiso para el servidor X con sudo)
// sólo cierra la ventana; el recorrido sigue en su hilo.
void Lidar_Display_Loop(volatile int *terminado) {
    const char *window = "Lidar Coordinates";
    try {
        cv::namedWindow(window, cv::WINDOW_NORMAL);
        cv::resizeWindow(window, 800, 800);
        cv::Mat lidar_canvas(1080, 1080, CV_8UC3, cv::Scalar(255, 255, 255));
        unsigned long last_lidar_seq = 0;

        while (*terminado == 0) {
            unsigned long current_lidar_seq = Oradar_S2L_Get_Scan_Seq();
            if (current_lidar_seq != last_lidar_seq) {
                last_lidar_seq = current_lidar_seq;
                draw_scan(lidar_canvas, current_lidar_seq);
                cv::imshow(window, lidar_canvas);
            }
            cv::waitKey(30);
        }
        cv::destroyAllWindows();
    } catch (const std::exception &e) {
        printf("Aviso: se cerro la ventana del lidar (%s); el robot sigue.\n", e.what());
        while (*terminado == 0) usleep(100000);
    }
}

void Lidar_Display_Enable_Gyro(void) {
    display_gyro_enabled = 1;
}
