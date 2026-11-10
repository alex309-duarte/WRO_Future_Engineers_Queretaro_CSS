#ifndef __ACKERMANN_CHASSIS_H
#define __ACKERMANN_CHASSIS_H
#include "chassis.h"

typedef struct {
	ChassisTypeDef base;
	float wheelbase;
	float shaft_length;
	float wheel_diameter;
	float correction_factor;
	void (*set_motors)(void *self, float rps_l, float rps_r,int position);
}AckermannChassisTypeDef;
void ackermann_chassis_object_init(AckermannChassisTypeDef *self);

#endif

