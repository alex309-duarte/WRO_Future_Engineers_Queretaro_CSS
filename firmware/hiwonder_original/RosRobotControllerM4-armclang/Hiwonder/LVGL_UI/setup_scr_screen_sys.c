/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"


void setup_scr_screen_sys(lv_ui *ui){

	//Write codes screen_sys
	ui->screen_sys = lv_obj_create(NULL, NULL);

	//Write style LV_OBJ_PART_MAIN for screen_sys
	static lv_style_t style_screen_sys_main;
	lv_style_reset(&style_screen_sys_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sys_main
	lv_style_set_bg_color(&style_screen_sys_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_opa(&style_screen_sys_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_sys, LV_OBJ_PART_MAIN, &style_screen_sys_main);

	//Write codes screen_sys_cont_1
	ui->screen_sys_cont_1 = lv_cont_create(ui->screen_sys, NULL);

	//Write style LV_CONT_PART_MAIN for screen_sys_cont_1
	static lv_style_t style_screen_sys_cont_1_main;
	lv_style_reset(&style_screen_sys_cont_1_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sys_cont_1_main
	lv_style_set_radius(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, 5);
	lv_style_set_bg_color(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xbf, 0x75));
	lv_style_set_bg_grad_color(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, 225);
	lv_style_set_shadow_color(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_border_color(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, lv_color_make(0x99, 0x99, 0x99));
	lv_style_set_border_width(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, 0);
	lv_style_set_border_opa(&style_screen_sys_cont_1_main, LV_STATE_DEFAULT, 255);
	lv_obj_add_style(ui->screen_sys_cont_1, LV_CONT_PART_MAIN, &style_screen_sys_cont_1_main);
	lv_obj_set_pos(ui->screen_sys_cont_1, 10, 2);
	lv_obj_set_size(ui->screen_sys_cont_1, 140, 20);
	lv_obj_set_click(ui->screen_sys_cont_1, false);
	lv_cont_set_layout(ui->screen_sys_cont_1, LV_LAYOUT_OFF);
	lv_cont_set_fit(ui->screen_sys_cont_1, LV_FIT_NONE);

	//Write codes screen_sys_label_volt111
	ui->screen_sys_label_volt111 = lv_label_create(ui->screen_sys, NULL);
	lv_label_set_text(ui->screen_sys_label_volt111, "电压:");
	lv_label_set_long_mode(ui->screen_sys_label_volt111, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sys_label_volt111, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sys_label_volt111
	static lv_style_t style_screen_sys_label_volt111_main;
	lv_style_reset(&style_screen_sys_label_volt111_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sys_label_volt111_main
	lv_style_set_radius(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_color(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Normal_14);
	lv_style_set_text_letter_space(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sys_label_volt111_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sys_label_volt111, LV_LABEL_PART_MAIN, &style_screen_sys_label_volt111_main);
	lv_obj_set_pos(ui->screen_sys_label_volt111, 4, 29);
	lv_obj_set_width(ui->screen_sys_label_volt111, 52);

	//Write codes screen_sys_title
	ui->screen_sys_title = lv_label_create(ui->screen_sys, NULL);
	lv_label_set_text(ui->screen_sys_title, "系统状态");
	lv_label_set_long_mode(ui->screen_sys_title, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sys_title, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sys_title
	static lv_style_t style_screen_sys_title_main;
	lv_style_reset(&style_screen_sys_title_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sys_title_main
	lv_style_set_radius(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sys_title_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sys_title_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sys_title_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sys_title_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sys_title_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sys_title_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sys_title_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Normal_16);
	lv_style_set_text_letter_space(&style_screen_sys_title_main, LV_STATE_DEFAULT, 2);
	lv_style_set_text_line_space(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sys_title_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sys_title, LV_LABEL_PART_MAIN, &style_screen_sys_title_main);
	lv_obj_set_pos(ui->screen_sys_title, 21, 5);
	lv_obj_set_width(ui->screen_sys_title, 118);

	//Write codes screen_sys_label_volt
	ui->screen_sys_label_volt = lv_label_create(ui->screen_sys, NULL);
	lv_label_set_text(ui->screen_sys_label_volt, "1100mv");
	lv_label_set_long_mode(ui->screen_sys_label_volt, LV_LABEL_LONG_BREAK);
	lv_label_set_align(ui->screen_sys_label_volt, LV_LABEL_ALIGN_CENTER);

	//Write style LV_LABEL_PART_MAIN for screen_sys_label_volt
	static lv_style_t style_screen_sys_label_volt_main;
	lv_style_reset(&style_screen_sys_label_volt_main);

	//Write style state: LV_STATE_DEFAULT for style_screen_sys_label_volt_main
	lv_style_set_radius(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_bg_color(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_color(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, lv_color_make(0xff, 0xff, 0xff));
	lv_style_set_bg_grad_dir(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, LV_GRAD_DIR_NONE);
	lv_style_set_bg_opa(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_color(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, lv_color_make(0x21, 0x95, 0xf6));
	lv_style_set_shadow_width(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_opa(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 255);
	lv_style_set_shadow_spread(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_x(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_shadow_ofs_y(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_text_color(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_font(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, &lv_font_SourceHanSansSC_Normal_14);
	lv_style_set_text_letter_space(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 1);
	lv_style_set_text_line_space(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_left(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_right(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_top(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_style_set_pad_bottom(&style_screen_sys_label_volt_main, LV_STATE_DEFAULT, 0);
	lv_obj_add_style(ui->screen_sys_label_volt, LV_LABEL_PART_MAIN, &style_screen_sys_label_volt_main);
	lv_obj_set_pos(ui->screen_sys_label_volt, 38, 30);
	lv_obj_set_width(ui->screen_sys_label_volt, 84);

	//Init events for screen
	events_init_screen_sys(ui);
}