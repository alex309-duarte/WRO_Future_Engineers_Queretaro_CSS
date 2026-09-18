#include "Oradar_S2L.h"
#include <ord_lidar_driver.h>
#include "spike.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <pthread.h>

using namespace ordlidar;

struct DistanceAccumulator{
    float distance_sum;
    int sample_count;
};

static OrdlidarDriver *oradar_device = nullptr;
static pthread_mutex_t oradar_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int oradar_terminating = 0;
static volatile float oradar_shared_buffer[360] = {0};
static volatile unsigned long oradar_scan_seq = 0; // incremented every time a new full scan is written

// MS200 default: 230400 baud.
static const char *serial_port_path = "/dev/ttyUSB0";
static const int   serial_baudrate = 230400;

void Oradar_S2L_Init_Lidar(void){
    oradar_device = new OrdlidarDriver(ORADAR_TYPE_SERIAL, ORADAR_MS200);
    oradar_device->SetSerialPort(serial_port_path, serial_baudrate);

    while (oradar_terminating == 0 && !oradar_device->Connect()){
        printf("Oradar lidar connect failed, retrying...\n");
        usleep(1000000);
    }
    printf("Oradar lidar connect successful\n");
}

void *Oradar_S2L_Lidar_Writer_Thread(void *arg){

    full_scan_data_st scan_data;
    DistanceAccumulator average[360] = {0};

    while(oradar_terminating == 0){

        if(oradar_device->GrabFullScan(scan_data)){
            for (int i = 0; i < 360; i++) {
                average[i].sample_count = 0;
                average[i].distance_sum = 0;
            }
            for (int pos = 0; pos < scan_data.vailtidy_point_num; ++pos) {
                int angle_bucket = (int)scan_data.data[pos].angle;
                if (angle_bucket >= 0 && angle_bucket < 360) {
                    average[angle_bucket].distance_sum += scan_data.data[pos].distance;
                    average[angle_bucket].sample_count += 1;
                }
            }

            pthread_mutex_lock(&oradar_buffer_mutex);
            for (int i = 0; i < 360; i++) {
                if(average[i].sample_count > 0){
                    oradar_shared_buffer[i] = average[i].distance_sum/average[i].sample_count;
                }
                else{
                    //printf("No measurements for angle %d\n", i);
                }
            }
            oradar_scan_seq++;
            pthread_mutex_unlock(&oradar_buffer_mutex); // Unlock after writing
        }
        usleep(1000);
    }
    return NULL;
}

float Oradar_S2L_Radians_To_Degrees(float radians){
    double degrees = ((radians * 180.f)/ M_PI);
    return degrees;
}

float Oradar_S2L_Degrees_To_Radians(float degrees){
    double radians = ((degrees * M_PI)/ 180.f);
    return radians;
}

direction Oradar_S2L_Advance_And_Detect_Side(int speed, int reference){

    float front_distance;
    float right_distance;
    float left_distance;
    float back_distance;

    front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
    right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
    left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];


    while((oradar_terminating == 0) && (((right_distance < 1350) && (left_distance < 1350)) || (front_distance > 1100))){
        front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
        right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
        left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];
        back_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(BACK)];
        Spike_Forward(speed,reference);
        usleep(1000);
        printf("dsitancia derecha : %f\n", right_distance);
        printf("dsitancia izquierda : %f\n", left_distance);
        printf("dsitancia frente : %f\n", front_distance);
        printf("dsitancia atras : %f\n", back_distance);
    }

    if(right_distance > 1350){
        return right;
    }
    else if (left_distance > 1350)
    {
        return left;
    }
    else{
        return invalid;
    }

}

void Oradar_S2L_Advance_Until_Left_Gap(int speed, int reference){

    float front_distance;
    float left_distance;

    front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
    left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    while((oradar_terminating == 0) && (((left_distance < 1350) && (front_distance > 500)) || (front_distance > 1100) || (front_distance == 0.0 ))){
        front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
        left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];
        Spike_Forward(speed,reference);
        usleep(2000);
        printf("dsitancia izquierda : %f\n", left_distance);
        //printf("dsitancia frente : %f\n", front_distance);
    }

}

void Oradar_S2L_Advance_Until_Right_Gap(int speed, int reference){

    float front_distance;
    float right_distance;

    front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
    right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];

    //printf("dsitancia derecha : %f\n", right_distance);
    //printf("dsitancia frente : %f\n", front_distance);

    while((oradar_terminating == 0) && (((right_distance < 1350) && (front_distance > 500))  || (front_distance > 1100) || (front_distance == 0.0 ))){
        front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
        right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
        Spike_Forward(speed,reference);
        //printf("dsitancia derecha : %f\n", right_distance);
        //printf("dsitancia frente : %f\n", front_distance);
        usleep(2000);
    }

}

int Oradar_S2L_Advance_And_Measure_Left_Slope(int speed, int degrees, int reference){
    float y1 = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    Spike_Advance_For_Degrees(speed, degrees, reference);

    float y2 = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    float distance_delta = y1-y2;
    int slope = (int)(10 *(Oradar_S2L_Radians_To_Degrees(atan(distance_delta/(175.0*degrees/360.0)))));
    printf("pendiente: %d\n", slope);
    return slope;
}

int Oradar_S2L_Advance_And_Measure_Right_Slope(int speed, int degrees, int reference){
    float y1 = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];

    Spike_Advance_For_Degrees(speed, degrees, reference);

    float y2 = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];

    float distance_delta = y1-y2;
    int slope = (int)(-10 *(Oradar_S2L_Radians_To_Degrees(atan(distance_delta/(175.0*degrees/360.0)))));
    printf("pendiente: %d, y1: %f, y2: %f\n", slope, y1, y2);
    return slope;
}

void Oradar_S2L_Advance_Until_Distance(int speed, int reference, int target_distance){
    float front_distance;

    front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];

    while((oradar_terminating == 0) && ((front_distance > target_distance) || (front_distance == 0))){
        front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
        Spike_Forward(speed,reference);
        //printf("dsitancia frente : %f\n", front_distance);
        usleep(1000);
    }

    Spike_Hold_Motors();
}

int Oradar_S2L_Correction_For_Triangles_Left(int degree){
    float H = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT + degree)];
    float CA = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(Oradar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = Oradar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = 90 - computed_angle;
    printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, missing_side, correction_angle, computed_angle, final_correction);
    return (int)(final_correction*10);

}

int Oradar_S2L_Correction_For_Triangles_Right(int degree){
    float CA = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
    float H = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT - degree)];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(Oradar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = Oradar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = computed_angle - 90;
    //printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, missing_side, correction_angle, computed_angle, final_correction);
    return (int)(final_correction*10);

}

int Oradar_S2L_Reconcile_Readings(int reading_a, int reading_b, int reading_c){
    if(((abs(reading_a - reading_b)) < 30) || ((abs(reading_b - reading_c)) < 30)){
        return reading_b;
    }
    else if ((abs(reading_a - reading_c)) < 30)
    {
        return ((reading_a + reading_c)/2);
    }
    else{
        return reading_b;
    }
}

// Distancia maxima (mm) que una lectura puede alejarse de la mediana del
// grupo antes de considerarse ruido (reflejo multipath, retorno rasante,
// esquina/objeto distinto a la pared) y descartarse del ajuste de recta.
#define SLOPE_OUTLIER_TOLERANCE_MM 200.0f

// Toma un snapshot consistente (bajo mutex, para no mezclar dos scans
// distintos a mitad de lectura) de las distancias alrededor de middle_point,
// descarta los buckets sin retorno de lidar (distancia <= 0) y los que se
// alejan demasiado de la mediana del grupo antes de convertirlos a
// coordenadas x,y. Devuelve cuantos puntos validos quedaron.
static int Oradar_S2L_Collect_Slope_Points(const int point, const int middle_point, float *x_point, float *y_point){
    float dis[point];
    int angle[point];

    pthread_mutex_lock(&oradar_buffer_mutex);
    for(int i = 0; i < point; i++){
        angle[i] = middle_point - point + i;
        dis[i] = oradar_shared_buffer[RP_TO_ORADAR_IDX(angle[i])];
    }
    pthread_mutex_unlock(&oradar_buffer_mutex);

    float valid_dis[point];
    int valid_angle[point];
    int valid_count = 0;
    for(int i = 0; i < point; i++){
        if(dis[i] > 0.0f){
            valid_dis[valid_count] = dis[i];
            valid_angle[valid_count] = angle[i];
            valid_count++;
        }
    }

    if(valid_count < 3){
        return 0;
    }

    // Los puntos con lecturas ruidosas suelen ser los que mas se alejan del
    // resto del grupo; como ademas caen en los extremos de la ventana
    // (angulos mas rasantes), tienen mas peso en la regresion y son la
    // principal fuente de variacion grande en la pendiente calculada.
    float sorted[point];
    memcpy(sorted, valid_dis, valid_count * sizeof(float));
    std::sort(sorted, sorted + valid_count);
    float median = sorted[valid_count / 2];

    int n = 0;
    for(int i = 0; i < valid_count; i++){
        if(fabs(valid_dis[i] - median) <= SLOPE_OUTLIER_TOLERANCE_MM){
            x_point[n] = valid_dis[i] * cos(Oradar_S2L_Degrees_To_Radians(valid_angle[i]));
            y_point[n] = valid_dis[i] * sin(Oradar_S2L_Degrees_To_Radians(valid_angle[i]));
            n++;
        }
    }

    return n;
}

int Oradar_S2L_Slope(const int point, const int middle_point){
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    float x_point[point];
    float y_point[point];

    int n = Oradar_S2L_Collect_Slope_Points(point, middle_point, x_point, y_point);
    if(n < 3){
        printf("Error: no hay suficientes lecturas validas del lidar para calcular la pendiente.\n");
        return 0;
    }

    for (int i = 0; i < n; i++) {
        sum_x += x_point[i];
        sum_y += y_point[i];
        sum_xy += x_point[i] * y_point[i];
        sum_x2 += x_point[i] * x_point[i];
    }

    double det = n * sum_x2 - sum_x * sum_x;

    if (det == 0) {
        printf("Error: El sistema no tiene solución única (todos los X son iguales).\n");
        return 0;
    }
    int p = (10)*(Oradar_S2L_Radians_To_Degrees(atan((n * sum_xy - sum_x * sum_y) / det)));
    printf("pendiente : %d\n",p);
    return (p);

}

// --- Deteccion de paredes por segmentacion de la nube de puntos del lidar,
// adaptada de object_detection.cpp (Select_Wall/Fit_Line_Orientation) en
// object_detection_original_h10. A diferencia de Oradar_S2L_Slope (que ajusta
// una recta directamente a una ventana angular fija), esto agrupa los puntos
// en segmentos por huecos de distancia y por esquinas, descarta los
// segmentos ruidosos/cortos/objetos-no-pared, y sigue la misma pared entre
// llamadas -- mucho mas resistente al ruido puntual que causaba la
// variacion grande en el angulo calculado.
struct Vec2{
    float x = 0.0f;
    float y = 0.0f;
};

static inline Vec2 Vec2_Sub(const Vec2 &a, const Vec2 &b){ return {a.x - b.x, a.y - b.y}; }
static inline Vec2 Vec2_Add(const Vec2 &a, const Vec2 &b){ return {a.x + b.x, a.y + b.y}; }
static inline Vec2 Vec2_Scale(const Vec2 &a, float s){ return {a.x * s, a.y * s}; }
static inline float Vec2_Norm(const Vec2 &a){ return sqrtf(a.x * a.x + a.y * a.y); }
static inline float Vec2_Cross(const Vec2 &a, const Vec2 &b){ return a.x * b.y - a.y * b.x; }

// Ajusta una recta a los puntos indicados por regresion ortogonal (PCA /
// Deming, la misma formula que ya usaba Oradar_S2L_Slope2) y devuelve su
// orientacion en grados.
static float Oradar_S2L_Fit_Line_Orientation(const std::vector<Vec2> &pts, const std::vector<int> &indices){
    double mean_x = 0, mean_y = 0;
    for(int idx : indices){ mean_x += pts[idx].x; mean_y += pts[idx].y; }
    mean_x /= indices.size();
    mean_y /= indices.size();

    double sxx = 0, syy = 0, sxy = 0;
    for(int idx : indices){
        double dx = pts[idx].x - mean_x;
        double dy = pts[idx].y - mean_y;
        sxx += dx * dx;
        syy += dy * dy;
        sxy += dx * dy;
    }
    return Oradar_S2L_Radians_To_Degrees((float)(0.5 * atan2(2.0 * sxy, sxx - syy)));
}

// Distancia perpendicular RMS de los puntos a su propia recta ajustada: mide
// que tan disperso/ruidoso esta el segmento (bajo = alineado, alto = ruidoso).
static float Oradar_S2L_Line_Fit_Residual_Mm(const std::vector<Vec2> &pts, const std::vector<int> &indices){
    if(indices.size() < 2) return 0.0f;
    float theta_rad = Oradar_S2L_Degrees_To_Radians(Oradar_S2L_Fit_Line_Orientation(pts, indices));
    double nx = -sin(theta_rad), ny = cos(theta_rad);

    double mean_x = 0, mean_y = 0;
    for(int idx : indices){ mean_x += pts[idx].x; mean_y += pts[idx].y; }
    mean_x /= indices.size();
    mean_y /= indices.size();

    double sum_sq = 0;
    for(int idx : indices){
        double r = (pts[idx].x - mean_x) * nx + (pts[idx].y - mean_y) * ny;
        sum_sq += r * r;
    }
    return (float)sqrt(sum_sq / indices.size());
}

// Divide recursivamente un segmento en su punto de maxima desviacion
// perpendicular respecto a la recta que une sus extremos (Iterative
// End-Point Fit). Detecta esquinas que un corte solo-por-hueco no ve: dos
// paredes que se encuentran en una esquina no tienen salto de distancia
// entre puntos consecutivos, solo un cambio de orientacion.
static void Oradar_S2L_Split_Segment_At_Corners(const std::vector<Vec2> &points_mm,
                                                 const std::vector<int> &indices,
                                                 float threshold_mm,
                                                 int corner_margin_points,
                                                 std::vector<std::vector<int>> &out_segments)
{
    if(indices.size() < 3){
        out_segments.push_back(indices);
        return;
    }

    const Vec2 &p1 = points_mm[indices.front()];
    const Vec2 &p2 = points_mm[indices.back()];
    Vec2 line_dir = Vec2_Sub(p2, p1);
    float line_len = Vec2_Norm(line_dir);

    float max_dist = 0;
    size_t split_at = 0;
    for(size_t k = 1; k + 1 < indices.size(); k++){
        const Vec2 &p = points_mm[indices[k]];
        float dist = (line_len < 1e-3f)
            ? Vec2_Norm(Vec2_Sub(p, p1))
            : fabsf(Vec2_Cross(line_dir, Vec2_Sub(p, p1))) / line_len;
        if(dist > max_dist){
            max_dist = dist;
            split_at = k;
        }
    }

    if(max_dist > threshold_mm && split_at > 0){
        size_t left_end = (split_at >= (size_t)corner_margin_points) ? split_at - corner_margin_points : 0;
        size_t right_start = std::min(split_at + (size_t)corner_margin_points, indices.size() - 1);
        std::vector<int> left(indices.begin(), indices.begin() + left_end + 1);
        std::vector<int> right(indices.begin() + right_start, indices.end());
        Oradar_S2L_Split_Segment_At_Corners(points_mm, left, threshold_mm, corner_margin_points, out_segments);
        Oradar_S2L_Split_Segment_At_Corners(points_mm, right, threshold_mm, corner_margin_points, out_segments);
    } else {
        out_segments.push_back(indices);
    }
}

struct Oradar_S2L_WallSelection{
    bool found = false;
    std::vector<Vec2> pts;
    std::vector<int> chosen_indices;
};

// Encuentra la mejor pared real dentro de una ventana angular centrada en
// middle_point (mismo sistema de angulos "RP" que Oradar_S2L_Slope: las
// constantes FRONT/RIGHT/BACK/LEFT de common_var.h). Segmenta la nube de
// puntos por huecos de distancia y por esquinas, descarta segmentos
// ruidosos/cortos, y entre los que califican elige el de mayor extension
// fisica (desempate por el ajuste mas limpio). Tambien recuerda la ultima
// pared seguida por cada lado (centroide+orientacion) para no saltar a una
// pared vecina de una llamada a otra.
static Oradar_S2L_WallSelection Oradar_S2L_Select_Wall(int middle_point){
    Oradar_S2L_WallSelection result;

    const int point = 70;
    const int middle_exclusion_deg = 1;
    const float min_range_mm = 150.0f;
    const int edge_pad_deg = 15;
    const int half_point = point / 2;

    float lidar_buffer[360];
    Oradar_S2L_Get_Buffer(&lidar_buffer[0]);

    std::vector<Vec2> pts;
    std::vector<bool> pt_is_core;
    pts.reserve(point + 2 * edge_pad_deg);
    pt_is_core.reserve(point + 2 * edge_pad_deg);
    for(int i = -edge_pad_deg; i < point + edge_pad_deg; i++){
        int angular_diff = std::abs(i - half_point);
        if(angular_diff <= middle_exclusion_deg) continue;

        int raw_angle = middle_point - half_point + i;
        float d = lidar_buffer[RP_TO_ORADAR_IDX(raw_angle)];
        if(d > min_range_mm){
            pts.push_back(Vec2{d * cosf(Oradar_S2L_Degrees_To_Radians((float)raw_angle)),
                                d * sinf(Oradar_S2L_Degrees_To_Radians((float)raw_angle))});
            pt_is_core.push_back(i >= 0 && i < point);
        }
    }

    if(pts.size() < 2){
        return result;
    }

    // Radius Outlier Removal: descarta puntos con pocos vecinos cerca. Sin
    // esto, un punto ruidoso aislado dispara un falso hueco/esquina y parte
    // una pared real larga en varios fragmentos pequenos.
    const float ror_radius_mm = 300.0f;
    const int ror_min_neighbors = 3;
    {
        std::vector<Vec2> filtered_pts;
        filtered_pts.reserve(pts.size());
        for(size_t i = 0; i < pts.size(); i++){
            if(!pt_is_core[i]) continue;
            int neighbor_count = 0;
            for(size_t j = 0; j < pts.size(); j++){
                if(i == j) continue;
                if(Vec2_Norm(Vec2_Sub(pts[i], pts[j])) <= ror_radius_mm){
                    neighbor_count++;
                    if(neighbor_count >= ror_min_neighbors) break;
                }
            }
            if(neighbor_count >= ror_min_neighbors){
                filtered_pts.push_back(pts[i]);
            }
        }
        pts = std::move(filtered_pts);
    }

    if(pts.size() < 2){
        return result;
    }

    const float gap_threshold_mm = 60.0f;
    std::vector<std::vector<int>> segments;
    std::vector<int> current_segment = {0};
    for(size_t i = 1; i < pts.size(); i++){
        if(Vec2_Norm(Vec2_Sub(pts[i], pts[i - 1])) > gap_threshold_mm){
            segments.push_back(current_segment);
            current_segment.clear();
        }
        current_segment.push_back((int)i);
    }
    segments.push_back(current_segment);

    const float corner_split_threshold_mm = 40.0f;
    const int corner_margin_points = 2;
    std::vector<std::vector<int>> wall_segments;
    for(auto &seg : segments){
        Oradar_S2L_Split_Segment_At_Corners(pts, seg, corner_split_threshold_mm, corner_margin_points, wall_segments);
    }

    const float max_wall_residual_mm = 15.0f;
    const int min_wall_points = 6;
    const float min_wall_extent_mm = 200.0f;
    struct SegmentScore{
        size_t seg_idx; float residual; bool qualifies;
        Vec2 centroid; float orientation_deg; float extent;
    };
    std::vector<SegmentScore> scores;
    for(size_t s = 0; s < wall_segments.size(); s++){
        const auto &seg = wall_segments[s];
        if((int)seg.size() < min_wall_points) continue;
        float extent = Vec2_Norm(Vec2_Sub(pts[seg.back()], pts[seg.front()]));
        if(extent < min_wall_extent_mm) continue;
        float residual = Oradar_S2L_Line_Fit_Residual_Mm(pts, seg);
        bool qualifies = (residual <= max_wall_residual_mm);
        Vec2 centroid{0.0f, 0.0f};
        for(int idx : seg) centroid = Vec2_Add(centroid, pts[idx]);
        centroid = Vec2_Scale(centroid, 1.0f / (float)seg.size());
        float orientation_deg = Oradar_S2L_Fit_Line_Orientation(pts, seg);
        scores.push_back(SegmentScore{s, residual, qualifies, centroid, orientation_deg, extent});
    }
    std::sort(scores.begin(), scores.end(), [](const SegmentScore &a, const SegmentScore &b){
        if(a.qualifies != b.qualifies) return a.qualifies;
        if(a.extent != b.extent) return a.extent > b.extent;
        return a.residual < b.residual;
    });

    if(scores.empty()){
        return result;
    }

    struct WallTrackState{ bool valid = false; Vec2 centroid; float orientation_deg = 0.0f; };
    static std::unordered_map<int, WallTrackState> wall_track_by_side;
    static pthread_mutex_t wall_track_mutex = PTHREAD_MUTEX_INITIALIZER;
    const float wall_track_max_match_mm = 300.0f;
    const float wall_track_max_angle_diff_deg = 20.0f;
    const float wall_track_min_extent_ratio = 0.7f;

    size_t best_idx = 0;
    {
        pthread_mutex_lock(&wall_track_mutex);
        auto track_it = wall_track_by_side.find(middle_point);
        if(track_it != wall_track_by_side.end() && track_it->second.valid){
            float best_match_dist = wall_track_max_match_mm;
            bool found_match = false;
            for(size_t i = 0; i < scores.size(); i++){
                if(!scores[i].qualifies) continue;
                float dist = Vec2_Norm(Vec2_Sub(scores[i].centroid, track_it->second.centroid));
                float angle_diff = fabsf(scores[i].orientation_deg - track_it->second.orientation_deg);
                if(angle_diff > 90.0f) angle_diff = 180.0f - angle_diff;
                if(dist < best_match_dist && angle_diff <= wall_track_max_angle_diff_deg){
                    best_match_dist = dist;
                    best_idx = i;
                    found_match = true;
                }
            }
            if(!found_match || scores[best_idx].extent < wall_track_min_extent_ratio * scores[0].extent){
                best_idx = 0;
            }
        }
        pthread_mutex_unlock(&wall_track_mutex);
    }

    const float max_merge_angle_diff_deg = 12.0f;
    const float max_merge_perp_offset_mm = 150.0f;
    std::vector<int> chosen_indices = wall_segments[scores[best_idx].seg_idx];
    {
        size_t merge_idx = (best_idx == 0) ? 1 : 0;
        if(merge_idx < scores.size() && merge_idx != best_idx){
            float orientation_diff = fabsf(scores[best_idx].orientation_deg - scores[merge_idx].orientation_deg);
            if(orientation_diff > 90.0f) orientation_diff = 180.0f - orientation_diff;
            float theta_rad = Oradar_S2L_Degrees_To_Radians(scores[best_idx].orientation_deg);
            float nx = -sinf(theta_rad), ny = cosf(theta_rad);
            Vec2 delta = Vec2_Sub(scores[merge_idx].centroid, scores[best_idx].centroid);
            float perp_offset_mm = fabsf(delta.x * nx + delta.y * ny);
            if(orientation_diff <= max_merge_angle_diff_deg && perp_offset_mm <= max_merge_perp_offset_mm){
                const auto &merge_seg = wall_segments[scores[merge_idx].seg_idx];
                chosen_indices.insert(chosen_indices.end(), merge_seg.begin(), merge_seg.end());
            }
        }
    }

    Vec2 final_centroid{0.0f, 0.0f};
    for(int idx : chosen_indices) final_centroid = Vec2_Add(final_centroid, pts[idx]);
    final_centroid = Vec2_Scale(final_centroid, 1.0f / (float)chosen_indices.size());
    {
        pthread_mutex_lock(&wall_track_mutex);
        wall_track_by_side[middle_point] = WallTrackState{true, final_centroid, Oradar_S2L_Fit_Line_Orientation(pts, chosen_indices)};
        pthread_mutex_unlock(&wall_track_mutex);
    }

    result.found = true;
    result.pts = std::move(pts);
    result.chosen_indices = std::move(chosen_indices);
    return result;
}

// Reemplaza a Oradar_S2L_Slope2: en vez de ajustar una recta a los puntos
// crudos de una ventana fija, detecta la pared real (segmentacion +
// puntuacion + seguimiento entre llamadas) y devuelve su orientacion. Mucho
// mas resistente al ruido puntual/esquinas que causaban la variacion grande.
int Oradar_S2L_Wall_Slope(int middle_point){
    Oradar_S2L_WallSelection sel = Oradar_S2L_Select_Wall(middle_point);
    if(!sel.found){
        printf("Error: no se encontro una pared valida para calcular la pendiente.\n");
        return 0;
    }
    int p = (int)(10 * Oradar_S2L_Fit_Line_Orientation(sel.pts, sel.chosen_indices));
    printf("pendiente (pared): %d\n", p);
    return p;
}

int Oradar_S2L_Average(int arg_1, int arg_2){
    int average = (arg_1 + arg_2)/2;
    printf("correcion: %d\n",average);
    return average;
}

void Oradar_S2L_Close(void){
    if(oradar_device){
        oradar_device->Disconnect();
        delete oradar_device;
        oradar_device = nullptr;
    }
}

void Oradar_S2L_Set_Terminating(void){
    oradar_terminating = 1;
}

void Oradar_S2L_Get_Buffer(float *buffer){
    pthread_mutex_lock(&oradar_buffer_mutex);
    for(int i = 0; i < 360; i++){
        buffer[i] = oradar_shared_buffer[i];
    }
    pthread_mutex_unlock(&oradar_buffer_mutex);
}

unsigned long Oradar_S2L_Get_Scan_Seq(void){
    return oradar_scan_seq;
}
