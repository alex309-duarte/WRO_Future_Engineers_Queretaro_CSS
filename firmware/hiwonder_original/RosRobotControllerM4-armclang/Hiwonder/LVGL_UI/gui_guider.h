/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"
#include "guider_fonts.h"

typedef struct
{
	lv_obj_t *screen_empty;
	bool screen_empty_del;
	lv_obj_t *screen_startup;
	bool screen_startup_del;
	lv_obj_t *screen_startup_img_1;
	lv_obj_t *screen_sys;
	bool screen_sys_del;
	lv_obj_t *screen_sys_cont_1;
	lv_obj_t *screen_sys_label_volt111;
	lv_obj_t *screen_sys_title;
	lv_obj_t *screen_sys_label_volt;
	lv_obj_t *screen_imu;
	bool screen_imu_del;
	lv_obj_t *screen_imu_cont_1;
	lv_obj_t *screen_imu_title;
	lv_obj_t *screen_imu_label_1;
	lv_obj_t *screen_imu_label_2;
	lv_obj_t *screen_imu_label_3;
	lv_obj_t *screen_imu_label_4;
	lv_obj_t *screen_imu_label_5;
	lv_obj_t *screen_imu_label_6;
	lv_obj_t *screen_imu_label_7;
	lv_obj_t *screen_imu_label_8;
	lv_obj_t *screen_imu_label_9;
	lv_obj_t *screen_ps2;
	bool screen_ps2_del;
	lv_obj_t *screen_ps2_cont_1;
	lv_obj_t *screen_ps2_title;
	lv_obj_t *screen_ps2_label_10;
	lv_obj_t *screen_ps2_label_11;
	lv_obj_t *screen_ps2_label_12;
	lv_obj_t *screen_ps2_label_13;
	lv_obj_t *screen_ps2_label_14;
	lv_obj_t *screen_ps2_label_15;
	lv_obj_t *screen_ps2_label_16;
	lv_obj_t *screen_ps2_label_17;
	lv_obj_t *screen_ps2_label_20;
	lv_obj_t *screen_ps2_label_21;
	lv_obj_t *screen_ps2_label_22;
	lv_obj_t *screen_ps2_label_23;
	lv_obj_t *screen_ps2_label_24;
	lv_obj_t *screen_ps2_label_25;
	lv_obj_t *screen_ps2_label_26;
	lv_obj_t *screen_ps2_label_27;
	lv_obj_t *screen_ps2_label_28;
	lv_obj_t *screen_ps2_label_29;
	lv_obj_t *screen_ps2_label_30;
	lv_obj_t *screen_ps2_label_31;
	lv_obj_t *screen_sbus;
	bool screen_sbus_del;
	lv_obj_t *screen_sbus_cont_1;
	lv_obj_t *screen_sbus_title;
	lv_obj_t *screen_sbus_label_lx;
	lv_obj_t *screen_sbus_label_ly;
	lv_obj_t *screen_sbus_label_sw5;
	lv_obj_t *screen_sbus_label_sw6;
	lv_obj_t *screen_sbus_label_sw7;
	lv_obj_t *screen_sbus_label_sw1;
	lv_obj_t *screen_sbus_label_sw2;
	lv_obj_t *screen_sbus_label_sw3;
	lv_obj_t *screen_sbus_label_rx;
	lv_obj_t *screen_sbus_label_ry;
}lv_ui;

void init_scr_del_flag(lv_ui *ui);
void setup_ui(lv_ui *ui);
extern lv_ui guider_ui;
void setup_scr_screen_empty(lv_ui *ui);
void setup_scr_screen_startup(lv_ui *ui);
void setup_scr_screen_sys(lv_ui *ui);
void setup_scr_screen_imu(lv_ui *ui);
void setup_scr_screen_ps2(lv_ui *ui);
void setup_scr_screen_sbus(lv_ui *ui);
LV_IMG_DECLARE(_logo18_148x18);

#ifdef __cplusplus
}
#endif
#endif