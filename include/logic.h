#pragma once
#include <Arduino.h>
#include <vector>

struct Point {
    double x, y;
};

struct MathExpression {
    String rawStr;
    bool valid = false;
    double evaluate(double x) const;
};

enum MathCommand {
    CMD_IDLE,
    CMD_VALIDATE_F1,
    CMD_VALIDATE_F2,
    CMD_CALC_INTERSECTS
};

extern volatile MathCommand currentMathCmd;
extern volatile bool mathBusy;
extern volatile bool f1_error;
extern volatile bool f2_error;
extern volatile bool needsRecalc;
extern volatile int mathProgress;

extern MathExpression expr1;
extern MathExpression expr2;
extern std::vector<Point> f1_roots;
extern std::vector<Point> f2_roots;
extern std::vector<Point> f1_f2_intersects;
extern Point f1_y_int;
extern Point f2_y_int;
extern bool f1_has_y_int, f2_has_y_int;

void Task_Logic(void *pvParameters);