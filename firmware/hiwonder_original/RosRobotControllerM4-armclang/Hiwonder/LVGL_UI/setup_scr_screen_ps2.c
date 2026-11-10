/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"


void setup_scr_screen_ps2(lv_ui *ui){

	//Write codes screen_ps2
	ui->screen_ps2 = lv_obj_create(NULL, NULL);

	//Write style LV_OBJ_PART_MAIN for screen_ps2
	static lv_style_t style_screen_ps2_main;
	lv_style_reset(&style_screen_ps2_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_main
	lv_style_set_bg_color(&style_screen_ps2_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_opa(&style_screen_ps2_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_ps2, LV_OBJ_PART_MAIN, &style_screen_ps2_main);

	//Write codes screen_ps2_cont_1
	ui->screen_ps2_cont_1 = lv_cont_create(ui->screen_ps2, NULL);

	//Write style LV_CONT_PART_MAIN for screen_ps2_cont_1
	static lv_style_t style_screen_ps2_cont_1_main;
	lv_style_reset(&style_screen_ps2_cont_1_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_cont_1_main
	lv_style_set_radius(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, 5);
	lv_style_set_bg_color(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xbf, 0x75));
	lv_style_set_bg_grad_color(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, 225);
	lv_style_set_shadow_color(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_border_color(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0x99, 0x99, 0x99));
	lv_style_set_border_width(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_border_opa(&style_screen_ps2_cont_1_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_ps2_cont_1, LV_CONT_PART_MAIN, &style_screen_ps2_cont_1_main);
	lv_obj_set_pos(ui->screen_ps2_cont_1, 10, 2);
	lv_obj_set_size(ui->screen_ps2_cont_1, 140, 20);
	lv_obj_set_click(ui->screen_ps2_cont_1, false);
	lv_cont_set_layout(ui->screen_ps2_cont_1, LV_LAYOUT_OFF);
	lv_cont_set_fit(ui->screen_ps2_cont_1, LV_FIT_NONE);

	//Write codes screen_ps2_title
	ui->screen_ps2_title = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_title, "PS2手柄数据");
	lv_label_set_long_mode(ui->screen_ps2_title, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_title, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_title
	static lv_style_t style_screen_ps2_title_main;
	lv_style_reset(&style_screen_ps2_title_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_title_main
	lv_style_set_radius(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_title_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_title_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_title_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_title_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_title_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_ps2_title_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Normal_16);
	lv_style_set_text_letter_space(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_title_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_title, LV_LABEL_PART_MAIN, &style_screen_ps2_title_main);
	lv_obj_set_pos(ui->screen_ps2_title, 15, 5);
	lv_obj_set_width(ui->screen_ps2_title, 130);

	//Write codes screen_ps2_label_10
	ui->screen_ps2_label_10 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_10, "");
	lv_label_set_long_mode(ui->screen_ps2_label_10, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_10, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_10
	static lv_style_t style_screen_ps2_label_10_main;
	lv_style_reset(&style_screen_ps2_label_10_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_10_main
	lv_style_set_radius(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_line_space(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_10_main, LV_STATE_DEFAULT, 1);
	lv_obj_add_style(ui->screen_ps2_label_10, LV_LABEL_PART_MAIN, &style_screen_ps2_label_10_main);
	lv_obj_set_pos(ui->screen_ps2_label_10, 30, 32);
	lv_obj_set_width(ui->screen_ps2_label_10, 16);

	//Write codes screen_ps2_label_11
	ui->screen_ps2_label_11 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_11, "");
	lv_label_set_long_mode(ui->screen_ps2_label_11, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_11, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_11
	static lv_style_t style_screen_ps2_label_11_main;
	lv_style_reset(&style_screen_ps2_label_11_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_11_main
	lv_style_set_radius(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_line_space(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_11_main, LV_STATE_DEFAULT, 1);
	lv_obj_add_style(ui->screen_ps2_label_11, LV_LABEL_PART_MAIN, &style_screen_ps2_label_11_main);
	lv_obj_set_pos(ui->screen_ps2_label_11, 30, 51);
	lv_obj_set_width(ui->screen_ps2_label_11, 15);

	//Write codes screen_ps2_label_12
	ui->screen_ps2_label_12 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_12, "");
	lv_label_set_long_mode(ui->screen_ps2_label_12, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_12, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_12
	static lv_style_t style_screen_ps2_label_12_main;
	lv_style_reset(&style_screen_ps2_label_12_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_12_main
	lv_style_set_radius(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_line_space(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_12_main, LV_STATE_DEFAULT, 1);
	lv_obj_add_style(ui->screen_ps2_label_12, LV_LABEL_PART_MAIN, &style_screen_ps2_label_12_main);
	lv_obj_set_pos(ui->screen_ps2_label_12, 14, 42);
	lv_obj_set_width(ui->screen_ps2_label_12, 16);

	//Write codes screen_ps2_label_13
	ui->screen_ps2_label_13 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_13, "");
	lv_label_set_long_mode(ui->screen_ps2_label_13, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_13, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_13
	static lv_style_t style_screen_ps2_label_13_main;
	lv_style_reset(&style_screen_ps2_label_13_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_13_main
	lv_style_set_radius(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_line_space(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_13_main, LV_STATE_DEFAULT, 1);
	lv_obj_add_style(ui->screen_ps2_label_13, LV_LABEL_PART_MAIN, &style_screen_ps2_label_13_main);
	lv_obj_set_pos(ui->screen_ps2_label_13, 45, 42);
	lv_obj_set_width(ui->screen_ps2_label_13, 16);

	//Write codes screen_ps2_label_14
	ui->screen_ps2_label_14 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_14, "X:128");
	lv_label_set_long_mode(ui->screen_ps2_label_14, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_14, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_14
	static lv_style_t style_screen_ps2_label_14_main;
	lv_style_reset(&style_screen_ps2_label_14_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_14_main
	lv_style_set_radius(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, lv_color_make(0xf0, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_14_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_14, LV_LABEL_PART_MAIN, &style_screen_ps2_label_14_main);
	lv_obj_set_pos(ui->screen_ps2_label_14, 0, 69);
	lv_obj_set_width(ui->screen_ps2_label_14, 40);

	//Write codes screen_ps2_label_15
	ui->screen_ps2_label_15 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_15, "Y:128");
	lv_label_set_long_mode(ui->screen_ps2_label_15, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_15, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_15
	static lv_style_t style_screen_ps2_label_15_main;
	lv_style_reset(&style_screen_ps2_label_15_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_15_main
	lv_style_set_radius(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0xff, 0x00));
	lv_style_set_text_font(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_15_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_15, LV_LABEL_PART_MAIN, &style_screen_ps2_label_15_main);
	lv_obj_set_pos(ui->screen_ps2_label_15, 38, 69);
	lv_obj_set_width(ui->screen_ps2_label_15, 40);

	//Write codes screen_ps2_label_16
	ui->screen_ps2_label_16 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_16, "X:128");
	lv_label_set_long_mode(ui->screen_ps2_label_16, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_16, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_16
	static lv_style_t style_screen_ps2_label_16_main;
	lv_style_reset(&style_screen_ps2_label_16_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_16_main
	lv_style_set_radius(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, lv_color_make(0xf0, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_16_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_16, LV_LABEL_PART_MAIN, &style_screen_ps2_label_16_main);
	lv_obj_set_pos(ui->screen_ps2_label_16, 83, 69);
	lv_obj_set_width(ui->screen_ps2_label_16, 40);

	//Write codes screen_ps2_label_17
	ui->screen_ps2_label_17 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_17, "Y:128");
	lv_label_set_long_mode(ui->screen_ps2_label_17, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_17, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_17
	static lv_style_t style_screen_ps2_label_17_main;
	lv_style_reset(&style_screen_ps2_label_17_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_17_main
	lv_style_set_radius(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0xff, 0x00));
	lv_style_set_text_font(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_17_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_17, LV_LABEL_PART_MAIN, &style_screen_ps2_label_17_main);
	lv_obj_set_pos(ui->screen_ps2_label_17, 121, 69);
	lv_obj_set_width(ui->screen_ps2_label_17, 40);

	//Write codes screen_ps2_label_20
	ui->screen_ps2_label_20 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_20, "");
	lv_label_set_long_mode(ui->screen_ps2_label_20, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_20, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_20
	static lv_style_t style_screen_ps2_label_20_main;
	lv_style_reset(&style_screen_ps2_label_20_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_20_main
	lv_style_set_radius(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_line_space(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_20_main, LV_STATE_DEFAULT, 1);
	lv_obj_add_style(ui->screen_ps2_label_20, LV_LABEL_PART_MAIN, &style_screen_ps2_label_20_main);
	lv_obj_set_pos(ui->screen_ps2_label_20, 114, 51);
	lv_obj_set_width(ui->screen_ps2_label_20, 15);

	//Write codes screen_ps2_label_21
	ui->screen_ps2_label_21 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_21, "");
	lv_label_set_long_mode(ui->screen_ps2_label_21, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_21, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_21
	static lv_style_t style_screen_ps2_label_21_main;
	lv_style_reset(&style_screen_ps2_label_21_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_21_main
	lv_style_set_radius(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_line_space(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_21_main, LV_STATE_DEFAULT, 1);
	lv_obj_add_style(ui->screen_ps2_label_21, LV_LABEL_PART_MAIN, &style_screen_ps2_label_21_main);
	lv_obj_set_pos(ui->screen_ps2_label_21, 130, 42);
	lv_obj_set_width(ui->screen_ps2_label_21, 15);

	//Write codes screen_ps2_label_22
	ui->screen_ps2_label_22 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_22, "");
	lv_label_set_long_mode(ui->screen_ps2_label_22, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_22, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_22
	static lv_style_t style_screen_ps2_label_22_main;
	lv_style_reset(&style_screen_ps2_label_22_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_22_main
	lv_style_set_radius(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_line_space(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_22_main, LV_STATE_DEFAULT, 1);
	lv_obj_add_style(ui->screen_ps2_label_22, LV_LABEL_PART_MAIN, &style_screen_ps2_label_22_main);
	lv_obj_set_pos(ui->screen_ps2_label_22, 99, 42);
	lv_obj_set_width(ui->screen_ps2_label_22, 15);

	//Write codes screen_ps2_label_23
	ui->screen_ps2_label_23 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_23, "");
	lv_label_set_long_mode(ui->screen_ps2_label_23, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_23, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_23
	static lv_style_t style_screen_ps2_label_23_main;
	lv_style_reset(&style_screen_ps2_label_23_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_23_main
	lv_style_set_radius(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_23_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_23, LV_LABEL_PART_MAIN, &style_screen_ps2_label_23_main);
	lv_obj_set_pos(ui->screen_ps2_label_23, 31, 22);
	lv_obj_set_width(ui->screen_ps2_label_23, 12);

	//Write codes screen_ps2_label_24
	ui->screen_ps2_label_24 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_24, "");
	lv_label_set_long_mode(ui->screen_ps2_label_24, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_24, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_24
	static lv_style_t style_screen_ps2_label_24_main;
	lv_style_reset(&style_screen_ps2_label_24_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_24_main
	lv_style_set_radius(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_24_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_24, LV_LABEL_PART_MAIN, &style_screen_ps2_label_24_main);
	lv_obj_set_pos(ui->screen_ps2_label_24, 31, 18);
	lv_obj_set_width(ui->screen_ps2_label_24, 12);

	//Write codes screen_ps2_label_25
	ui->screen_ps2_label_25 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_25, "");
	lv_label_set_long_mode(ui->screen_ps2_label_25, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_25, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_25
	static lv_style_t style_screen_ps2_label_25_main;
	lv_style_reset(&style_screen_ps2_label_25_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_25_main
	lv_style_set_radius(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_line_space(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_25_main, LV_STATE_DEFAULT, 1);
	lv_obj_add_style(ui->screen_ps2_label_25, LV_LABEL_PART_MAIN, &style_screen_ps2_label_25_main);
	lv_obj_set_pos(ui->screen_ps2_label_25, 114, 32);
	lv_obj_set_width(ui->screen_ps2_label_25, 15);

	//Write codes screen_ps2_label_26
	ui->screen_ps2_label_26 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_26, "");
	lv_label_set_long_mode(ui->screen_ps2_label_26, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_26, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_26
	static lv_style_t style_screen_ps2_label_26_main;
	lv_style_reset(&style_screen_ps2_label_26_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_26_main
	lv_style_set_radius(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_26_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_26, LV_LABEL_PART_MAIN, &style_screen_ps2_label_26_main);
	lv_obj_set_pos(ui->screen_ps2_label_26, 115, 18);
	lv_obj_set_width(ui->screen_ps2_label_26, 12);

	//Write codes screen_ps2_label_27
	ui->screen_ps2_label_27 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_27, "");
	lv_label_set_long_mode(ui->screen_ps2_label_27, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_27, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_27
	static lv_style_t style_screen_ps2_label_27_main;
	lv_style_reset(&style_screen_ps2_label_27_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_27_main
	lv_style_set_radius(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_27_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_27, LV_LABEL_PART_MAIN, &style_screen_ps2_label_27_main);
	lv_obj_set_pos(ui->screen_ps2_label_27, 115, 22);
	lv_obj_set_width(ui->screen_ps2_label_27, 12);

	//Write codes screen_ps2_label_28
	ui->screen_ps2_label_28 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_28, "");
	lv_label_set_long_mode(ui->screen_ps2_label_28, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_28, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_28
	static lv_style_t style_screen_ps2_label_28_main;
	lv_style_reset(&style_screen_ps2_label_28_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_28_main
	lv_style_set_radius(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_28_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_28, LV_LABEL_PART_MAIN, &style_screen_ps2_label_28_main);
	lv_obj_set_pos(ui->screen_ps2_label_28, 66, 42);
	lv_obj_set_width(ui->screen_ps2_label_28, 12);

	//Write codes screen_ps2_label_29
	ui->screen_ps2_label_29 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_29, "");
	lv_label_set_long_mode(ui->screen_ps2_label_29, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_29, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_29
	static lv_style_t style_screen_ps2_label_29_main;
	lv_style_reset(&style_screen_ps2_label_29_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_29_main
	lv_style_set_radius(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_29_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_29, LV_LABEL_PART_MAIN, &style_screen_ps2_label_29_main);
	lv_obj_set_pos(ui->screen_ps2_label_29, 82, 42);
	lv_obj_set_width(ui->screen_ps2_label_29, 12);

	//Write codes screen_ps2_label_30
	ui->screen_ps2_label_30 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_30, "");
	lv_label_set_long_mode(ui->screen_ps2_label_30, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_30, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_30
	static lv_style_t style_screen_ps2_label_30_main;
	lv_style_reset(&style_screen_ps2_label_30_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_30_main
	lv_style_set_radius(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_30_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_30, LV_LABEL_PART_MAIN, &style_screen_ps2_label_30_main);
	lv_obj_set_pos(ui->screen_ps2_label_30, 66, 54);
	lv_obj_set_width(ui->screen_ps2_label_30, 12);

	//Write codes screen_ps2_label_31
	ui->screen_ps2_label_31 = lv_label_create(ui->screen_ps2, NULL);
	lv_label_set_text(ui->screen_ps2_label_31, "");
	lv_label_set_long_mode(ui->screen_ps2_label_31, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_ps2_label_31, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_ps2_label_31
	static lv_style_t style_screen_ps2_label_31_main;
	lv_style_reset(&style_screen_ps2_label_31_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_ps2_label_31_main
	lv_style_set_radius(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, lv_color_make(0x4f, 0x4f, 0x4f));
	lv_style_set_text_font(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, &lv_font_ps2_icons_14);
	lv_style_set_text_letter_space(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_ps2_label_31_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_ps2_label_31, LV_LABEL_PART_MAIN, &style_screen_ps2_label_31_main);
	lv_obj_set_pos(ui->screen_ps2_label_31, 81, 54);
	lv_obj_set_width(ui->screen_ps2_label_31, 12);

	//Init events for screen
	events_init_screen_ps2(ui);
}