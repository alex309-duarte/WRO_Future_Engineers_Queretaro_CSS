/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */


#ifndef EVENTS_INIT_H_
#define EVENTS_INIT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "gui_guider.h"

void events_init(lv_ui *ui);
void events_init_screen_empty(lv_ui *ui);
void events_init_screen_startup(lv_ui *ui);
void events_init_screen_sys(lv_ui *ui);
void events_init_screen_imu(lv_ui *ui);
void events_init_screen_ps2(lv_ui *ui);
void events_init_screen_sbus(lv_ui *ui);

#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */