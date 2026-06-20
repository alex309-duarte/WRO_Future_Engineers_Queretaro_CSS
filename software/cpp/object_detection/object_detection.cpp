#include "toolbox.hpp"
#include "hailo_infer.hpp"
#include "utils.hpp"

#include "spike.h"
#include "rasp_gpio.h"
#include "RPLidar_S2L.h"
#include "common_var.h"

struct traffic_lights_struct{
    float  middle_point_x;
    float  middle_point_y;
    int light_color; /* red = 2,  green = 1, xparking = 3 */
    float area;
    float confidence;
};

static traffic_lights_struct traffic_lights;

pthread_t writer;
pthread_t Main_Actions;
static volatile int terminating_main = 0;

void signal_handler(int signum);
void *Obstacle_Challenge_Thread(void *arg);
void Follow_cubes(int vel, float referencia_2, float area);
float Slope(float dis[], int angle[], int n);
void Print_Slope(const int point, const int middle_point);
void Follow_Wall(const direction side,const int point);

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

    // Show lidar points in their own window (mm scaled down to pixels)
    {
        const int canvas_size = 680;
        const float lidar_scale = 0.2f; // pixels per mm
        const cv::Point origin(canvas_size / 2, canvas_size / 2);
        cv::Mat lidar_canvas = cv::Mat::zeros(canvas_size, canvas_size, CV_8UC3);
        float lidar_buffer[360];
        RPLidar_S2L_Get_Buffer(&lidar_buffer[0]);
        for (int angle = 0; angle < 360; angle++) {
            if (lidar_buffer[angle] <= 0) continue;
            float x_point = lidar_buffer[angle] * cos(RPLidar_S2L_Grados_A_Radianes((float)angle)) * lidar_scale;
            float y_point = lidar_buffer[angle] * sin(RPLidar_S2L_Grados_A_Radianes((float)angle)) * lidar_scale;
            cv::Point p = origin + cv::Point((int)x_point, (int)y_point);
            if (p.x >= 0 && p.x < lidar_canvas.cols && p.y >= 0 && p.y < lidar_canvas.rows) {
                cv::circle(lidar_canvas, p, 0.5, cv::Scalar(0, 255, 0), -1);
            }
        }
        cv::imshow("Lidar Coordinates", lidar_canvas);
    }
    int index = 0;
    for (const auto &named_bbox : bboxes){
        traffic_light_area[index] = (named_bbox.bbox.x_max - named_bbox.bbox.x_min) * (named_bbox.bbox.y_max - named_bbox.bbox.y_min);
        index++;
    }
    int max_index = find_max_index(traffic_light_area, 10);
    if (bboxes.size() > 0){
        const auto named_bbox = bboxes.at(max_index); // Assuming only one bbox for traffic light detection
        traffic_lights.middle_point_x = (named_bbox.bbox.x_min + named_bbox.bbox.x_max) / 2.0;
        traffic_lights.middle_point_y = (named_bbox.bbox.y_min + named_bbox.bbox.y_max) / 2.0;
        traffic_lights.light_color = (int)named_bbox.class_id;
        traffic_lights.area = traffic_light_area[max_index];
        traffic_lights.confidence = named_bbox.bbox.score;
        //printf(" color = %d, area = %f, middle point = (%f, %f), confidence = %f", traffic_lights.light_color, traffic_lights.area, traffic_lights.middle_point_x, traffic_lights.middle_point_y, traffic_lights.confidence);
        //printf("\n");
    }
}


int main(int argc, char** argv)
{
    signal(SIGINT, signal_handler); /* Set interrupt for ctrl+C */
    RPLidar_S2L_Init_Lidar();
    pthread_create(&writer, NULL, RPLidar_S2L_Lidar_Writer_Thread, NULL);

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
        HailoInfer model(args.net, args.batch_size);

        // Load visualization config params
        VisualizationParams vis_param = load_visualization_params("/home/maker/WRO_Future_Engineers_Queretaro_CSS/software/cpp/object_detection/visualization_config.yaml");
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

    RPLidar_S2L_Get_Buffer(&lidar_shared_buffer[0] );
    distancia_frente = lidar_shared_buffer[0];
    distancia_derecha = lidar_shared_buffer[270];
    distancia_izquierda = lidar_shared_buffer[90];

    printf("dsitancia derecha : %f\n", distancia_derecha);
    printf("dsitancia izquierda : %f\n", distancia_izquierda);
    printf("dsitancia frente : %f\n", distancia_frente);

    /*
    while(1){
        Print_Slope(13, 60);
        usleep(1000);
    }*/

    Rasp_Gpio_Wait_For_Button();
    usleep(200000); //wiating for reset gyro
    Spike_Reset_Gyro(0);
    Spike_Center_Vehicle_Short();
    
    /*if(distancia_derecha > 600){
        printf("Sentido horario\n");
        Spike_Turn_For_Degrees(right, 60, 88, 40);


    }

    else if (distancia_izquierda > 600){
        printf("Sentido antihorario\n");
        Spike_Turn_For_Degrees(left, 60, 88, 40);

    
    }*/
   Follow_cubes(60, 0.5, 0.06);
   float grados = Spike_Get_Gyro();
   Spike_Turn_To_Zero(left, 60, 0, 30);
   Spike_Turn_For_Degrees(left, 60, grados + 30, 30);
   Spike_Center_Vehicle_Short();
   Spike_Advance_For_Degrees(60,30*abs(grados), 30);
   Spike_Turn_To_Zero(right, 60, 0, 30);
   Spike_Center_Vehicle_Short();
   Follow_cubes(60, 0.5, 0.06);  
   grados = Spike_Get_Gyro();
   
   Spike_Turn_For_Degrees(right, 60, (grados*-1) + 30, 30);
   Spike_Center_Vehicle_Short();
   Spike_Advance_For_Degrees(60,30*abs(grados), -30);
   Spike_Turn_To_Zero(left, 60, 0, 30);
   Spike_Center_Vehicle_Short();


   

   


    while(terminating_main == 0){
        /*RPLidar_S2L_Get_Buffer(&lidar_shared_buffer[0] );
        distancia_izquierda = lidar_shared_buffer[90];
        printf("dsitancia izquierda : %f\n", distancia_izquierda);*/
        usleep(1000);
    }

    Spike_Coast_Motors();
    RPLidar_S2L_Set_Terminating();
    Rasp_Gpio_Clean();
    Spike_Close_Serial();
    RPLidar_S2L_Close();
    return NULL;
}

void Follow_cubes(int vel, float referencia_2, float area){

    while((terminating_main == 0) && (traffic_lights.area < area) && (traffic_lights.confidence > 0.8)){
        Spike_Follow_Reference(vel, traffic_lights.middle_point_x, referencia_2);
        usleep(1000);
    }
    Spike_Coast_Motors();

}

float Slope(float dis[], int angle[], int n) {
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    float x_point[n] = {0};
    float y_point[n] = {0};

    for(int i = 0; i < n; i++) {
        x_point[i] = (dis[i] * cos(RPLidar_S2L_Grados_A_Radianes(angle[i])));
        y_point[i] = (dis[i] * sin(RPLidar_S2L_Grados_A_Radianes(angle[i])));
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
    float p = RPLidar_S2L_Radianes_A_Grados(atan((n * sum_xy - sum_x * sum_y) / det));

    return (p);

}

/** 
 * function name: Print_Slope
 * description: This function calculates the slope of the line formed by the points detected by the lidar
 * return: void
 * parameters: direction side, const int point
 * side: the side of the vehicle where the points are located (right or left)
 * point: the number of points to be used for the calculation of the slope, this parameter shuold be odd
 */
void Print_Slope(const int point, const int middle_point){
    float pendiente = 0;
    float lidar_shared_buffer[360];
    float dis[point] = {0};
    int angle[point] = {0};

    RPLidar_S2L_Get_Buffer(&lidar_shared_buffer[0] );
    for(int i = 0; i < point; i++){
        dis[i] = lidar_shared_buffer[middle_point - point + i];
        angle[i] = middle_point - point  +i;
    }

    pendiente = Slope(dis, angle, point);
    printf("Pendiente: %f\n", pendiente);
}

void Follow_Wall(const direction side, const int point){
    float pendiente = 0;
    int left_right_point = 0;
    float lidar_shared_buffer[360];
    float dis[point] = {0};
    int angle[point] = {0};

    if (side == right){
        left_right_point = 270;
    }
    else if (side == left)
    {
        left_right_point = 90;
    }
    while(terminating_main == 0){
        RPLidar_S2L_Get_Buffer(&lidar_shared_buffer[0] );
        for(int i = 0; i < point; i++){
            dis[i] = lidar_shared_buffer[left_right_point - point + i];
            angle[i] = left_right_point - point + i;
        }


        pendiente = Slope(dis, angle, point);
        printf("Pendiente: %f\n", pendiente);
        

        Spike_Follow_Reference(60, 0 ,pendiente);
    }
}


void signal_handler(int signum){
    printf("\nCtrl+C detceted\n");
    terminating_main = 1;
    signal(SIGINT, SIG_DFL);
}
