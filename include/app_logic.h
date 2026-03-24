#pragma once
#include "math_engine.h"

enum MathCommand {
    CMD_IDLE,
    CMD_VALIDATE_F1,
    CMD_VALIDATE_F2,
    CMD_CALC_INTERSECTS,
    CMD_CALC_PUNKTPROBE,
    CMD_CALC_GRADKOEFF,
    CMD_CALC_SYMMETRIE,
    CMD_CALC_EXTREM,
    CMD_CALC_WENDEPUNKTE,
    CMD_CALC_KRUEMMUNG,
    CMD_CALC_MONOTONIE,
    CMD_CALC_UNENDLICH
};

extern volatile MathCommand currentMathCmd;
extern volatile bool mathBusy;
extern volatile bool showBusyOverlay;
extern volatile bool f1_error;
extern volatile bool f2_error;
extern volatile bool needsRecalc;
extern volatile int mathProgress;
extern volatile bool needRedraw;

extern MathExpression expr1;
extern MathExpression expr2;
extern std::vector<Point> f1_roots;
extern std::vector<Point> f2_roots;
extern std::vector<Point> f1_f2_intersects;
extern Point f1_y_int;
extern Point f2_y_int;
extern bool f1_has_y_int, f2_has_y_int;

extern volatile bool f1_equals_f2;

extern String probeInput; 
extern ProbeResult resF1, resF2;
extern bool probeFormatError;

extern GradKoeffResult gradKoeffF1;
extern GradKoeffResult gradKoeffF2;

extern SymmetrieResult symF1;
extern SymmetrieResult symF2;

extern ExtremResult   extremF1;
extern ExtremResult   extremF2;

extern WendeResult    wendeF1;
extern WendeResult    wendeF2;

extern KruemmungsResult kruemmungF1;
extern KruemmungsResult kruemmungF2;

extern MonotonieResult  monotF1;
extern MonotonieResult  monotF2;

extern InfResult      infF1;
extern InfResult      infF2;

extern bool extremF1_valid;
extern bool extremF2_valid;
extern bool wendeF1_valid;
extern bool wendeF2_valid;

void calculatePunktprobe();
void calculateGradKoeff();
void calculateSymmetrie();
void calculateExtrem();
void calculateWendepunkte();
void calculateKruemmung();
void calculateMonotonie();
void calculateUnendlich();

void Task_Logic(void *pvParameters);