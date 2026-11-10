/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"


void setup_scr_screen_empty(lv_ui *ui){

	//Write codes screen_empty
	ui->screen_empty = lv_obj_create(NULL, NULL);

	//Write style LV_OBJ_PART_MAIN for screen_empty
	static lv_style_t style_screen_empty_main;
	lv_style_reset(&style_screen_empty_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_empty_main
	lv_style_set_bg_color(&style_screen_empty_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_opa(&style_screen_empty_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_empty, LV_OBJ_PART_MAIN, &style_screen_empty_main);

	//Init events for screen
	events_init_screen_empty(ui);
}