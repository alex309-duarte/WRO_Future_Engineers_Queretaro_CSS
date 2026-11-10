/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: MIT
 */

#include "events_init.h"
#include <stdio.h>
#include "lvgl/lvgl.h"
extern void update_sbus_view(void);

void events_init(lv_ui *ui)
{
}

static void screen_emptyevent_handler(lv_obj_t * obj, lv_event_t event)
{
	switch (event)
	{
	case LV_EVENT_CLICKED:
	{
		lv_disp_t * d = lv_obj_get_disp(lv_scr_act());
		if (d->prev_scr == NULL && d->scr_to_load == NULL)
		{
			if (guider_ui.screen_startup_del == true)
				setup_scr_screen_startup(&guider_ui);
			lv_scr_load_anim(guider_ui.screen_startup, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
		}
		guider_ui.screen_empty_del = false;
	}
		break;
	default:
		break;
	}
}

void events_init_screen_empty(lv_ui *ui)
{
	lv_obj_set_event_cb(ui->screen_empty, screen_emptyevent_handler);
}

static void screen_startupevent_handler(lv_obj_t * obj, lv_event_t event)
{
	switch (event)
	{
	case LV_EVENT_CLICKED:
	{
		lv_disp_t * d = lv_obj_get_disp(lv_scr_act());
		if (d->prev_scr == NULL && d->scr_to_load == NULL)
		{
			if (guider_ui.screen_sys_del == true)
				setup_scr_screen_sys(&guider_ui);
			lv_scr_load_anim(guider_ui.screen_sys, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
		}
		guider_ui.screen_startup_del = false;
	}
		break;
	default:
		break;
	}
}

void events_init_screen_startup(lv_ui *ui)
{
	lv_obj_set_event_cb(ui->screen_startup, screen_startupevent_handler);
}

static void screen_sysevent_handler(lv_obj_t * obj, lv_event_t event)
{
	switch (event)
	{
	case LV_EVENT_CLICKED:
	{
		lv_disp_t * d = lv_obj_get_disp(lv_scr_act());
		if (d->prev_scr == NULL && d->scr_to_load == NULL)
		{
			if (guider_ui.screen_imu_del == true)
				setup_scr_screen_imu(&guider_ui);
			lv_scr_load_anim(guider_ui.screen_imu, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
		}
		guider_ui.screen_sys_del = false;
	}
		break;
	default:
		break;
	}
}

void events_init_screen_sys(lv_ui *ui)
{
	lv_obj_set_event_cb(ui->screen_sys, screen_sysevent_handler);
}

static void screen_imuevent_handler(lv_obj_t * obj, lv_event_t event)
{
	switch (event)
	{
	case LV_EVENT_CLICKED:
	{
		lv_disp_t * d = lv_obj_get_disp(lv_scr_act());
		if (d->prev_scr == NULL && d->scr_to_load == NULL)
		{
			if (guider_ui.screen_ps2_del == true)
				setup_scr_screen_ps2(&guider_ui);
			lv_scr_load_anim(guider_ui.screen_ps2, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
		}
		guider_ui.screen_imu_del = false;
	}
		break;
	default:
		break;
	}
}

void events_init_screen_imu(lv_ui *ui)
{
	lv_obj_set_event_cb(ui->screen_imu, screen_imuevent_handler);
}

static void screen_ps2event_handler(lv_obj_t * obj, lv_event_t event)
{
	switch (event)
	{
	case LV_EVENT_CLICKED:
	{
		lv_disp_t * d = lv_obj_get_disp(lv_scr_act());
		if (d->prev_scr == NULL && d->scr_to_load == NULL)
		{
			if (guider_ui.screen_sbus_del == true)
				setup_scr_screen_sbus(&guider_ui);
			lv_scr_load_anim(guider_ui.screen_sbus, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
		}
		guider_ui.screen_ps2_del = false;
	}
		break;
	default:
		break;
	}
}

void events_init_screen_ps2(lv_ui *ui)
{
	lv_obj_set_event_cb(ui->screen_ps2, screen_ps2event_handler);
}

static void screen_sbusevent_handler(lv_obj_t * obj, lv_event_t event)
{
	switch (event)
	{
	case LV_EVENT_PRESSED:
	{
		update_sbus_view();
	}
		break;
	case LV_EVENT_CLICKED:
	{
		lv_disp_t * d = lv_obj_get_disp(lv_scr_act());
		if (d->prev_scr == NULL && d->scr_to_load == NULL)
		{
			if (guider_ui.screen_sys_del == true)
				setup_scr_screen_sys(&guider_ui);
			lv_scr_load_anim(guider_ui.screen_sys, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
		}
		guider_ui.screen_sbus_del = false;
	}
		break;
	default:
		break;
	}
}

void events_init_screen_sbus(lv_ui *ui)
{
	lv_obj_set_event_cb(ui->screen_sbus, screen_sbusevent_handler);
}
