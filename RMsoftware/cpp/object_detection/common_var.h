#ifndef _COMMON_VAR_H
#define _COMMON_VAR_H

#include <pthread.h>
#include <signal.h>

enum direction{
    right = 1,
    left = -1,
    front = 2,
    behind = -2,
    invalid = 0
};

enum Color_traffic_light{
    light_red = 2,
    light_green = 1,
    light_xparking = 3
};

enum Cube_number{
    cube_first = 2000,
    cube_second = 1250,

};
#endif // _COMMON_VAR_H