#ifndef LIDAR_DISPLAY_H
#define LIDAR_DISPLAY_H

// Open Challenge debug view: "Lidar Coordinates" OpenCV window, viewed over
// VNC, like the one of the Obstacle Challenge program (object_detection_h10):
// lidar points in mm, segmented walls and, for each side, the wall found by
// Oradar_S2L_Wall_Slope with its distance and angle.
// Read-only: it does not change what the robot does.

// Returns false (with a warning) if there is no DISPLAY, so the boot service
// and "sudo" runs keep working without a window.
bool Lidar_Display_Available(void);
// Draws the window until *terminado becomes non-zero. Must be called from the
// main thread: the OpenCV Qt backend does not support the window in another
// thread. If the window fails, it prints a warning and keeps waiting, without
// stopping the run.
void Lidar_Display_Loop(volatile int *terminado);
// Enables the display of the board gyro yaw. Call only after
// Spike_Hiwonder_Init() succeeds: before that, the board may not exist yet.
void Lidar_Display_Enable_Gyro(void);

#endif // LIDAR_DISPLAY_H
