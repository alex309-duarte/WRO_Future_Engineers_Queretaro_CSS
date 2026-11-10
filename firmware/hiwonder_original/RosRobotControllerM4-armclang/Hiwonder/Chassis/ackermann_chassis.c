#include "ackermann_chassis.h"
#include "math.h"
#include <stdio.h>


#define PI 3.141592654f

static inline float linear_speed_to_rps(AckermannChassisTypeDef *self,  float speed)
{
    return speed / (PI * self->wheel_diameter);
}

/*vx mm/s
  angule_rate rad/s
*/


float ackermann_velocity_difference(AckermannChassisTypeDef *self,int size,float v1){
	int angle = 0;
	if (size > 500){angle = (size-500)/(1000/240);}
	else if(size == 500){float v2=v1;return v2;}
	else {angle = (500-size)/(1000/240);}
	
	double t = tan((angle*PI)/180);
	float v_t = ((AckermannChassisTypeDef*)self)->shaft_length / t + (((AckermannChassisTypeDef*)self)->wheel_diameter/2);
	float v2 = v1*((v_t+(((AckermannChassisTypeDef*)self)->wheel_diameter/2))/(v_t-(((AckermannChassisTypeDef*)self)->wheel_diameter/2)));
	
	return v2;
}

void ackermann_chassis_move(AckermannChassisTypeDef *self, float vx, float r )
{   
	  int servos = 500;
	  float rps_r ;
	  bool swerve = true;
	  float steering_ratio; 
	  float rps_l = linear_speed_to_rps(self, vx);
	  if(r == 0){steering_ratio = 0;}else{steering_ratio = ((AckermannChassisTypeDef*)self)->shaft_length / fabs(r);}
//		printf("%f/n",steering_ratio);
		if (r>0){swerve = true;}else if(r<0){swerve = false;}
		if (steering_ratio > tan(PI/180*30)){steering_ratio = tan(PI/180*30);}
		if (swerve == true){steering_ratio = steering_ratio;}else{steering_ratio = -steering_ratio;}
	  if(rps_l !=0){
		    if(steering_ratio != 0){
					  servos = (steering_ratio * 187.5f)+500;
					  
					  if (servos > 625){servos=625;}else if(servos < 375){servos=375;}
				}else if(steering_ratio == 0){
				    servos = 500;
				}
		
		rps_r = ackermann_velocity_difference(self,servos,rps_l);		
		}else{rps_r = rps_l;}
//		printf("%d/n",servos);
		if (swerve == true){self->set_motors(self, -rps_r, rps_l,servos);;
		}else{self->set_motors(self, rps_l,-rps_r,servos);}
    
}

static void stop(void *self)
{
    ((AckermannChassisTypeDef*)self)->set_motors(self, 0, 0,500);
}

static void set_velocity(void *self, float vx, float vy, float r)
{
    ackermann_chassis_move(self, vx, r);
}

static void set_velocity_radius(void* self, float linear, float r,bool swerve)
{
	if( 0 > r )
	{
		ackermann_chassis_move(self, -linear, r);
	} else {
		ackermann_chassis_move(self, linear, r);
	}
}

void ackermann_chassis_object_init(AckermannChassisTypeDef *self){
    self->base.stop = stop;
    self->base.set_velocity = set_velocity;
    self->base.set_velocity_radius = set_velocity_radius;
}