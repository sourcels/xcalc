#pragma once
#include "math_engine.h"

enum MathCommand {
    CMD_IDLE,
    CMD_VALIDATE_F1,
    CMD_VALIDATE_F2,
    CMD_CALC_INTERSECTS,
    CMD_CALC_PUNKTPROBE,
    CMD_CALC_GRADKOEFF,
    CMD_CALC_SYMMETRIE
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

extern String probeInput; 
extern ProbeResult resF1, resF2;
extern bool probeFormatError;

extern GradKoeffResult gradKoeffF1;
extern GradKoeffResult gradKoeffF2;

extern SymmetrieResult symF1;
extern SymmetrieResult symF2;

void calculatePunktprobe();
void calculateGradKoeff();
void calculateSymmetrie();

void Task_Logic(void *pvParameters);