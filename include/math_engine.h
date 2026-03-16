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

int getPrecedence(char op);
double applyOp(double a, double b, char op);

void findRootsUniversal(const MathExpression& e, std::vector<Point>& roots, int startPct, int endPct, volatile int& progressRef, volatile bool& redrawRef);