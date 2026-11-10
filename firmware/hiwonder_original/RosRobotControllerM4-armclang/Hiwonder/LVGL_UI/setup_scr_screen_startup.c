/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"


void setup_scr_screen_startup(lv_ui *ui){

	//Write codes screen_startup
	ui->screen_startup = lv_obj_create(NULL, NULL);

	//Write style LV_OBJ_PART_MAIN for screen_startup
	static lv_style_t style_screen_startup_main;
	lv_style_reset(&style_screen_startup_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_startup_main
	lv_style_set_bg_color(&style_screen_startup_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_opa(&style_screen_startup_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_startup, LV_OBJ_PART_MAIN, &style_screen_startup_main);

	//Write codes screen_startup_img_1
	ui->screen_startup_img_1 = lv_img_create(ui->screen_startup, NULL);

	//Write style LV_IMG_PART_MAIN for screen_startup_img_1
	static lv_style_t style_screen_startup_img_1_main;
	lv_style_reset(&style_screen_startup_img_1_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_startup_img_1_main
	lv_style_set_image_recolor(&style_screen_startup_img_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_image_recolor_opa(&style_screen_startup_img_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_image_opa(&style_screen_startup_img_1_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_startup_img_1, LV_IMG_PART_MAIN, &style_screen_startup_img_1_main);
	lv_obj_set_pos(ui->screen_startup_img_1, 6, 31);
	lv_obj_set_size(ui->screen_startup_img_1, 148, 18);
	lv_obj_set_click(ui->screen_startup_img_1, true);
	lv_img_set_src(ui->screen_startup_img_1,&_logo18_148x18);
	lv_img_set_pivot(ui->screen_startup_img_1, 50,50);
	lv_img_set_angle(ui->screen_startup_img_1, 0);

	//Init events for screen
	events_init_screen_startup(ui);
}