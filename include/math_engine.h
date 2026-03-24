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

enum ExtremType { EXT_HOCH, EXT_TIEF, EXT_SATTEL };

struct ExtremPoint {
    double x, y;
    ExtremType type;
};

struct ExtremResult {
    bool defined = false;
    std::vector<ExtremPoint> points;
};

double numericalDerivative(const MathExpression& e, double x, double h = 1e-5);
double numericalDerivative2(const MathExpression& e, double x, double h = 1e-5);

ExtremResult findExtremPunkte(const MathExpression& e,
                               volatile int& progressRef,
                               volatile bool& redrawRef,
                               int startPct = 0, int endPct = 100);

struct WendePunkt {
    double x, y;
    bool concaveToConvex;
};

struct WendeResult {
    bool defined = false;
    std::vector<WendePunkt> points;
};

WendeResult findWendePunkte(const MathExpression& e,
                              volatile int& progressRef,
                              volatile bool& redrawRef,
                              int startPct = 0, int endPct = 100);

struct KruemmungsInterval {
    double xFrom, xTo;
    bool isConvex;
};

struct KruemmungsResult {
    bool defined = false;
    std::vector<KruemmungsInterval> intervals;
};

KruemmungsResult buildKruemmung(const WendeResult& wende, const MathExpression& e);


struct MonotonieInterval {
    double xFrom, xTo;
    bool isIncreasing;
};

struct MonotonieResult {
    bool defined = false;
    std::vector<MonotonieInterval> intervals;
};

MonotonieResult buildMonotonie(const ExtremResult& ext, const MathExpression& e);


enum InfBehavior {
    INF_PLUS_INF,
    INF_MINUS_INF,
    INF_ZERO,
    INF_CONST,
    INF_OSCILLATE,
};

struct InfResult {
    bool defined = false;
    InfBehavior atPlusInf;
    InfBehavior atMinusInf;
    double constValPlus  = 0.0;
    double constValMinus = 0.0;
};

InfResult checkVerhaltenUnendlich(const MathExpression& e);