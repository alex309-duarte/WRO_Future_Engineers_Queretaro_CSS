/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */

#include "lvgl/lvgl.h"
#include <stdio.h>
#include "gui_guider.h"


void init_scr_del_flag(lv_ui *ui){
	ui->screen_empty_del = true;
	ui->screen_startup_del = true;
	ui->screen_sys_del = true;
	ui->screen_imu_del = true;
	ui->screen_ps2_del = true;
	ui->screen_sbus_del = true;
}

void setup_ui(lv_ui *ui){
	LV_THEME_DEFAULT_INIT(LV_THEME_DEFAULT_COLOR_PRIMARY, LV_THEME_DEFAULT_COLOR_SECONDARY, 
		LV_THEME_MATERIAL_FLAG_LIGHT, LV_THEME_DEFAULT_FONT_SMALL,  LV_THEME_DEFAULT_FONT_NORMAL, 
		LV_THEME_DEFAULT_FONT_SUBTITLE, LV_THEME_DEFAULT_FONT_TITLE);
	init_scr_del_flag(ui);
	setup_scr_screen_empty(ui);
	lv_scr_load(ui->screen_empty);
}
