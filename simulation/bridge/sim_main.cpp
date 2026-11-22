// Entry point of the Obstacle Challenge program in the digital twin.
//
// It follows main() of src/cpp/object_detection_hiwonder_h10/object_detection.cpp
// step by step: LiDAR thread, emergency stop, start button, board start-up
// (Spike_Reto2_Init), wait for the first LiDAR scan, camera pipeline, mission
// thread. The only difference is where the camera frames come from: the
// robot opens the Raspberry Pi camera through GStreamer, the twin requests
// each frame from the simulated Camera Module 3 Wide (800 x 600, as the
// "rpi" input). The rest of the pipeline is the repository's own code:
// preprocess_callback, run_inference_async, run_post_process and
// postprocess_callback (detections, LiDAR walls, traffic_lights), and the
// mission (Obstacle_Challenge_Thread) is untouched. object_detection.cpp is
// compiled with its main() renamed (-Dmain=wro_vehicle_main).
//
// Twin options (environment):
//   WRO_SIM_HOST=<ip>          scene host (default: the Windows host of WSL)
//   WRO_SIM_AUTOSTART=<s>      press the start button after <s> seconds
//   WRO_SIM_EXIT_AFTER_RUN=<s> end the program <s> seconds after the mission
//   WRO_SURPRISE_RULE=1        surprise-rule prototype (team plan, README 6.10)
//   WRO_SIM_FRAME_LOG=<csv>    simulation time of every processed camera frame
#include "toolbox.hpp"
#include "hailo_infer.hpp"
#include "utils.hpp"

#include "spike.h"
#include "reto2_config.h"
#include "rasp_gpio.h"
#include "Oradar_S2L.h"
#include "parada_emergencia.h"
#include "common_var.h"
#include "sim_link.hpp"

#include <opencv2/imgproc.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>
#include <limits>
#include <iostream>
#include <pthread.h>
#include <string>
#include <thread>

using namespace hailo_utils;

// Defined in object_detection.cpp (repository code).
extern pthread_t writer;
extern pthread_t Main_Actions;
extern std::shared_ptr<BoundedTSQueue<std::pair<std::vector<cv::Mat>, std::vector<cv::Mat>>>> preprocessed_batch_queue;
extern std::shared_ptr<BoundedTSQueue<InferenceResult>> results_queue;
void *Obstacle_Challenge_Thread(void *arg);
void preprocess_callback(const std::vector<cv::Mat> &org_frames, std::vector<cv::Mat> &preprocessed_frames,
                         uint32_t target_width, uint32_t target_height);
void postprocess_callback(cv::Mat &frame_to_draw,
                          const std::vector<std::pair<uint8_t *, hailo_vstream_info_t>> &output_data_and_infos,
                          const VisualizationParams &vis);
void signal_handler(int signum);

namespace wro_sim {
// Surprise rule prototype (bridge/surprise_rule.cpp), WRO_SURPRISE_RULE=1.
void Surprise_Process(cv::Mat &frame, const std::vector<std::pair<uint8_t *, hailo_vstream_info_t>> &outputs);
bool Surprise_Enabled();
double Board_Sim_Time();
}

namespace {

constexpr int kCameraWidth = 800;   // the "rpi" input of the robot: 800 x 600
constexpr int kCameraHeight = 600;

bool g_main_actions_started = false;
std::atomic<bool> g_mission_done{false};

// Twin camera: one frame per request ("img" -> uint32 width, uint32 height,
// RGB rows bottom-up, as CoppeliaSim renders them). Returns a BGR frame like
// the GStreamer appsink of the robot, or an empty Mat on error.
cv::Mat Grab_Twin_Frame(wro_sim::SimChannel &camera) {
    std::string reply;
    try {
        reply = camera.Request("img");
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Twin camera: %s\n", e.what());
        return cv::Mat();
    }
    if (reply.size() < 8) return cv::Mat();
    std::uint32_t size[2];
    std::memcpy(size, reply.data(), sizeof(size));
    const int w = static_cast<int>(size[0]);
    const int h = static_cast<int>(size[1]);
    if (reply.size() < 8 + static_cast<size_t>(w) * h * 3) return cv::Mat();
    cv::Mat rgb(h, w, CV_8UC3, const_cast<char *>(reply.data() + 8));
    cv::Mat bgr;
    cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
    cv::flip(bgr, bgr, 0);
    if (w != kCameraWidth || h != kCameraHeight) cv::resize(bgr, bgr, cv::Size(kCameraWidth, kCameraHeight));
    return bgr;
}

// Frames sent to the pipeline and not yet through its last stage.
std::atomic<int> g_frames_in_flight{0};

// Producer: the same loop as hailo_utils::preprocess_video_frames (toolbox.cpp),
// with the twin camera in place of cv::VideoCapture.
hailo_status Run_Twin_Camera(const std::string &hef_path, HailoInfer &model, size_t batch_size,
                             double framerate) {
    auto shape = model.get_infer_model()->hef().get_input_vstream_infos().release()[0].shape;
    print_net_banner(get_hef_name(hef_path), std::ref(model.get_inputs()), std::ref(model.get_outputs()));
    wro_sim::SimChannel camera(wro_sim::kPortCamera, 3000);

    std::vector<cv::Mat> org_frames;
    std::vector<cv::Mat> preprocessed_frames;
    const bool limit_fps = framerate > 0.0;
    using clock = std::chrono::steady_clock;
    const auto frame_interval =
        std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(limit_fps ? 1.0 / framerate : 0.0));
    auto next_frame_time = clock::now() + frame_interval;
    auto last_push = clock::now();

    while (terminating_main == 0) {
        if (limit_fps && clock::now() < next_frame_time) std::this_thread::sleep_until(next_frame_time);
        // A live camera, not a file: while the previous frame is still being
        // processed, this one is skipped. The queues of the pipeline hold up
        // to 60 frames each; if the PC falls a little behind the frame rate
        // they fill up and the program would decide on images that are
        // seconds old (seen in the twin while it was saving its video).
        if (g_frames_in_flight.load() > 0 && clock::now() - last_push < std::chrono::milliseconds(500)) {
            if (limit_fps) next_frame_time = clock::now() + frame_interval / 4;
            else std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }
        cv::Mat org_frame = Grab_Twin_Frame(camera);
        if (org_frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        org_frames.push_back(org_frame);
        if (org_frames.size() == batch_size) {
            preprocessed_frames.clear();
            preprocess_callback(org_frames, preprocessed_frames, shape.width, shape.height);
            g_frames_in_flight.store(static_cast<int>(batch_size));
            last_push = clock::now();
            preprocessed_batch_queue->push(std::make_pair(org_frames, preprocessed_frames));
            org_frames.clear();
        }
        if (limit_fps) {
            next_frame_time += frame_interval;
            if (next_frame_time < clock::now()) next_frame_time = clock::now();   // no bursts after a delay
        }
    }
    // Ctrl+C / end of the run: close both queues, so the post-processing
    // thread ends too and closes the saved camera video (-s) properly.
    preprocessed_batch_queue->stop();
    results_queue->stop();
    return HAILO_SUCCESS;
}

// Same order as Apagar_Todo() in object_detection.cpp.
void Shutdown_All() {
    terminating_main = 1;
    if (g_main_actions_started) {
        pthread_join(Main_Actions, NULL);
        g_main_actions_started = false;
    }
    if (spike_compat::Spike_Is_Attached()) {
        try { Spike_Coast_Motors(); } catch (...) {}
        Spike_Close_Serial();
    }
    Rasp_Gpio_Clean();
    Oradar_S2L_Set_Terminating();
    pthread_join(writer, NULL);
    Oradar_S2L_Close();
}

// The mission thread of the robot runs until Ctrl+C. In the twin the
// program can end by itself a few seconds after the mission returns, so a
// series of runs can be scripted.
void *Mission_Wrapper(void *arg) {
    void *r = Obstacle_Challenge_Thread(arg);
    g_mission_done = true;
    std::printf("Twin: mission thread finished\n");
    const char *after = std::getenv("WRO_SIM_EXIT_AFTER_RUN");
    if (after != nullptr && *after != '\0') {
        std::this_thread::sleep_for(std::chrono::duration<double>(std::atof(after)));
        terminating_main = 1;
        preprocessed_batch_queue->stop();
        results_queue->stop();
    }
    return r;
}

}  // namespace

int main(int argc, char **argv) {
    signal(SIGINT, signal_handler);
    // Scripted end of a run (tools/run_episode.ps1 sends SIGUSR1 at its time
    // limit): the normal shutdown path, so the saved camera video is closed.
    // Ctrl+C and kill keep the emergency stop of the robot, which ends the
    // process at once.
    signal(SIGUSR1, [](int) { terminating_main = 1; });
    std::printf("Digital twin: scene at %s (ports %d-%d)\n", wro_sim::Sim_Host().c_str(),
                wro_sim::kPortBoard, wro_sim::kPortCamera);

    Oradar_S2L_Init_Lidar();
    {
        unsigned char parada_lidar[16];
        const int largo = Oradar_S2L_Stop_Frames(parada_lidar, sizeof(parada_lidar));
        if (largo > 0) Parada_Emergencia_Agregar_Puerto(Oradar_S2L_Port(), parada_lidar, largo);
    }
    pthread_create(&writer, NULL, Oradar_S2L_Lidar_Writer_Thread, NULL);

    if (Rasp_Gpio_Init() != 0) {
        Oradar_S2L_Set_Terminating();
        pthread_join(writer, NULL);
        Oradar_S2L_Close();
        return HAILO_INTERNAL_FAILURE;
    }
    if (Spike_Reto2_Init() != 0) {
        std::cerr << "Twin: the board did not start; nothing will move.\n";
        Shutdown_All();
        return HAILO_INTERNAL_FAILURE;
    }

    for (int i = 0; (i < 50) && (Oradar_S2L_Get_Scan_Seq() == 0) && (terminating_main == 0); i++) {
        usleep(100000);
    }
    if (Oradar_S2L_Get_Scan_Seq() == 0) {
        std::cerr << "ERROR: the lidar is not sending data (no scans for 5 s). Nothing will move.\n";
        Shutdown_All();
        return HAILO_INTERNAL_FAILURE;
    }
    {
        float lidar[360];
        Oradar_S2L_Get_Buffer(&lidar[0]);
        printf("Lidar ready: front %.0f mm, right %.0f mm, left %.0f mm\n", lidar[RP_TO_ORADAR_IDX(FRONT)],
               lidar[RP_TO_ORADAR_IDX(RIGHT)], lidar[RP_TO_ORADAR_IDX(LEFT)]);
    }

    try {
        std::chrono::duration<double> inference_time;
        auto t_start = Clock::now();
        double org_height = kCameraHeight, org_width = kCameraWidth;
        cv::VideoCapture capture;  // unused: frames come from the twin camera
        size_t frame_count = 0;
        InputType input_type;
        input_type.is_camera = true;

        CommandLineArgs args = parse_command_line_arguments(argc, argv);
        if (args.net.empty()) args.net = "wro_h8_yolov8n_v4.hef";
        if (args.framerate <= 0.0) args.framerate = 30.0;
        HailoInfer model(args.net, args.batch_size);

        const char *visualization_config_env = std::getenv("WRO_VISUALIZATION_CONFIG");
        const std::string visualization_config =
            visualization_config_env ? visualization_config_env : "visualization_config.yaml";
        VisualizationParams vis_param = load_visualization_params(visualization_config);
        validate_visualization_params(vis_param, AppVisMode::object_detection);
        // The repository's postprocess_callback; the surprise-rule prototype
        // (off unless WRO_SURPRISE_RULE=1) runs just before it on each result.
        auto post_cb = [&vis_param](cv::Mat &frame,
                                    const std::vector<std::pair<uint8_t *, hailo_vstream_info_t>> &outputs) {
            if (wro_sim::Surprise_Enabled()) wro_sim::Surprise_Process(frame, outputs);
            // Simulation time of every processed frame, to line the saved
            // camera video (-s -o <dir>) up with the replay of the run.
            static FILE *frame_log = [] {
                const char *path = std::getenv("WRO_SIM_FRAME_LOG");
                return (path != nullptr && *path != '\0') ? std::fopen(path, "w") : nullptr;
            }();
            static long frame_index = 0;
            if (frame_log != nullptr) {
                try {
                    std::fprintf(frame_log, "%ld,%.3f\n", frame_index, wro_sim::Board_Sim_Time());
                    std::fflush(frame_log);
                } catch (...) {
                }
            }
            ++frame_index;
            postprocess_callback(frame, outputs, vis_param);
            g_frames_in_flight.fetch_sub(1);   // this frame is done: the camera may send the next one
        };

        auto preprocess_thread = std::async(std::launch::async, Run_Twin_Camera, args.net, std::ref(model),
                                            args.batch_size, args.framerate);
        ModelInputQueuesMap input_queues = {
            {model.get_infer_model()->get_input_names().at(0), preprocessed_batch_queue}};
        auto inference_thread = std::async(run_inference_async, std::ref(model), std::ref(inference_time),
                                           std::ref(input_queues), results_queue);
        auto output_parser_thread = std::async(
            run_post_process, std::ref(input_type), std::ref(org_height), std::ref(org_width),
            std::ref(frame_count), std::ref(capture), std::ref(args.framerate), std::ref(args.batch_size),
            std::ref(args.save_stream_output), std::ref(args.no_display), std::ref(args.output_dir),
            std::ref(args.output_resolution), results_queue, post_cb);

        if (pthread_create(&Main_Actions, NULL, Mission_Wrapper, NULL) != 0) {
            throw std::runtime_error("could not create the mission thread");
        }
        g_main_actions_started = true;

        hailo_status status = wait_and_check_threads(preprocess_thread, "Preprocess", inference_thread,
                                                     "Inference", output_parser_thread, "Postprocess ");
        Shutdown_All();
        if (status != HAILO_SUCCESS) return status;
        print_inference_statistics(inference_time, args.net, static_cast<double>(frame_count),
                                   Clock::now() - t_start);
        return HAILO_SUCCESS;
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        Shutdown_All();
        return HAILO_INTERNAL_FAILURE;
    }
}
