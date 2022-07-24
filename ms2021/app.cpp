/*
    app.cpp

    Copyright © 2021 Wataru Taniguchi. All rights reserved.
*/
#include "app.h"
#include "appusr.hpp"
#include <iostream>
#include <list>
#include <numeric>
#include <math.h>


/* this is to avoid linker error, undefined reference to `__sync_synchronize' */
extern "C" void __sync_synchronize() {}

/* global variables */
FILE*           bt;
Clock*          clock;
TouchSensor*    touchSensor;
SonarSensor*    sonarSensor;
FilteredColorSensor*    colorSensor;
GyroSensor*     gyroSensor;
SRLF*           srlf_l;
FilteredMotor*  leftMotor;
SRLF*           srlf_r;
FilteredMotor*  rightMotor;
Motor*          tailMotor;
Motor*          armMotor;
Plotter*        plotter;
Logger*         logger;

BrainTree::BehaviorTree* tr_calibration = nullptr;
BrainTree::BehaviorTree* tr_run         = nullptr;
BrainTree::BehaviorTree* tr_slalom      = nullptr;
BrainTree::BehaviorTree* tr_garage      = nullptr;
State state = ST_initial;

class IsTouchOn : public BrainTree::Node {
public:
    Status update() override {
        /* keep resetting clock until touch sensor gets pressed */
        clock->reset();
        if (touchSensor->isPressed()) {
            _log("touch sensor pressed.");
            /* indicate departure by LED color */
            ev3_led_set_color(LED_GREEN);
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
};

class IsBackOn : public BrainTree::Node {
public:
    Status update() override {
        if (ev3_button_is_pressed(BACK_BUTTON)) {
            _log("back button pressed.");
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
};

class IsTargetColorDetected : public BrainTree::Node {
public:
    IsTargetColorDetected(Color c) : color(c),updated(false) {}
    Status update() override {
        if (!updated) {
            originalTime = round((int32_t)clock->now()/10000);
            plotter->setDistanceRecord_prev(plotter->getDistanceRecord());
            prev_dis = plotter->getDistanceRecord();
            plotter->setDistanceRecord(prev_dis);
            updated = true;
            switch(color){
                case White_4:
                    plotter->setDistanceRecord(260);
                    break;
                default:
                    break;
            }
        }
        rgb_raw_t cur_rgb;
        colorSensor->getRawColor(cur_rgb);
        switch(color){
            case Black:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r <=50 && cur_rgb.g <=45 && cur_rgb.b <=60) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(deltaTime);
                    printf("Black deltaTime =%d\n",deltaTime);
                    _log("found black.");
                    return Status::Success;
                }
                break;
            case Black_2:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r <=30 && cur_rgb.g <=25 && cur_rgb.b <=40) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(prev_dis);
                    printf("Black2 deltaTime =%d prev_dis=%d\n",deltaTime,prev_dis);
                    _log("found black.");
                    return Status::Success;
                }
                break;
            case Black_3:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r <=30 && cur_rgb.g <=30 && cur_rgb.b <=40) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(prev_dis);
                    printf("Black3 deltaTime =%d prev_dis=%d\n",deltaTime,prev_dis);
                    _log("found black.");
                    return Status::Success;
                }
                break;
            case Blue:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.b - cur_rgb.r > 45 && cur_rgb.b <= 255 && cur_rgb.r <= 255) {
                    _log("found blue.");
                    return Status::Success;
                }
                break;
            case Red:
                if (cur_rgb.r - cur_rgb.b >= 40 && cur_rgb.g < 60 && cur_rgb.r - cur_rgb.g > 30) {
                    _log("found red.");
                    return Status::Success;
                }
                break;
            case Yellow:
                if (cur_rgb.r + cur_rgb.g - cur_rgb.b >= 130 &&  cur_rgb.r - cur_rgb.g <= 30) {
                    _log("found Yellow.");
                    return Status::Success;
                }
                break;
            case Green:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r <= 10 && cur_rgb.b <= 35 && cur_rgb.g > 43) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(deltaTime);
                    printf("Green deltaTime =%d\n",deltaTime);
                    _log("found Green.");
                    return Status::Success;
                }
                break;
            case Green_2:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
               if (cur_rgb.r <= 13 && cur_rgb.b <= 40 && cur_rgb.g >= 45){
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(deltaTime);
                    printf("Green_2 deltaTime =%d\n",deltaTime);
                    _log("found Green_2.");
                    return Status::Success;
                }
                break;
            case Green_4:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
               if (cur_rgb.r <= 57 && cur_rgb.b <= 85 && cur_rgb.g <= 75){
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(deltaTime);
                    printf("Green_4 deltaTime =%d\n",deltaTime);
                    _log("found Green_4.");
                    return Status::Success;
                }
                break;
            case Green_5:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
               if (cur_rgb.r <= 6 && cur_rgb.b <= 31 && cur_rgb.g <= 50){
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(prev_dis);
                    printf("Green_5 deltaTime =%d\n",deltaTime);
                    _log("found Green_5.");
                    return Status::Success;
                }
                break;
            case White:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r >= 78 && cur_rgb.b >= 110 && cur_rgb.g >= 75) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(deltaTime);
                    printf("White deltaTime =%d\n",deltaTime);
                    _log("found White.");
                    return Status::Success;
                }
                break;
            case White_2:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r >= 75 && cur_rgb.b >= 100 && cur_rgb.g >= 74) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    int32_t aa = round((int32_t)clock->now() / 10000) ;
                    plotter->setDistanceRecord(deltaTime);
                    printf("White_2 deltaTime =%d,clock=%d\n",deltaTime,aa);
                    _log("found White_2.");
                    return Status::Success;
                }
                break;
            case White_3:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r >= 81 && cur_rgb.b >= 111 && cur_rgb.g >= 77) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    int32_t aa = round((int32_t)clock->now() / 10000) ;
                    plotter->setDistanceRecord(deltaTime);
                    printf("White_3 deltaTime =%d,clock=%d\n",deltaTime,aa);
                    _log("found White_3.");
                    return Status::Success;
                }
                break;
            case Green_6:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r > 57 && cur_rgb.b > 85 && cur_rgb.g > 67) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(deltaTime);
                    printf("Green_6 deltaTime =%d\n",deltaTime);
                    _log("found Green_6.");
                    return Status::Success;
                }
                break;
            case Jetblack:
                if (cur_rgb.r <=35 && cur_rgb.g <=35 && cur_rgb.b <=50) { 
                    _log("found black.");
                    return Status::Success;
                }
                break;
            case Green_3:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r <= 79 && cur_rgb.b <= 109 && cur_rgb.g <= 77) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(deltaTime);
                    printf("Green_3 deltaTime =%d\n",deltaTime);
                    _log("found Green_3.");
                    return Status::Success;
                }
                break;
            case Gray:
                if (cur_rgb.r <=80 && cur_rgb.g <=75 && cur_rgb.b <=105) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                     plotter->setDistanceRecord(deltaTime);
                    printf("Gray deltaTime =%d\n",deltaTime); 
                    _log("found gray.");
                    return Status::Success;
                }
                break;
            case Gray_3:
                if (cur_rgb.r <=80 && cur_rgb.g <=75 && cur_rgb.b <=105) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                     plotter->setDistanceRecord(deltaTime);
                    printf("Gray3 deltaTime =%d\n",deltaTime); 
                    _log("found gray.");
                    return Status::Success;
                }
                break;
            case Gray_2:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r <= 50 && cur_rgb.b <= 69 && cur_rgb.g <= 48) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(deltaTime);
                    printf("Gray_2 deltaTime =%d\n",deltaTime);
                    _log("found Gray_2.");
                    return Status::Success;
                }
                break;
            case White_4:
                //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);
                if (cur_rgb.r >= 82 && cur_rgb.b >= 112 && cur_rgb.g >= 78) {
                    deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
                    plotter->setDistanceRecord(deltaTime);
                    printf("White_4 deltaTime =%d\n",deltaTime);
                    _log("found White_4.");
                    return Status::Success;
                }
                break;
            default:
                break;
        }
        return Status::Running;
    }
protected:
    Color color;
    bool updated;
    int32_t  originalTime, deltaTime,prev_dis;
};

class IsSonarOn : public BrainTree::Node {
public:
    IsSonarOn(int32_t d) : alertDistance(d) {}
    Status update() override {
        int32_t distance = sonarSensor->getDistance();
        if ((distance <= alertDistance) && (distance >= 0)) {
            _log("sonar alert at %d", distance);
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
protected:
    int32_t alertDistance;
};

class IsDistanceEarned : public BrainTree::Node {
public:
    IsDistanceEarned(int32_t d) : deltaDistTarget(d),updated(false),earned(false) {}
    Status update() override {
        if (!updated) {
            originalDist = plotter->getDistance();
            updated = true;
        }
        int32_t deltaDist = plotter->getDistance() - originalDist;
        
        if(deltaDist< 0){deltaDist= deltaDist* (-1);}

        if (deltaDist >= deltaDistTarget) {
            if (!earned) {
                _log("Delta %d is earned at absolute distance %d.", deltaDistTarget, plotter->getDistance());
                earned = true;
            }
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
protected:
    int32_t deltaDistTarget, originalDist;
    bool updated, earned;
};


/* argument -> 100 = 1 sec */
class IsTimeEarned : public BrainTree::Node {
public:
    IsTimeEarned(int32_t t) : deltaTimeTarget(t),updated(false),earned(false) {}
     Status update() override {
        if (!updated) {
            originalTime = round((int32_t)clock->now()/10000);
            updated = true;
        }
        deltaTime = round((int32_t)clock->now() / 10000) - originalTime;

        if (deltaTime >= deltaTimeTarget ) {
            if (!earned) {
                 _log("Delta %d getnow= %d", deltaTime,clock->now());
                earned = true;
            }
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
protected:
    int32_t deltaTimeTarget, originalTime, deltaTime;
    bool updated, earned;
};


/* argument -> 100 = 1 sec */
class IsTimeEarned2 : public BrainTree::Node {
public:
    IsTimeEarned2(int32_t t) : deltaTimeTarget(t),updated(false),earned(false) {}
     Status update() override {
        if (!updated) {
            originalTime = round((int32_t)clock->now()/10000);
            updated = true;
        }
        deltaTime = round((int32_t)clock->now() / 10000) - originalTime;
        int32_t distance = sonarSensor->getDistance();
        rgb_raw_t cur_rgb;
        colorSensor->getRawColor(cur_rgb);
        //printf("r=%d b=%d g=%d\n",cur_rgb.r, cur_rgb.b ,cur_rgb.g);

        if (deltaTime >= deltaTimeTarget ) {
            if (!earned) {
                printf("distance = %d\n",plotter->getDistance());
                 _log("Delta %d getnow= %d", deltaTime,clock->now());
                earned = true;
            }
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
protected:
    int32_t deltaTimeTarget, originalTime, deltaTime;
    bool updated, earned;
};


/* argument -> 100 = 1 sec */
class IsVariableTimeEarned : public BrainTree::Node {
public:
    IsVariableTimeEarned(double a, double b,bool o) :coefficient(a),intercept(b),option_add(o),deltaTimeTarget(0),deltaTime(0),updated(false),earned(false) {}
     Status update() override {
        if (!updated) {
            originalTime = round((int32_t)clock->now()/10000);
            updated = true;
            deltaTimeTarget = (int32_t)round(coefficient * (double)plotter->getDistanceRecord() + intercept);
            if(option_add){
                deltaTimeTarget =deltaTimeTarget - (int32_t)plotter->getDistanceRecord_prev();
            }
            printf("deltaTimeTarget = %d distance=%d\n",deltaTimeTarget,plotter->getDistanceRecord());
        }
        deltaTime = round((int32_t)clock->now() / 10000) - originalTime;

        if (deltaTime >= deltaTimeTarget ) {
            if (!earned) {
                 _log("Delta %d getnow= %d", deltaTime,clock->now());
                earned = true;
            }
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
protected:
    int32_t deltaTimeTarget, originalTime, deltaTime;
    double coefficient,intercept;
    bool updated, earned,option_add;
};


/* argument -> 100 = 1 sec */
class IsVariableTimeEarned2 : public BrainTree::Node {
public:
    IsVariableTimeEarned2(double a1,double a2, double b) :coefficient1(a1),coefficient2(a2),intercept(b),deltaTimeTarget(0),deltaTime(0),updated(false),earned(false) {}
     Status update() override {
        if (!updated) {
            originalTime = round((int32_t)clock->now()/10000);
            updated = true;
            deltaTimeTarget = (int32_t)round(coefficient1 * pow((double)plotter->getDistanceRecord(),2) + coefficient2 * (double)plotter->getDistanceRecord() + intercept);
            printf("2jou deltaTimeTarget = %d distance=%d\n",deltaTimeTarget,plotter->getDistanceRecord());
        }
        deltaTime = round((int32_t)clock->now() / 10000) - originalTime;

        if (deltaTime >= deltaTimeTarget ) {
            if (!earned) {
                 _log("Delta %d getnow= %d", deltaTime,clock->now());
                earned = true;
            }
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
protected:
    int32_t deltaTimeTarget, originalTime, deltaTime;
    double coefficient1,coefficient2,intercept;
    bool updated, earned;
};

/* argument -> 100 = 1 sec */
class IsVariableTimeEarned3 : public BrainTree::Node {
public:
    IsVariableTimeEarned3(double a,double m) :coefficient(a),multiplier(m),deltaTimeTarget(0),deltaTime(0),updated(false),earned(false) {}
     Status update() override {
        if (!updated) {
            originalTime = round((int32_t)clock->now()/10000);
            updated = true;
            deltaTimeTarget = (int32_t)round(coefficient * pow((double)plotter->getDistanceRecord(),multiplier) );
            printf("累乗近 deltaTimeTarget = %d distance=%d\n",deltaTimeTarget,plotter->getDistanceRecord());
        }
        deltaTime = round((int32_t)clock->now() / 10000) - originalTime;

        if (deltaTime >= deltaTimeTarget ) {
            if (!earned) {
                 _log("Delta %d getnow= %d", deltaTime,clock->now());
                earned = true;
            }
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
protected:
    int32_t deltaTimeTarget, originalTime, deltaTime;
    double coefficient,multiplier;
    bool updated, earned;
};



class IsCurveEarned : public BrainTree::Node {
public:
    IsCurveEarned(int32_t d,int32_t t,int32_t it ,CalcMode mode) : deltaDegreeTarget(d),interval(t),invalidTime(it),cnt(0),calcMode(mode),earned(false) {}
    Status update() override {
        if (cnt >= invalidTime/10 && cnt >= interval) {

            startDegree = plotter->getDegree();
            if(startDegree > 180){
                startDegree = startDegree - 360;
            }
            deltaDegree.insert(deltaDegree.begin(), startDegree);

            if (deltaDegree.size() >= interval){
                int32_t degree = startDegree - deltaDegree.back();
                //printf("startDegree = %d 5komae=%d sa = %d\n",startDegree,deltaDegree.back(),degree);
                switch (calcMode) {
                case Less:
                    if(startDegree - deltaDegree.back() <= deltaDegreeTarget){
                        _log("Delta %d getnow= %d", startDegree,clock->now());
                        return Status::Success;
                    }
                    break;    
                case More:
                    if(startDegree - deltaDegree.back() >= deltaDegreeTarget){
                        _log("Delta %d getnow= %d", startDegree,clock->now());
                        return Status::Success;
                    }
                    break;
                default:
                    break;
                }
                deltaDegree.pop_back();
            }
        }
        cnt ++ ;
        return Status::Failure;
    }
protected:
    int32_t deltaDegreeTarget, interval,invalidTime, cnt,startDegree;
    bool earned;
    CalcMode calcMode;
    std::vector<int32_t> deltaDegree;
};


class IsCurveAveEarned : public BrainTree::Node {
public:
    IsCurveAveEarned(int32_t d,int32_t t,int32_t it ,CalcMode mode) : deltaDegreeTarget(d),interval(t),invalidTime(it),cnt(0),calcMode(mode),earned(false) {}
    Status update() override {

        startDegree = plotter->getDegree();
        if(startDegree > 180){
            startDegree = startDegree - 360;
        }
        deltaDegree.insert(deltaDegree.begin(), startDegree);

        if (cnt >= invalidTime/10 && cnt >= interval) {
            aveDegree = (double)(accumulate(deltaDegree.begin(), deltaDegree.end(), 0)/interval);
            aveDeltaDegree.insert(aveDeltaDegree.begin(), aveDegree);

            if (aveDeltaDegree.size() >= interval){
                //printf("nowaveDegree = %f 5komae=%f sa = %f size=%d\n",aveDegree,aveDeltaDegree.back(),aveDegree - aveDeltaDegree.back(),aveDeltaDegree.size());
                switch (calcMode) {
                case Less:
                    if(aveDegree - aveDeltaDegree.back() <= deltaDegreeTarget){
                        _log("Delta %d getnow= %d", aveDegree,clock->now());
                        return Status::Success;
                    }
                    break;    
                case More:
                    if(aveDegree - aveDeltaDegree.back() >= deltaDegreeTarget){
                        _log("Delta %d getnow= %d", aveDegree,clock->now());
                        return Status::Success;
                    }
                    break;
                default:
                    break;
                }
                aveDeltaDegree.pop_back();
            }
        }
        if (deltaDegree.size() >= interval){
            deltaDegree.pop_back();
        }
        cnt ++ ;
        return Status::Failure;
    }
protected:
    int32_t deltaDegreeTarget, interval,invalidTime, cnt,startDegree;
    double aveDegree;
    bool earned;
    CalcMode calcMode;
    std::vector<int32_t> deltaDegree;
    std::vector<double> aveDeltaDegree;
};



class IsArmRepositioned : public BrainTree::Node {
public:
    Status update() override {
        if (armMotor->getCount() == ARM_INITIAL_ANGLE){
            return Status::Success;
        } else {
            return Status::Failure;
        }
    }
};

class TraceLine : public BrainTree::Node {
public:
    TraceLine(int s, int t, double p, double i, double d, double srew_rate) : speed(s),target(t),srewRate(srew_rate) {
        ltPid = new PIDcalculator(p, i, d, PERIOD_UPD_TSK, -speed, speed);
    }
    ~TraceLine() {
        delete ltPid;
    }
    Status update() override {
        int16_t sensor;
        int8_t forward, turn, pwm_L, pwm_R;
        rgb_raw_t cur_rgb;

        colorSensor->getRawColor(cur_rgb);
        sensor = cur_rgb.r;
        /* compute necessary amount of steering by PID control */
        turn = (-1) * _COURSE * ltPid->compute(sensor, (int16_t)target);
        forward = speed;
        /* steer EV3 by setting different speed to the motors */
        pwm_L = forward - turn;
        pwm_R = forward + turn;
        srlf_l->setRate(srewRate);
        leftMotor->setPWM(pwm_L);
        srlf_r->setRate(srewRate);
        rightMotor->setPWM(pwm_R);
        return Status::Running;
    }
protected:
    int speed, target;
    PIDcalculator* ltPid;
    double srewRate;
};

class ShiftTailPosition : public BrainTree::Node {
public:
    ShiftTailPosition(int tailpwm) : tailPwm(tailpwm),traceCnt(0) {}
    Status update() override {
        tailMotor->setPWM(tailPwm);
        return Status::Running;
    }
protected:
    int tailPwm;
private:
    int traceCnt;
};

class IsAngleEarned : public BrainTree::Node {
public:
    IsAngleEarned(int32_t d,CalcMode mode) : deltaDegreeTarget(d),cnt(0),calcMode(mode),earned(false) {}
    Status update() override {
        if(cnt==0){
            startDegree = plotter->getDegree();
            if(startDegree > 180){
                startDegree = startDegree - 360;
            }
        }
        switch (calcMode) {
        case Less:
            if(plotter->getDegree() - startDegree <= deltaDegreeTarget){
                _log("Delta %d getnow= %d", startDegree,clock->now());
                return Status::Success;
            }
            break;    
        case More:
            if(plotter->getDegree() -startDegree >= deltaDegreeTarget){
                _log("Delta %d getnow= %d", startDegree,clock->now());
                return Status::Success;
            }
            break;
        default:
            break;
        }
        cnt ++ ;
        return Status::Running;
    }
protected:
    int32_t deltaDegreeTarget,cnt,startDegree;
    bool earned;
    CalcMode calcMode;
};


class TraceLineOpposite : public BrainTree::Node {
public:
    TraceLineOpposite(int s, int t, double p, double i, double d, double srew_rate) : speed(s),target(t),srewRate(srew_rate) {
        ltPid = new PIDcalculator(p, i, d, PERIOD_UPD_TSK, -speed, speed);
    }
    ~TraceLineOpposite() {
        delete ltPid;
    }
    Status update() override {
        int16_t sensor;
        int8_t forward, turn, pwm_L, pwm_R;
        rgb_raw_t cur_rgb;

        colorSensor->getRawColor(cur_rgb);
        sensor = cur_rgb.r;
        /* compute necessary amount of steering by PID control */
        turn = _COURSE * ltPid->compute(sensor, (int16_t)target);
        forward = speed;
        /* steer EV3 by setting different speed to the motors */
        pwm_L = forward - turn;
        pwm_R = forward + turn;
        srlf_l->setRate(srewRate);
        leftMotor->setPWM(pwm_L);
        srlf_r->setRate(srewRate);
        rightMotor->setPWM(pwm_R);
        return Status::Running;
    }
protected:
    int speed, target;
    PIDcalculator* ltPid;
    double srewRate;
};


/*
    usage:
    ".leaf<RunAsInstructed>(pwm_l, pwm_r, srew_rate)"
    is to move the robot at the instructed speed.
    srew_rate = 0.0 indidates NO tropezoidal motion.
    srew_rate = 0.5 instructs FilteredMotor to change 1 pwm every two executions of update()
    until the current speed gradually reaches the instructed target speed.
*/
class RunAsInstructed : public BrainTree::Node {
public:
    RunAsInstructed(int pwm_l, int pwm_r, double srew_rate) : pwmL(pwm_l),pwmR(pwm_r),srewRate(srew_rate) {
        if (_COURSE == -1){
            pwm = pwmL;
            pwmL = pwmR;
            pwmR = pwm;            
        }     
    }
    Status update() override {
        srlf_l->setRate(srewRate);
        srlf_r->setRate(srewRate);
        leftMotor->setPWM(pwmL);
        rightMotor->setPWM(pwmR);
        return Status::Running;
    }
protected:
    int pwmL, pwmR,pwm;
    double srewRate;
};

class ShiftArmPosition : public BrainTree::Node {
public:
    ShiftArmPosition(int armpwm) : armPwm(armpwm),traceCnt(0) {}
    Status update() override {
        armMotor->setPWM(armPwm);
        return Status::Running;
    }
protected:
    int armPwm;
private:
    int traceCnt;
};

/*
    usage:
    ".leaf<RotateEV3>(30 * _COURSE, speed, srew_rate)"
    is to rotate robot 30 degrees clockwise at the speed when in L course
    srew_rate = 0.0 indidates NO tropezoidal motion.
    srew_rate = 0.5 instructs FilteredMotor to change 1 pwm every two executions of update()
    until the current speed gradually reaches the instructed target speed.
*/
class RotateEV3 : public BrainTree::Node {
public:
    RotateEV3(int16_t degree, int s, double srew_rate) : deltaDegreeTarget(degree),speed(s),srewRate(srew_rate),updated(false) {
        deltaDegreeTrpzMtrCtrl = 0;
        assert(degree >= -180 && degree <= 180);
        if (degree > 0) {
            clockwise = 1;
        } else {
            clockwise = -1;
        }
    }
    Status update() override {
        if (!updated) {
            originalDegree = plotter->getDegree();
            /* stop the robot at start */
            srlf_l->setRate(0.0);
            srlf_r->setRate(0.0);
            leftMotor->setPWM(0.0);
            rightMotor->setPWM(0.0);
            updated = true;
            return Status::Running;
        }
        int16_t deltaDegree = plotter->getDegree() - originalDegree;
        if (deltaDegree > 180) {
            deltaDegree -= 360;
        } else if (deltaDegree < -180) {
            deltaDegree += 360;
        }
        deltaDegree = deltaDegree * _COURSE;

        if (clockwise * deltaDegree < clockwise * deltaDegreeTarget) {
            srlf_l->setRate(srewRate);
            srlf_r->setRate(srewRate);
            if(clockwise * speed <= leftMotor->getPWM() * _COURSE && clockwise * deltaDegree < floor(clockwise * deltaDegreeTarget * 0.5) && deltaDegreeTrpzMtrCtrl == 0){
                deltaDegreeTrpzMtrCtrl = deltaDegree; 
            }else if(clockwise * speed > leftMotor->getPWM() * _COURSE && clockwise * deltaDegree >= floor(clockwise * deltaDegreeTarget * 0.5) && deltaDegreeTrpzMtrCtrl == 0){
                deltaDegreeTrpzMtrCtrl = deltaDegreeTarget;
            }

            if(clockwise * deltaDegree < clockwise * deltaDegreeTarget - deltaDegreeTrpzMtrCtrl ){
                leftMotor->setPWM(clockwise * speed * _COURSE);
                rightMotor->setPWM((-clockwise) * speed * _COURSE);
            }else{
                leftMotor->setPWM(clockwise * 3);
                rightMotor->setPWM((-clockwise) * 3);
            }
            return Status::Running;
        } else {
            return Status::Success;
        }
    }
private:
    int16_t deltaDegreeTarget, originalDegree, deltaDegreeTrpzMtrCtrl;
    int clockwise, speed;
    bool updated;
    double srewRate;
};

class IsTargetAngleEarned : public BrainTree::Node {
public:
    IsTargetAngleEarned(int angle, CalcMode mode) : targetAngle(angle),calcMode(mode),prevAngle(0) {}
    Status update() override {
        curAngle = gyroSensor->getAngle(); 

        switch (calcMode) {
        case Less:
            if(curAngle <= targetAngle){
                return Status::Success;
            }
            break;    
        case More:
            if(curAngle >= targetAngle){
                return Status::Success;
            }
            break;    
        default:
            break;
        }
        return Status::Running;
    }
protected:
    CalcMode calcMode;
    int targetAngle;
    int32_t curAngle,prevAngle;
    
};


/* a cyclic handler to activate a task */
void task_activator(intptr_t tskid) {
    ER ercd = act_tsk(tskid);
    assert(ercd == E_OK || E_QOVR);
    if (ercd != E_OK) {
        syslog(LOG_NOTICE, "act_tsk() returned %d", ercd);
    }
}

void main_task(intptr_t unused) {
    bt = ev3_serial_open_file(EV3_SERIAL_BT);
    assert(bt != NULL);
    /* create and initialize EV3 objects */
    clock       = new Clock();
    touchSensor = new TouchSensor(PORT_1);
    sonarSensor = new SonarSensor(PORT_2);
    colorSensor = new FilteredColorSensor(PORT_3);
    gyroSensor  = new GyroSensor(PORT_4);
    leftMotor   = new FilteredMotor(PORT_C);
    rightMotor  = new FilteredMotor(PORT_B);
    tailMotor   = new Motor(PORT_D);
    armMotor    = new Motor(PORT_A);
    plotter     = new Plotter(leftMotor, rightMotor, gyroSensor);
    logger      = new Logger();

    /* FIR parameters for a low-pass filter with normalized cut-off frequency of 0.2
        using a function of the Hamming Window */
    const int FIR_ORDER = 4; 
    const double hn[FIR_ORDER+1] = { 7.483914270309116e-03, 1.634745733863819e-01, 4.000000000000000e-01, 1.634745733863819e-01, 7.483914270309116e-03 };
    /* set filters to FilteredColorSensor */
    Filter *lpf_r = new FIR_Transposed(hn, FIR_ORDER);
    Filter *lpf_g = new FIR_Transposed(hn, FIR_ORDER);
    Filter *lpf_b = new FIR_Transposed(hn, FIR_ORDER);
    colorSensor->setRawColorFilters(lpf_r, lpf_g, lpf_b);

    srlf_l = new SRLF(0.0);
    leftMotor->setPWMFilter(srlf_l);
    srlf_r = new SRLF(0.0);
    rightMotor->setPWMFilter(srlf_r);

    /* BEHAVIOR TREE DEFINITION */

    /* robot starts when touch sensor is turned on */
    tr_calibration = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .decorator<BrainTree::UntilSuccess>()
            .leaf<IsTouchOn>()
        .end()
        .build();

    /* robot continues running unless:
        ultrasonic sonar detects an obstacle or
        back button is pressed or
        the second blue part of line is reached at further than BLUE_DISTANCE */
    //Left Course
    if(_COURSE==1){

        //Left Course
        //Left Course
        tr_run = (BrainTree::BehaviorTree*) BrainTree::Builder()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsBackOn>()
                .decorator<BrainTree::Inverter>()
                    .leaf<IsTargetAngleEarned>(-12,Less)
                .end()  
                .composite<BrainTree::ParallelSequence>(2,2)
                    .leaf<IsDistanceEarned>(BLUE_DISTANCE)
                    .composite<BrainTree::MemSequence>()
                        .leaf<IsTargetColorDetected>(Blue)
                        .leaf<IsTargetColorDetected>(Black)
                        .leaf<IsTargetColorDetected>(Blue)
                    .end()
                .end()
                .composite<BrainTree::MemSequence>()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(80)
                        .leaf<TraceLine>(65, GS_TARGET, 0.75, 1.0, D_CONST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(397)
                        .leaf<TraceLine>(SPEED_FAST, GS_TARGET, P_CONST_FAST, I_CONST_FAST, D_CONST_FAST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(300)
                        .leaf<TraceLine>(65, GS_TARGET, 0.75, 1.0, D_CONST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(2,2)
                        .leaf<IsTimeEarned>(55)
                        .leaf<IsTargetColorDetected>(Jetblack)
                        .leaf<RunAsInstructed>(62,62, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(135)
                        .leaf<RunAsInstructed>(35,85, 0.49)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(103)
                        .leaf<RunAsInstructed>(85,85, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(30)
                        .leaf<RunAsInstructed>(84,85, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(88)
                        .leaf<RunAsInstructed>(85,42, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(40)
                        .leaf<RunAsInstructed>(85,85, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(60)
                        .leaf<RunAsInstructed>(84,85, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(20)
                        .leaf<RunAsInstructed>(83,85, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(100)
                        .leaf<RunAsInstructed>(85,20, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(63)
                        .leaf<RunAsInstructed>(85,65, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(90)
                        .leaf<RunAsInstructed>(30,85, 1.0)
                    .end()

                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(40)
                        .leaf<RunAsInstructed>(25,70, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(103)
                        .leaf<RunAsInstructed>(84,100, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(65)
                        .leaf<RunAsInstructed>(40,85, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(57)
                        .leaf<RunAsInstructed>(85,85, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(142)
                        .leaf<RunAsInstructed>(85,26, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(2,1)
                        .decorator<BrainTree::Inverter>()
                            .leaf<IsTimeEarned>(500)
                        .end()                       
                        .leaf<IsTargetColorDetected>(Black)
                        .leaf<RunAsInstructed>(85,83, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsDistanceEarned>(50)
                        .leaf<RunAsInstructed>(70,65, 0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsDistanceEarned>(140)
                        .leaf<TraceLine>(SPEED_NORM, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetColorDetected>(Blue)
                        .leaf<TraceLine>(SPEED_FAST, GS_TARGET, P_CONST_FAST, I_CONST_FAST, D_CONST_FAST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsDistanceEarned>(3000)
                        .leaf<TraceLine>(65, GS_TARGET, 0.75, 1.0, D_CONST, 0.0)
                    .end()
                .end()
            .end()
            .build();

        //Left Course
        tr_slalom = (BrainTree::BehaviorTree*) BrainTree::Builder()
                .composite<BrainTree::MemSequence>()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsDistanceEarned>(170) // 170
                            .leaf<TraceLine>(SPEED_NORM, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetAngleEarned>(-10,Less)
                        .leaf<RunAsInstructed>(23,23, 0.0)
                        .leaf<ShiftArmPosition>(80)                
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetAngleEarned>(0,More)
                        .leaf<RunAsInstructed>(23,23, 0.0)
                        .leaf<ShiftArmPosition>(80)                
                    .end()


                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(100)
                        .leaf<ShiftArmPosition>(-100)
                        .leaf<RunAsInstructed>(0, 0, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsArmRepositioned>()
                        .leaf<ShiftArmPosition>(10)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(20)
                        .leaf<ShiftArmPosition>(0)
                    .end()


                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(540)
                        .leaf<TraceLine>(8, GS_TARGET_SLOW, P_CONST_SLOW, I_CONST_SLOW, D_CONST_SLOW, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(300)
                        .leaf<RunAsInstructed>(6,8, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(1245)
                        .leaf<TraceLine>(8, GS_TARGET_SLOW, P_CONST_SLOW, I_CONST_SLOW, D_CONST_SLOW, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(110)
                        .leaf<RunAsInstructed>(10,9, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(800)
                        .leaf<RunAsInstructed>(10,4, 0.5)
                        //.leaf<ShiftArmPosition>(-10) 
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(245)
                        .leaf<RunAsInstructed>(12,3, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(175)
                        .leaf<RunAsInstructed>(11,4, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(170)
                        .leaf<RunAsInstructed>(4,11, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(295)
                        .leaf<RunAsInstructed>(2,9, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(300)
                        .leaf<ShiftArmPosition>(-100)
                        .leaf<RunAsInstructed>(10,10, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(50)
                        .leaf<ShiftArmPosition>(30)
                        .leaf<RunAsInstructed>(10,10, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(180)
                        .leaf<ShiftArmPosition>(30)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(65)
                        .leaf<ShiftArmPosition>(-100)
                        .leaf<RunAsInstructed>(0, 0, 0.0)
                    .end()

                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsArmRepositioned>()
                        .leaf<ShiftArmPosition>(10)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(20)
                        .leaf<ShiftArmPosition>(0)
                    .end()
                .end()
                .build();


        //Left Course
        // 20210830
        tr_garage = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()

             //ブロック方面に曲がりながら指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(65)
                .leaf<RunAsInstructed>(30,10, 0.0)
            .end()

             //ブロック方面に曲がりながら指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(145)
                .leaf<RunAsInstructed>(30,6, 0.0)
            .end()

             //指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(90)
                .leaf<RunAsInstructed>(30,30, 0.0)
            .end()

            //黒色検知
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Black)
                .leaf<RunAsInstructed>(60,60, 1.0)
            .end()
             //指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(80)
                .leaf<RunAsInstructed>(50,50, 0.0)
            .end()

            //黒色検知
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Black)
                .leaf<RunAsInstructed>(50,50, 0.0)
            .end()
             //指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(100)
                .leaf<RunAsInstructed>(50,50, 0.0)
            .end()

            //黒色検知
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Black)
                .leaf<RunAsInstructed>(30,31, 1.0)
            .end()
            // 左に曲がる
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(90)
                .leaf<RunAsInstructed>(-20,30, 0.0)
            .end()
            // 青検知するまでライントーレス
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Blue)
                .leaf<TraceLine>(SPEED_SLOW, GS_TARGET_SLOW,  P_CONST_SLOW, I_CONST_SLOW,  D_CONST_SLOW, 0.0)
            .end()
            // 左に曲がる
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(140)
                .leaf<RunAsInstructed>(3,40, 0.0)
            .end()  
            // 赤検知するまでライントーレス
            // ※ライントレースは赤を白色とみなすため、赤検知すると大きく右に曲がる。曲がることを利用しブロックを掴む
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Red)
                .leaf<TraceLine>(SPEED_SLOW, GS_TARGET_SLOW,  P_CONST_SLOW, I_CONST_SLOW,  D_CONST_SLOW, 0.0)
            .end()

            // //スラローム後、アームの位置摩擦？でカーブ変化が出ることの調整
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(10)
                .leaf<ShiftArmPosition>(-3)
            .end()

            //指定時間走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(5)
                .leaf<RunAsInstructed>(30,30, 0.0)
            .end()

            //指定時間走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(180)
                .leaf<RunAsInstructed>(23,30, 0.0)
            .end()

            //指定時間走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(120)
                .leaf<RunAsInstructed>(50,50, 0.0)
            .end()
            //やや左に指定時間走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(160)
                .leaf<RunAsInstructed>(18,30, 0.0)
            .end()

            //やや右に指定時間走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(80)
                .leaf<RunAsInstructed>(30,25, 0.0)
            .end()

            //やや右に指定時間走行（別コースの黒色をかわす距離まで）
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(150)
                .leaf<RunAsInstructed>(30,22.5, 0.0)
            .end()

            //青か黒色検知するまでやや右に低速走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Blue)
                .leaf<IsTargetColorDetected>(Black)
                .leaf<RunAsInstructed>(30,22.5, 0.0)
            .end()

            // 回転
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(93)
                .leaf<RunAsInstructed>(30,-20, 0.0)
            .end()
            //ライントレース前スピード調整
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(5)
                .leaf<RunAsInstructed>(5,5, 0.0)
            .end()
            //ライントレース遅め
            .composite<BrainTree::ParallelSequence>(1,2)
                 .leaf<IsTimeEarned>(180)
               .leaf<TraceLine>(5, GS_TARGET_SLOW, P_CONST_SLOW, I_CONST_SLOW, D_CONST_SLOW, 0.0)
            .end() 

            //ライントレース＆ソナー
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsSonarOn>(29)
                .leaf<TraceLine>(19, GS_TARGET_SLOW, P_CONST_SLOW, I_CONST_SLOW, D_CONST_SLOW, 0.0)
            .end()

            //まっすぐ距離調整用 sonarが指定距離前に目標物を検知するまで
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsSonarOn>(4)
                .leaf<RunAsInstructed>(20,20, 0.0)
            .end()
            // ガレージ停止後カウント用
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(500)
                .leaf<RunAsInstructed>(0,0, 0)
            .end()
        .end()
        .build();


    }else{

    //Right Course
        tr_run = (BrainTree::BehaviorTree*) BrainTree::Builder()
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsBackOn>()
                .decorator<BrainTree::Inverter>()
                    .leaf<IsTargetAngleEarned>(-12,Less)
                .end()  
                .composite<BrainTree::ParallelSequence>(2,2)
                    .leaf<IsDistanceEarned>(BLUE_DISTANCE)
                    .composite<BrainTree::MemSequence>()
                        //.leaf<IsTargetColorDetected>(Blue)
                        .leaf<IsTargetColorDetected>(Black)
                        .leaf<IsTargetColorDetected>(Blue)
                    .end()
                .end()
                .composite<BrainTree::MemSequence>()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(80)
                        .leaf<TraceLine>(60, GS_TARGET, 0.75, 1.0, D_CONST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(63) //397
                        .leaf<TraceLine>(SPEED_FAST, GS_TARGET, P_CONST_FAST, I_CONST_FAST, D_CONST_FAST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetColorDetected>(Gray_2)
                        .leaf<IsTimeEarned>(53)
                        .leaf<RunAsInstructed>(83,100, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsVariableTimeEarned>(-1,53,false)
                        .leaf<RunAsInstructed>(83,100, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetColorDetected>(White_4)
                        .leaf<IsTimeEarned>(260)
                        .leaf<RunAsInstructed>(85,85, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsVariableTimeEarned>(-1,260,false)
                        .leaf<RunAsInstructed>(85,85, 0.0)
                    .end()
                    // .composite<BrainTree::ParallelSequence>(1,2)
                    //     .leaf<IsTimeEarned>(300)
                    //     .leaf<RunAsInstructed>(0,0, 0)
                    // .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        //.leaf<IsTimeEarned>(50)
                        .leaf<IsVariableTimeEarned2>(-0.0003,0.0757,49.477)
                        .leaf<RunAsInstructed>(35,85, 0.7)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(34)
                        .leaf<RunAsInstructed>(57,85, 0.7)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(62)
                        .leaf<RunAsInstructed>(57,85, 0.7)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        //.leaf<IsTimeEarned>(120)
                        .leaf<IsVariableTimeEarned2>(-0.0007,0.2355,111.08)
                        .leaf<RunAsInstructed>(85,85, 0.7)
                    .end()

                    // .composite<BrainTree::ParallelSequence>(1,2)
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(26)//31
                        .leaf<RunAsInstructed>(83,85, 0.7)
                    .end()
 
                    //コースの右下のヘアピン
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(118)
                        .leaf<RunAsInstructed>(35,85, 5)
                    .end()
                    // .composite<BrainTree::ParallelSequence>(1,2)
                    //     .leaf<IsTargetColorDetected>(Gray)
                    //     .leaf<IsTimeEarned>(120)
                    //     .leaf<RunAsInstructed>(100,100, 0.5)
                    // .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(120)
                        .leaf<RunAsInstructed>(100,100, 0.5)
                    .end()
                    //ヘアピン後のうねうねの始まりの角
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(84)
                        .leaf<RunAsInstructed>(100,45, 4.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(38)
                        .leaf<RunAsInstructed>(100,100, 1)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(64)
                        .leaf<RunAsInstructed>(99,100, 1.0)
                    .end()

                    //うねうねのひとつめのヘアピン
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(72)
                        .leaf<RunAsInstructed>(100,23, 3)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        //.leaf<IsTimeEarned>(45)
                        .leaf<IsTargetColorDetected>(Gray)
                        .leaf<RunAsInstructed>(100,76, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(30)
                        .leaf<RunAsInstructed>(70,100, 1)
                    .end()

                    
                    //うねうねのふたつめのヘアピン
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(7)
                        .leaf<RunAsInstructed>(35,100, 2.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,3)
                        .leaf<IsTimeEarned>(44)
                        .leaf<RunAsInstructed>(19,100, 3.1)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,3)
                        .leaf<IsTimeEarned>(124)
                        .leaf<RunAsInstructed>(100,100, 1.0)
                    .end()

                    //GATE２手前降下のための転回
                    .composite<BrainTree::ParallelSequence>(1,3)
                        .leaf<IsTimeEarned>(41)
                        .leaf<RunAsInstructed>(100,19, 4)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetColorDetected>(Green)
                        .leaf<RunAsInstructed>(84,100, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(13)
                        .leaf<RunAsInstructed>(84,100, 1.0)
                    .end()

                    //ショートカットパス後の左回り
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(42)
                        .leaf<RunAsInstructed>(25,100, 4)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetColorDetected>(Black)
                        .leaf<IsTimeEarned>(50)
                        .leaf<RunAsInstructed>(100,100, 2.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsVariableTimeEarned2>(0.0338,-6.5088,218.22)
                        .leaf<RunAsInstructed>(100,100, 2.0)
                    .end()
                    // .composite<BrainTree::ParallelSequence>(1,2)
                    //     .leaf<IsVariableTimeEarned>(-1,50,false)
                    //     .leaf<RunAsInstructed>(100,100, 2.0)
                    // .end()
                    //最後のストレートへの回り
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(21)//24
                        //.leaf<IsVariableTimeEarned2>(0.0226,-1.7053,53.173)
                        .leaf<RunAsInstructed>(100,25, 3)//25
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(32)
                        .leaf<RunAsInstructed>(100,100, 2.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(36)
                        .leaf<RunAsInstructed>(100,25, 3)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(280)
                        .leaf<RunAsInstructed>(100,100, 2.0)
                    .end()

                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(5)
                        .leaf<RunAsInstructed>(0,0, 0)
                    .end()

                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(85)
                        .leaf<RunAsInstructed>(40,-40, 1.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(5)
                        .leaf<RunAsInstructed>(0,0, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetColorDetected>(Green_2)
                        .leaf<RunAsInstructed>(39,39, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(5)
                        .leaf<RunAsInstructed>(0,0, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(45)
                        .leaf<RunAsInstructed>(-40,40, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(5)
                        .leaf<RunAsInstructed>(0,0, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(1)
                        .leaf<RunAsInstructed>(60,60, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetColorDetected>(Black)
                        .leaf<RunAsInstructed>(40,40, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(10)
                        .leaf<RunAsInstructed>(40,40, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(5)
                        .leaf<RunAsInstructed>(0,0, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(38)
                        .leaf<RunAsInstructed>(40,-40, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsDistanceEarned>(150)
                        .leaf<TraceLine>(30, GS_TARGET, 0.75, 1.0, D_CONST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsDistanceEarned>(3000)
                        .leaf<TraceLine>(65, GS_TARGET, 0.75, 1.0, D_CONST, 0.0)
                    .end()

                    // .composite<BrainTree::ParallelSequence>(1,2)
                    //     .leaf<IsDistanceEarned>(120)
                    //     .leaf<TraceLine>(SPEED_NORM, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0)
                    // .end()
                    // .composite<BrainTree::ParallelSequence>(1,2)
                    //     //.leaf<IsDistanceEarned>(1200)
                    //     .leaf<IsTargetColorDetected>(Blue)
                    //     .leaf<TraceLine>(SPEED_FAST, GS_TARGET, P_CONST_FAST, I_CONST_FAST, D_CONST_FAST, 0.0)
                    // .end()
                    // .composite<BrainTree::ParallelSequence>(1,2)
                    //     .leaf<IsDistanceEarned>(3000)
                    //     .leaf<TraceLine>(65, GS_TARGET, 0.75, 1.0, D_CONST, 0.0)
                    // .end()
                .end()
            .end()
            .build();

 //Right Course 本番用
        tr_slalom = (BrainTree::BehaviorTree*) BrainTree::Builder()
            .composite<BrainTree::MemSequence>()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsDistanceEarned>(170)
                            .leaf<TraceLine>(SPEED_NORM, GS_TARGET, P_CONST, I_CONST, D_CONST, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetAngleEarned>(-10,Less)
                        .leaf<RunAsInstructed>(23,23, 0.0)
                        .leaf<ShiftArmPosition>(80)                
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTargetAngleEarned>(0,More)
                        .leaf<RunAsInstructed>(23,23, 0.0)
                        .leaf<ShiftArmPosition>(80)                
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(150)
                        .leaf<RunAsInstructed>(0,0, 0.0)
                        .leaf<ShiftArmPosition>(-50)                
                    .end()

                    // // //試走会
                    // .composite<BrainTree::ParallelSequence>(1,2)
                    //     .leaf<IsTimeEarned>(5600)
                    //     .leaf<RunAsInstructed>(100,100, 0.0)
                    // .end()

                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(560)
                        .leaf<TraceLine>(9, GS_TARGET_SLOW, P_CONST_SLOW, I_CONST_SLOW, D_CONST_SLOW, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(300)
                        .leaf<RunAsInstructed>(7,10, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(1088)
                        .leaf<TraceLine>(SPEED_SLOW, GS_TARGET_SLOW, P_CONST_SLOW, I_CONST_SLOW, D_CONST_SLOW, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(110)
                        .leaf<RunAsInstructed>(10,9, 0.0)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(750)
                        .leaf<RunAsInstructed>(10,4, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(150)
                        .leaf<RunAsInstructed>(10,2, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(296)
                        .leaf<RunAsInstructed>(15,3, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(385)//335
                        .leaf<RunAsInstructed>(13,3, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(55)//25
                        .leaf<RunAsInstructed>(10,10, 1)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(350)//304
                        .leaf<RunAsInstructed>(2,10, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(165)
                        .leaf<RunAsInstructed>(2,12, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(300)
                        .leaf<RunAsInstructed>(10,10, 0.5)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(50)
                        .leaf<ShiftArmPosition>(30)
                        .leaf<RunAsInstructed>(10,10, 0.5)
                    .end()
                    // .composite<BrainTree::ParallelSequence>(1,2)
                    //     .leaf<IsTimeEarned>(350)
                    //     .leaf<ShiftArmPosition>(30)
                    // .end()
                    // .composite<BrainTree::ParallelSequence>(1,2)
                    //     .leaf<IsTimeEarned>(100)
                    //     .leaf<ShiftArmPosition>(-100)
                    //     .leaf<RunAsInstructed>(0, 0, 0.0)
                    // .end()

                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(180)
                        .leaf<ShiftArmPosition>(30)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(65)
                        .leaf<ShiftArmPosition>(-100)
                        .leaf<RunAsInstructed>(0, 0, 0.0)
                    .end()

                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsArmRepositioned>()
                        .leaf<ShiftArmPosition>(10)
                    .end()
                    .composite<BrainTree::ParallelSequence>(1,2)
                        .leaf<IsTimeEarned>(20)
                        .leaf<ShiftArmPosition>(0)
                    .end()

                .end()
                .build();


        //Right Course
        //Right 20210829
        tr_garage = (BrainTree::BehaviorTree*) BrainTree::Builder()
        .composite<BrainTree::MemSequence>()
// // 単体テストでのアーム位置調整ここから
//            .composite<BrainTree::ParallelSequence>(1,2)
//                .leaf<IsTimeEarned>(180)
//                .leaf<ShiftArmPosition>(30)
//            .end()
//            .composite<BrainTree::ParallelSequence>(1,2)
//                .leaf<IsTimeEarned>(65)
//                .leaf<ShiftArmPosition>(-100)
//                .leaf<RunAsInstructed>(0, 0, 0.0)
//            .end()
//            .composite<BrainTree::ParallelSequence>(1,2)
//                .leaf<IsArmRepositioned>()
//                .leaf<ShiftArmPosition>(10)
//            .end()
//            .composite<BrainTree::ParallelSequence>(1,2)
//                .leaf<IsTimeEarned>(20)
//                .leaf<ShiftArmPosition>(0)
//            .end()
// // 単体テストでのアーム位置調整ここまで
             //ブロック方面に曲がりながら指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(65)
                .leaf<RunAsInstructed>(30,10, 0.0)
            .end()

             //ブロック方面に曲がりながら指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(145)
                .leaf<RunAsInstructed>(30,6, 0.0)
            .end()
             //指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(80)
                .leaf<RunAsInstructed>(30,30, 0.0)
            .end()
            //黒色検知
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Black)
                .leaf<RunAsInstructed>(60,60, 1.0)
            .end()
             //指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(80)
                .leaf<RunAsInstructed>(50,50, 0.0)
            .end()

            //黒色検知
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Black)
                .leaf<RunAsInstructed>(50,50, 0.0)
            .end()
             //指定距離走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(85)
                .leaf<RunAsInstructed>(50,50, 0.0)
            .end()

            //黒色検知
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Black)
                .leaf<RunAsInstructed>(30,31, 1.0)
            .end()
            // 曲がる
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(90)
                .leaf<RunAsInstructed>(-20,30, 0.0)
            .end()
            // 青検知するまでライントーレス
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Blue)
                .leaf<TraceLine>(SPEED_SLOW, GS_TARGET_SLOW,  P_CONST_SLOW, I_CONST_SLOW,  D_CONST_SLOW, 0.0)
            .end()
            // 曲がる
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(140)
                .leaf<RunAsInstructed>(3,40, 0.0)
            .end()  
            // 赤検知するまでライントーレス
            // ※ライントレースは赤を白色とみなすため、赤検知すると大きく曲がる。曲がることを利用しブロックを掴む
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Red)
                .leaf<TraceLine>(SPEED_SLOW, GS_TARGET_SLOW,  P_CONST_SLOW, I_CONST_SLOW,  D_CONST_SLOW, 0.0)
            .end()

            // //スラローム後、アームの位置摩擦？でカーブ変化が出ることの調整
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(10)
                .leaf<ShiftArmPosition>(-3)
            .end()

            //指定時間走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(5)
                .leaf<RunAsInstructed>(30,30, 0.0)
            .end()

            //角度調整走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(180)
                .leaf<RunAsInstructed>(24,30, 0.0)
            .end()

            //指定時間走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(120)
                .leaf<RunAsInstructed>(50,50, 0.0)
            .end()
            //やや左に指定時間走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(160)
                .leaf<RunAsInstructed>(20,30, 0.0)
            .end()

            //角度調整走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(80)
                .leaf<RunAsInstructed>(30,22.5, 0.0)
            .end()
            //角度調整走行（別コースの黒色をかわす距離まで）
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(130)
                .leaf<RunAsInstructed>(30,22.5, 0.0)
            .end()
            
            //青か黒色検知するまでやや角度調整走行
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTargetColorDetected>(Blue)
                .leaf<IsTargetColorDetected>(Black)
                .leaf<RunAsInstructed>(30,22, 0.0)
            .end()

            // 回転
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(89)
                .leaf<RunAsInstructed>(40,-20, 0.0)
            .end()
            //ライントレース前スピード調整
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(5)
                .leaf<RunAsInstructed>(5,5, 0.0)
            .end()
            //ライントレース遅め
            .composite<BrainTree::ParallelSequence>(1,2)
                 .leaf<IsTimeEarned>(180)
               .leaf<TraceLine>(5, GS_TARGET_SLOW, P_CONST_SLOW, I_CONST_SLOW, D_CONST_SLOW, 0.0)
            .end() 

            //ライントレース＆ソナー
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsSonarOn>(29)
                .leaf<TraceLine>(19, GS_TARGET_SLOW, P_CONST_SLOW, I_CONST_SLOW, D_CONST_SLOW, 0.0)
            .end()

            //まっすぐ距離調整用 sonarが指定距離前に目標物を検知するまで
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsSonarOn>(3)
                .leaf<RunAsInstructed>(16,16, 0.0)
            .end()
            // ガレージ停止後カウント用
            .composite<BrainTree::ParallelSequence>(1,2)
                .leaf<IsTimeEarned>(500)
                .leaf<RunAsInstructed>(0,0, 0)
            .end()
        .end()
        .build();
    }

    /* register cyclic handler to EV3RT */
    sta_cyc(CYC_UPD_TSK);

    /* indicate initialization completion by LED color */
    _log("initialization completed.");
    ev3_led_set_color(LED_ORANGE);
    state = ST_calibrating;

    /* sleep until being waken up */
    _log("going to sleep...");
    ER ercd = slp_tsk();
    assert(ercd == E_OK);
    if (ercd != E_OK) {
        syslog(LOG_NOTICE, "slp_tsk() returned %d", ercd);
    }

    /* deregister cyclic handler from EV3RT */
    stp_cyc(CYC_UPD_TSK);
    /* destroy behavior tree */
    delete tr_garage;
    delete tr_slalom;
    delete tr_run;
    delete tr_calibration;
    /* destroy EV3 objects */
    delete lpf_b;
    delete lpf_g;
    delete lpf_r;
    delete plotter;
    delete armMotor;
    delete tailMotor;
    delete rightMotor;
    delete leftMotor;
    delete gyroSensor;
    delete colorSensor;
    delete sonarSensor;
    delete touchSensor;
    delete clock;
    _log("being terminated...");
    fclose(bt);
    ETRoboc_notifyCompletedToSimulator();
    ext_tsk();
}

/* periodic task to update the behavior tree */
void update_task(intptr_t unused) {
    BrainTree::Node::Status status;
    ER ercd;

    colorSensor->sense();
    plotter->plot();
    switch (state) {
    case ST_calibrating:
        if (tr_calibration != nullptr) {
            status = tr_calibration->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
                switch (JUMP) { /* JUMP = 1 or 2 is for testing only */
                    case 1:
                        state = ST_slalom;
                        _log("State changed: ST_calibration to ST_slalom");
                        break;
                    case 2:
                        state = ST_garage;
                        _log("State changed: ST_calibration to ST_garage");
                        break;
                    default:
                        state = ST_running;
                        _log("State changed: ST_calibration to ST_running");
                        break;
                }
                break;
            case BrainTree::Node::Status::Failure:
                state = ST_ending;
                _log("State changed: ST_calibration to ST_ending");
                break;
            default:
                break;
            }
        }
        break;
    case ST_running:
        if (tr_run != nullptr) {
            status = tr_run->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
                state = ST_slalom;
                _log("State changed: ST_running to ST_slalom");
                break;
            case BrainTree::Node::Status::Failure:
                state = ST_ending;
                _log("State changed: ST_running to ST_ending");
                break;
            default:
                break;
            }
        }
        break;
    case ST_slalom:
        if (tr_slalom != nullptr) {
            status = tr_slalom->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
                state = ST_garage;
                _log("State changed: ST_slalom to ST_garage");
                break;
            case BrainTree::Node::Status::Failure:
                state = ST_ending;
                _log("State changed: ST_slalom to ST_ending");
                break;
            default:
                break;
            }
        }
        break;
    case ST_garage:
        if (tr_garage != nullptr) {
            status = tr_garage->update();
            switch (status) {
            case BrainTree::Node::Status::Success:
            case BrainTree::Node::Status::Failure:
                state = ST_ending;
                _log("State changed: ST_garage to ST_ending");
                break;
            default:
                break;
            }
        }
        break;
    case ST_ending:
        _log("waking up main...");
        /* wake up the main task */
        ercd = wup_tsk(MAIN_TASK);
        assert(ercd == E_OK);
        if (ercd != E_OK) {
            syslog(LOG_NOTICE, "wup_tsk() returned %d", ercd);
        }
        state = ST_end;
        _log("State changed: ST_ending to ST_end");
        break;    
    case ST_initial:
    case ST_end:
    default:
        break;
    }
    rightMotor->drive();
    leftMotor->drive();

    logger->outputLog(LOG_INTERVAL);
}