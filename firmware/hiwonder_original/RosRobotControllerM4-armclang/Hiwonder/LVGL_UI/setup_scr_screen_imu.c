/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"


void setup_scr_screen_imu(lv_ui *ui){

	//Write codes screen_imu
	ui->screen_imu = lv_obj_create(NULL, NULL);

	//Write style LV_OBJ_PART_MAIN for screen_imu
	static lv_style_t style_screen_imu_main;
	lv_style_reset(&style_screen_imu_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_main
	lv_style_set_bg_color(&style_screen_imu_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_opa(&style_screen_imu_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_imu, LV_OBJ_PART_MAIN, &style_screen_imu_main);

	//Write codes screen_imu_cont_1
	ui->screen_imu_cont_1 = lv_cont_create(ui->screen_imu, NULL);

	//Write style LV_CONT_PART_MAIN for screen_imu_cont_1
	static lv_style_t style_screen_imu_cont_1_main;
	lv_style_reset(&style_screen_imu_cont_1_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_cont_1_main
	lv_style_set_radius(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, 5);
	lv_style_set_bg_color(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xbf, 0x75));
	lv_style_set_bg_grad_color(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, 225);
	lv_style_set_shadow_color(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_border_color(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0x99, 0x99, 0x99));
	lv_style_set_border_width(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_border_opa(&style_screen_imu_cont_1_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_imu_cont_1, LV_CONT_PART_MAIN, &style_screen_imu_cont_1_main);
	lv_obj_set_pos(ui->screen_imu_cont_1, 10, 2);
	lv_obj_set_size(ui->screen_imu_cont_1, 140, 20);
	lv_obj_set_click(ui->screen_imu_cont_1, false);
	lv_cont_set_layout(ui->screen_imu_cont_1, LV_LAYOUT_OFF);
	lv_cont_set_fit(ui->screen_imu_cont_1, LV_FIT_NONE);

	//Write codes screen_imu_title
	ui->screen_imu_title = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_title, "IMU数据");
	lv_label_set_long_mode(ui->screen_imu_title, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_title, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_title
	static lv_style_t style_screen_imu_title_main;
	lv_style_reset(&style_screen_imu_title_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_title_main
	lv_style_set_radius(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_title_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_title_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_title_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_imu_title_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_title_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_title_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_imu_title_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Normal_16);
	lv_style_set_text_letter_space(&style_screen_imu_title_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_title_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_title, LV_LABEL_PART_MAIN, &style_screen_imu_title_main);
	lv_obj_set_pos(ui->screen_imu_title, 33, 5);
	lv_obj_set_width(ui->screen_imu_title, 93);

	//Write codes screen_imu_label_1
	ui->screen_imu_label_1 = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_label_1, "AX:0.01");
	lv_label_set_long_mode(ui->screen_imu_label_1, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_label_1, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_label_1
	static lv_style_t style_screen_imu_label_1_main;
	lv_style_reset(&style_screen_imu_label_1_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_label_1_main
	lv_style_set_radius(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, lv_color_make(0xf0, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_label_1_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_label_1, LV_LABEL_PART_MAIN, &style_screen_imu_label_1_main);
	lv_obj_set_pos(ui->screen_imu_label_1, 2, 29);
	lv_obj_set_width(ui->screen_imu_label_1, 52);

	//Write codes screen_imu_label_2
	ui->screen_imu_label_2 = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_label_2, "AY:0.01");
	lv_label_set_long_mode(ui->screen_imu_label_2, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_label_2, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_label_2
	static lv_style_t style_screen_imu_label_2_main;
	lv_style_reset(&style_screen_imu_label_2_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_label_2_main
	lv_style_set_radius(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0xff, 0x00));
	lv_style_set_text_font(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_label_2_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_label_2, LV_LABEL_PART_MAIN, &style_screen_imu_label_2_main);
	lv_obj_set_pos(ui->screen_imu_label_2, 54, 29);
	lv_obj_set_width(ui->screen_imu_label_2, 52);

	//Write codes screen_imu_label_3
	ui->screen_imu_label_3 = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_label_3, "AZ:0.01");
	lv_label_set_long_mode(ui->screen_imu_label_3, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_label_3, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_label_3
	static lv_style_t style_screen_imu_label_3_main;
	lv_style_reset(&style_screen_imu_label_3_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_label_3_main
	lv_style_set_radius(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0xFF));
	lv_style_set_text_font(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_label_3_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_label_3, LV_LABEL_PART_MAIN, &style_screen_imu_label_3_main);
	lv_obj_set_pos(ui->screen_imu_label_3, 106, 29);
	lv_obj_set_width(ui->screen_imu_label_3, 53);

	//Write codes screen_imu_label_4
	ui->screen_imu_label_4 = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_label_4, "GX:0.01");
	lv_label_set_long_mode(ui->screen_imu_label_4, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_label_4, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_label_4
	static lv_style_t style_screen_imu_label_4_main;
	lv_style_reset(&style_screen_imu_label_4_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_label_4_main
	lv_style_set_radius(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, lv_color_make(0xf0, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_label_4_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_label_4, LV_LABEL_PART_MAIN, &style_screen_imu_label_4_main);
	lv_obj_set_pos(ui->screen_imu_label_4, 2, 47);
	lv_obj_set_width(ui->screen_imu_label_4, 52);

	//Write codes screen_imu_label_5
	ui->screen_imu_label_5 = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_label_5, "GZ:0.01");
	lv_label_set_long_mode(ui->screen_imu_label_5, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_label_5, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_label_5
	static lv_style_t style_screen_imu_label_5_main;
	lv_style_reset(&style_screen_imu_label_5_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_label_5_main
	lv_style_set_radius(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0xFF));
	lv_style_set_text_font(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_label_5_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_label_5, LV_LABEL_PART_MAIN, &style_screen_imu_label_5_main);
	lv_obj_set_pos(ui->screen_imu_label_5, 106, 47);
	lv_obj_set_width(ui->screen_imu_label_5, 52);

	//Write codes screen_imu_label_6
	ui->screen_imu_label_6 = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_label_6, "GY:0.01");
	lv_label_set_long_mode(ui->screen_imu_label_6, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_label_6, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_label_6
	static lv_style_t style_screen_imu_label_6_main;
	lv_style_reset(&style_screen_imu_label_6_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_label_6_main
	lv_style_set_radius(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0xFF, 0x00));
	lv_style_set_text_font(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_label_6_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_label_6, LV_LABEL_PART_MAIN, &style_screen_imu_label_6_main);
	lv_obj_set_pos(ui->screen_imu_label_6, 54, 47);
	lv_obj_set_width(ui->screen_imu_label_6, 52);

	//Write codes screen_imu_label_7
	ui->screen_imu_label_7 = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_label_7, "R:0.01");
	lv_label_set_long_mode(ui->screen_imu_label_7, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_label_7, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_label_7
	static lv_style_t style_screen_imu_label_7_main;
	lv_style_reset(&style_screen_imu_label_7_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_label_7_main
	lv_style_set_radius(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, lv_color_make(0xFF, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_label_7_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_label_7, LV_LABEL_PART_MAIN, &style_screen_imu_label_7_main);
	lv_obj_set_pos(ui->screen_imu_label_7, 8, 65);
	lv_obj_set_width(ui->screen_imu_label_7, 50);

	//Write codes screen_imu_label_8
	ui->screen_imu_label_8 = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_label_8, "P:0.01");
	lv_label_set_long_mode(ui->screen_imu_label_8, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_label_8, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_label_8
	static lv_style_t style_screen_imu_label_8_main;
	lv_style_reset(&style_screen_imu_label_8_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_label_8_main
	lv_style_set_radius(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0xff, 0x00));
	lv_style_set_text_font(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_label_8_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_label_8, LV_LABEL_PART_MAIN, &style_screen_imu_label_8_main);
	lv_obj_set_pos(ui->screen_imu_label_8, 60, 65);
	lv_obj_set_width(ui->screen_imu_label_8, 50);

	//Write codes screen_imu_label_9
	ui->screen_imu_label_9 = lv_label_create(ui->screen_imu, NULL);
	lv_label_set_text(ui->screen_imu_label_9, "Y:0.01");
	lv_label_set_long_mode(ui->screen_imu_label_9, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_imu_label_9, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_imu_label_9
	static lv_style_t style_screen_imu_label_9_main;
	lv_style_reset(&style_screen_imu_label_9_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_imu_label_9_main
	lv_style_set_radius(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0xFF));
	lv_style_set_text_font(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_imu_label_9_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_imu_label_9, LV_LABEL_PART_MAIN, &style_screen_imu_label_9_main);
	lv_obj_set_pos(ui->screen_imu_label_9, 112, 65);
	lv_obj_set_width(ui->screen_imu_label_9, 50);

	//Init events for screen
	events_init_screen_imu(ui);
}