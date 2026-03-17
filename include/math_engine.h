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

struct ProbeResult {
    bool defined = false;
    bool isOnGraph = false;
    double diff = 0;
};

ProbeResult checkPoint(const MathExpression& e, double px, double py);

int getPrecedence(char op);
double applyOp(double a, double b, char op);

void findRootsUniversal(const MathExpression& e, std::vector<Point>& roots, int startPct, int endPct, volatile int& progressRef, volatile bool& redrawRef);