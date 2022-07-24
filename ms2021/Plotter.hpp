/*
    Plotter.hpp

    Copyright © 2021 Wataru Taniguchi. All rights reserved.
*/
#ifndef Plotter_hpp
#define Plotter_hpp

#define TIRE_DIAMETER     90.0F  /* diameter of tire in milimater           */
#define WHEEL_TREAD      140.0F  /* distance between right and left wheels  */

#include "GyroSensor.h"
#include "Motor.h"

/* M_PI and M_TWOPI is NOT available even with math header file under -std=c++11
   because they are not strictly comforming to C++11 standards
   this program is compiled under -std=gnu++11 option */
#include <math.h>

class Plotter {
public:
    Plotter(ev3api::Motor* lm, ev3api::Motor* rm, ev3api::GyroSensor* gs);
    int32_t getDistance();
    int16_t getAzimuth();
    int16_t getDegree();
    int32_t getLocX();
    int32_t getLocY();
    int32_t getAngL();
    int32_t getAngR();
    int32_t setDistanceRecord(int32_t deltaDist);
    int32_t getDistanceRecord();
    int32_t setDistanceRecord_prev(int32_t deltaDist);
    int32_t getDistanceRecord_prev();
    void plot();
protected:
    ev3api::Motor *leftMotor, *rightMotor;
    ev3api::GyroSensor *gyroSensor;
    double distance, azimuth, locX, locY;
    int32_t prevAngL, prevAngR, distanceRecord,distanceRecord_prev;
};

#endif /* Plotter_hpp */