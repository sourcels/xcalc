#pragma once
#include <Arduino.h>
#include <vector>
#include <map>

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

typedef std::map<int, double> Polynomial;
 
struct GradKoeffResult {
    bool success = false;
    int grad = 0;
    Polynomial coeffs;
    String errorMsg = "";
};

GradKoeffResult expandToPolynomial(const String& rawStr);

enum SymmetrieType {
    SYM_UNKNOWN,
    SYM_ACHSE,
    SYM_PUNKT,
    SYM_KEINE
};

struct SymmetrieResult {
    bool defined = false;
    SymmetrieType type = SYM_UNKNOWN;
    double achseErr = 0.0;
    double punktErr = 0.0;
};

SymmetrieResult checkSymmetrie(const MathExpression& e);

ProbeResult checkPoint(const MathExpression& e, double px, double py);

int getPrecedence(char op);
double applyOp(double a, double b, char op);

void findRootsUniversal(const MathExpression& e, std::vector<Point>& roots, int startPct, int endPct, volatile int& progressRef, volatile bool& redrawRef);