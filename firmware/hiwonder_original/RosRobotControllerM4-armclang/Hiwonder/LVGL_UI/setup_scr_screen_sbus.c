/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"


void setup_scr_screen_sbus(lv_ui *ui){

	//Write codes screen_sbus
	ui->screen_sbus = lv_obj_create(NULL, NULL);

	//Write style LV_OBJ_PART_MAIN for screen_sbus
	static lv_style_t style_screen_sbus_main;
	lv_style_reset(&style_screen_sbus_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_main
	lv_style_set_bg_color(&style_screen_sbus_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_opa(&style_screen_sbus_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_sbus, LV_OBJ_PART_MAIN, &style_screen_sbus_main);

	//Write codes screen_sbus_cont_1
	ui->screen_sbus_cont_1 = lv_cont_create(ui->screen_sbus, NULL);

	//Write style LV_CONT_PART_MAIN for screen_sbus_cont_1
	static lv_style_t style_screen_sbus_cont_1_main;
	lv_style_reset(&style_screen_sbus_cont_1_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_cont_1_main
	lv_style_set_radius(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, 5);
	lv_style_set_bg_color(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xbf, 0x75));
	lv_style_set_bg_grad_color(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, 225);
	lv_style_set_shadow_color(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_border_color(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0x99, 0x99, 0x99));
	lv_style_set_border_width(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_border_opa(&style_screen_sbus_cont_1_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_sbus_cont_1, LV_CONT_PART_MAIN, &style_screen_sbus_cont_1_main);
	lv_obj_set_pos(ui->screen_sbus_cont_1, 10, 2);
	lv_obj_set_size(ui->screen_sbus_cont_1, 140, 20);
	lv_obj_set_click(ui->screen_sbus_cont_1, false);
	lv_cont_set_layout(ui->screen_sbus_cont_1, LV_LAYOUT_OFF);
	lv_cont_set_fit(ui->screen_sbus_cont_1, LV_FIT_NONE);

	//Write codes screen_sbus_title
	ui->screen_sbus_title = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_title, "SBUS遥控数据");
	lv_label_set_long_mode(ui->screen_sbus_title, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_title, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_title
	static lv_style_t style_screen_sbus_title_main;
	lv_style_reset(&style_screen_sbus_title_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_title_main
	lv_style_set_radius(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_title_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_title_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_title_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_title_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_title_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sbus_title_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Normal_16);
	lv_style_set_text_letter_space(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_title_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_title, LV_LABEL_PART_MAIN, &style_screen_sbus_title_main);
	lv_obj_set_pos(ui->screen_sbus_title, 10, 5);
	lv_obj_set_width(ui->screen_sbus_title, 140);

	//Write codes screen_sbus_label_lx
	ui->screen_sbus_label_lx = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_lx, "LX:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_lx, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_lx, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_lx
	static lv_style_t style_screen_sbus_label_lx_main;
	lv_style_reset(&style_screen_sbus_label_lx_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_lx_main
	lv_style_set_radius(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, lv_color_make(0xf0, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_lx_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_lx, LV_LABEL_PART_MAIN, &style_screen_sbus_label_lx_main);
	lv_obj_set_pos(ui->screen_sbus_label_lx, 10, 59);
	lv_obj_set_width(ui->screen_sbus_label_lx, 69);

	//Write codes screen_sbus_label_ly
	ui->screen_sbus_label_ly = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_ly, "LY:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_ly, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_ly, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_ly
	static lv_style_t style_screen_sbus_label_ly_main;
	lv_style_reset(&style_screen_sbus_label_ly_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_ly_main
	lv_style_set_radius(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, lv_color_make(0x2f, 0xda, 0x64));
	lv_style_set_text_font(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_ly_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_ly, LV_LABEL_PART_MAIN, &style_screen_sbus_label_ly_main);
	lv_obj_set_pos(ui->screen_sbus_label_ly, 10, 71);
	lv_obj_set_width(ui->screen_sbus_label_ly, 69);

	//Write codes screen_sbus_label_sw5
	ui->screen_sbus_label_sw5 = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_sw5, "SW5:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_sw5, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_sw5, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_sw5
	static lv_style_t style_screen_sbus_label_sw5_main;
	lv_style_reset(&style_screen_sbus_label_sw5_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_sw5_main
	lv_style_set_radius(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_sw5_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_sw5, LV_LABEL_PART_MAIN, &style_screen_sbus_label_sw5_main);
	lv_obj_set_pos(ui->screen_sbus_label_sw5, 10, 23);
	lv_obj_set_width(ui->screen_sbus_label_sw5, 69);

	//Write codes screen_sbus_label_sw6
	ui->screen_sbus_label_sw6 = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_sw6, "SW6:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_sw6, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_sw6, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_sw6
	static lv_style_t style_screen_sbus_label_sw6_main;
	lv_style_reset(&style_screen_sbus_label_sw6_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_sw6_main
	lv_style_set_radius(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_sw6_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_sw6, LV_LABEL_PART_MAIN, &style_screen_sbus_label_sw6_main);
	lv_obj_set_pos(ui->screen_sbus_label_sw6, 10, 35);
	lv_obj_set_width(ui->screen_sbus_label_sw6, 69);

	//Write codes screen_sbus_label_sw7
	ui->screen_sbus_label_sw7 = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_sw7, "SW7:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_sw7, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_sw7, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_sw7
	static lv_style_t style_screen_sbus_label_sw7_main;
	lv_style_reset(&style_screen_sbus_label_sw7_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_sw7_main
	lv_style_set_radius(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_sw7_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_sw7, LV_LABEL_PART_MAIN, &style_screen_sbus_label_sw7_main);
	lv_obj_set_pos(ui->screen_sbus_label_sw7, 10, 47);
	lv_obj_set_width(ui->screen_sbus_label_sw7, 69);

	//Write codes screen_sbus_label_sw1
	ui->screen_sbus_label_sw1 = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_sw1, "SW1:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_sw1, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_sw1, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_sw1
	static lv_style_t style_screen_sbus_label_sw1_main;
	lv_style_reset(&style_screen_sbus_label_sw1_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_sw1_main
	lv_style_set_radius(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_sw1_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_sw1, LV_LABEL_PART_MAIN, &style_screen_sbus_label_sw1_main);
	lv_obj_set_pos(ui->screen_sbus_label_sw1, 79, 23);
	lv_obj_set_width(ui->screen_sbus_label_sw1, 69);

	//Write codes screen_sbus_label_sw2
	ui->screen_sbus_label_sw2 = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_sw2, "SW2:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_sw2, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_sw2, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_sw2
	static lv_style_t style_screen_sbus_label_sw2_main;
	lv_style_reset(&style_screen_sbus_label_sw2_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_sw2_main
	lv_style_set_radius(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_sw2_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_sw2, LV_LABEL_PART_MAIN, &style_screen_sbus_label_sw2_main);
	lv_obj_set_pos(ui->screen_sbus_label_sw2, 79, 35);
	lv_obj_set_width(ui->screen_sbus_label_sw2, 69);

	//Write codes screen_sbus_label_sw3
	ui->screen_sbus_label_sw3 = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_sw3, "SW3:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_sw3, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_sw3, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_sw3
	static lv_style_t style_screen_sbus_label_sw3_main;
	lv_style_reset(&style_screen_sbus_label_sw3_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_sw3_main
	lv_style_set_radius(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_sw3_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_sw3, LV_LABEL_PART_MAIN, &style_screen_sbus_label_sw3_main);
	lv_obj_set_pos(ui->screen_sbus_label_sw3, 79, 47);
	lv_obj_set_width(ui->screen_sbus_label_sw3, 69);

	//Write codes screen_sbus_label_rx
	ui->screen_sbus_label_rx = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_rx, "RX:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_rx, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_rx, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_rx
	static lv_style_t style_screen_sbus_label_rx_main;
	lv_style_reset(&style_screen_sbus_label_rx_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_rx_main
	lv_style_set_radius(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, lv_color_make(0xf0, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_rx_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_rx, LV_LABEL_PART_MAIN, &style_screen_sbus_label_rx_main);
	lv_obj_set_pos(ui->screen_sbus_label_rx, 79, 59);
	lv_obj_set_width(ui->screen_sbus_label_rx, 69);

	//Write codes screen_sbus_label_ry
	ui->screen_sbus_label_ry = lv_label_create(ui->screen_sbus, NULL);
	lv_label_set_text(ui->screen_sbus_label_ry, "RY:1280");
	lv_label_set_long_mode(ui->screen_sbus_label_ry, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sbus_label_ry, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sbus_label_ry
	static lv_style_t style_screen_sbus_label_ry_main;
	lv_style_reset(&style_screen_sbus_label_ry_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sbus_label_ry_main
	lv_style_set_radius(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, lv_color_make(0x2f, 0xda, 0x64));
	lv_style_set_text_font(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Light_12);
	lv_style_set_text_letter_space(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sbus_label_ry_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sbus_label_ry, LV_LABEL_PART_MAIN, &style_screen_sbus_label_ry_main);
	lv_obj_set_pos(ui->screen_sbus_label_ry, 79, 71);
	lv_obj_set_width(ui->screen_sbus_label_ry, 69);

	//Init events for screen
	events_init_screen_sbus(ui);
}