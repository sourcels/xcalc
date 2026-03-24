#include "app_logic.h"

volatile MathCommand currentMathCmd = CMD_IDLE;
volatile bool mathBusy = false;
volatile bool showBusyOverlay = false;
volatile bool f1_error = false;
volatile bool f2_error = false;
volatile bool needsRecalc = true;
volatile int mathProgress = 0;
volatile bool needRedraw = true;

MathExpression expr1, expr2;
std::vector<Point> f1_roots, f2_roots, f1_f2_intersects;
Point f1_y_int, f2_y_int;
bool f1_has_y_int = false;
bool f2_has_y_int = false;

volatile bool f1_equals_f2 = false;

String probeInput = "";
ProbeResult resF1, resF2;
bool probeFormatError = false;

GradKoeffResult gradKoeffF1;
GradKoeffResult gradKoeffF2;

SymmetrieResult symF1;
SymmetrieResult symF2;

ExtremResult   extremF1;
ExtremResult   extremF2;

WendeResult    wendeF1;
WendeResult    wendeF2;

KruemmungsResult kruemmungF1;
KruemmungsResult kruemmungF2;

MonotonieResult  monotF1;
MonotonieResult  monotF2;

InfResult      infF1;
InfResult      infF2;

bool extremF1_valid = false;
bool extremF2_valid = false;
bool wendeF1_valid  = false;
bool wendeF2_valid  = false;

void calculatePunktprobe() {
    mathBusy = true;
    mathProgress = 0;
    needRedraw = true;

    probeFormatError = false;

    int sepPos = probeInput.indexOf(',');
    if (sepPos == -1) sepPos = probeInput.indexOf(' ');

    if (sepPos != -1) {
        double px = probeInput.substring(0, sepPos).toDouble();
        double py = probeInput.substring(sepPos + 1).toDouble();

        resF1 = checkPoint(expr1, px, py);
        mathProgress = 50;
        resF2 = checkPoint(expr2, px, py);
        mathProgress = 100;
    } else {
        probeFormatError = true;
    }

    mathBusy = false;
    needRedraw = true;
}

void calculateGradKoeff() {
    mathBusy = true;
    mathProgress = 0;
    needRedraw = true;

    if (expr1.valid && expr1.rawStr.length() > 0) {
        gradKoeffF1 = expandToPolynomial(expr1.rawStr);
    } else {
        gradKoeffF1 = GradKoeffResult();
    }
    mathProgress = 50;
    needRedraw = true;

    if (expr2.valid && expr2.rawStr.length() > 0) {
        gradKoeffF2 = expandToPolynomial(expr2.rawStr);
    } else {
        gradKoeffF2 = GradKoeffResult();
    }
    mathProgress = 100;

    mathBusy = false;
    needRedraw = true;
}

void calculateIntersections() {
    if (!needsRecalc) {
        mathProgress = 100;
        return;
    }

    mathBusy = true;
    mathProgress = 0;
    f1_roots.clear(); f2_roots.clear(); f1_f2_intersects.clear();
    f1_equals_f2 = false;

    if (expr1.valid && expr2.valid) {
        GradKoeffResult p1 = expandToPolynomial(expr1.rawStr);
        GradKoeffResult p2 = expandToPolynomial(expr2.rawStr);
        if (p1.success && p2.success) {
            bool same = true;
            for (auto& kv : p1.coeffs) if (fabs(p2.coeffs[kv.first] - kv.second) > 1e-4) { same = false; break; }
            if (same) {
                for (auto& kv : p2.coeffs) if (fabs(p1.coeffs[kv.first] - kv.second) > 1e-4) { same = false; break; }
            }
            f1_equals_f2 = same;
        } else {
            bool same = true;
            for (double tx : {-3.1, 0.0, 1.2, 2.7, 5.5}) {
                if (abs(expr1.evaluate(tx) - expr2.evaluate(tx)) > 1e-4) { same = false; break; }
            }
            f1_equals_f2 = same;
        }
    }

    findRootsUniversal(expr1, f1_roots, 0, 30, mathProgress, needRedraw);
    f1_y_int = {0, expr1.evaluate(0)};
    f1_has_y_int = !isnan(f1_y_int.y) && !isinf(f1_y_int.y);

    findRootsUniversal(expr2, f2_roots, 30, 60, mathProgress, needRedraw);
    f2_y_int = {0, expr2.evaluate(0)};
    f2_has_y_int = !isnan(f2_y_int.y) && !isinf(f2_y_int.y);

    if (expr1.valid && expr2.valid) {
        double step = 0.05;
        for (double x = -100; x <= 100; x += step) {
            mathProgress = 60 + (int)((x + 100) * 40 / 200);
            if ((int)(x * 10) % 50 == 0) needRedraw = true;

            double h1 = expr1.evaluate(x) - expr2.evaluate(x);
            double h2 = expr1.evaluate(x + step) - expr2.evaluate(x + step);
            
            if (isnan(h1) || isnan(h2) || isinf(h1) || isinf(h2)) continue;

            if (h1 * h2 <= 0) {
                double low = x, high = x + step;
                for(int k = 0; k < 40; k++) {
                    double mid = (low + high) / 2.0;
                    double hMid = expr1.evaluate(mid) - expr2.evaluate(mid);
                    if (h1 * hMid <= 0) high = mid; else low = mid;
                }
                double resX = (low + high) / 2.0;
                double finalH = expr1.evaluate(resX) - expr2.evaluate(resX);

                if (abs(finalH) < 0.0001) {
                    bool isDup = false;
                    for (auto& p : f1_f2_intersects) {
                        if (abs(p.x - resX) < 0.0001) { isDup = true; break; }
                    }
                    if (!isDup) {
                        f1_f2_intersects.push_back({resX, expr1.evaluate(resX)});
                    }
                }
            }
        }
    }
    needsRecalc = false;
    mathProgress = 100;
    mathBusy = false;
}

void calculateSymmetrie() {
    mathBusy = true;
    mathProgress = 0;
    needRedraw = true;

    symF1 = checkSymmetrie(expr1);
    mathProgress = 50;
    needRedraw = true;

    symF2 = checkSymmetrie(expr2);
    mathProgress = 100;

    mathBusy = false;
    needRedraw = true;
}

void calculateExtrem() {
    mathBusy = true;
    mathProgress = 0;
    needRedraw = true;

    if (expr1.valid) {
        extremF1 = findExtremPunkte(expr1, mathProgress, needRedraw, 0, 50);
        extremF1_valid = true;
    } else {
        extremF1 = ExtremResult();
        extremF1_valid = false;
    }
    mathProgress = 50;
    needRedraw = true;

    if (expr2.valid) {
        extremF2 = findExtremPunkte(expr2, mathProgress, needRedraw, 50, 100);
        extremF2_valid = true;
    } else {
        extremF2 = ExtremResult();
        extremF2_valid = false;
    }
    mathProgress = 100;

    mathBusy = false;
    needRedraw = true;
}

void calculateWendepunkte() {
    mathBusy = true;
    mathProgress = 0;
    needRedraw = true;

    if (expr1.valid) {
        wendeF1 = findWendePunkte(expr1, mathProgress, needRedraw, 0, 50);
        wendeF1_valid = true;
    } else {
        wendeF1 = WendeResult();
        wendeF1_valid = false;
    }
    mathProgress = 50;
    needRedraw = true;

    if (expr2.valid) {
        wendeF2 = findWendePunkte(expr2, mathProgress, needRedraw, 50, 100);
        wendeF2_valid = true;
    } else {
        wendeF2 = WendeResult();
        wendeF2_valid = false;
    }
    mathProgress = 100;

    mathBusy = false;
    needRedraw = true;
}

void calculateKruemmung() {
    mathBusy = true;
    mathProgress = 0;
    needRedraw = true;

    if (expr1.valid && !wendeF1_valid) {
        wendeF1 = findWendePunkte(expr1, mathProgress, needRedraw, 0, 40);
        wendeF1_valid = true;
    }
    mathProgress = 40;
    needRedraw = true;

    if (expr2.valid && !wendeF2_valid) {
        wendeF2 = findWendePunkte(expr2, mathProgress, needRedraw, 40, 80);
        wendeF2_valid = true;
    }
    mathProgress = 80;
    needRedraw = true;

    kruemmungF1 = buildKruemmung(wendeF1, expr1);
    mathProgress = 90;
    kruemmungF2 = buildKruemmung(wendeF2, expr2);
    mathProgress = 100;

    mathBusy = false;
    needRedraw = true;
}

void calculateMonotonie() {
    mathBusy = true;
    mathProgress = 0;
    needRedraw = true;

    if (expr1.valid && !extremF1_valid) {
        extremF1 = findExtremPunkte(expr1, mathProgress, needRedraw, 0, 40);
        extremF1_valid = true;
    }
    mathProgress = 40;
    needRedraw = true;

    if (expr2.valid && !extremF2_valid) {
        extremF2 = findExtremPunkte(expr2, mathProgress, needRedraw, 40, 80);
        extremF2_valid = true;
    }
    mathProgress = 80;
    needRedraw = true;

    monotF1 = buildMonotonie(extremF1, expr1);
    mathProgress = 90;
    monotF2 = buildMonotonie(extremF2, expr2);
    mathProgress = 100;

    mathBusy = false;
    needRedraw = true;
}

void calculateUnendlich() {
    mathBusy = true;
    mathProgress = 0;
    needRedraw = true;

    infF1 = checkVerhaltenUnendlich(expr1);
    mathProgress = 50;
    needRedraw = true;

    infF2 = checkVerhaltenUnendlich(expr2);
    mathProgress = 100;

    mathBusy = false;
    needRedraw = true;
}

void Task_Logic(void *pvParameters) {
    while (true) {
        if (currentMathCmd != CMD_IDLE) {
            mathBusy = true;
            showBusyOverlay = true;
            needRedraw = true;

            vTaskDelay(80 / portTICK_PERIOD_MS);

            if (currentMathCmd == CMD_VALIDATE_F1) {
                expr1.valid = (expr1.rawStr.length() > 0);
                f1_error = false;
                extremF1_valid = false;
                wendeF1_valid  = false;
            } else if (currentMathCmd == CMD_VALIDATE_F2) {
                expr2.valid = (expr2.rawStr.length() > 0);
                f2_error = false;
                extremF2_valid = false;
                wendeF2_valid  = false;
            } else if (currentMathCmd == CMD_CALC_INTERSECTS) {
                calculateIntersections();
            } else if (currentMathCmd == CMD_CALC_PUNKTPROBE) {
                calculatePunktprobe();
            } else if (currentMathCmd == CMD_CALC_GRADKOEFF) {
                calculateGradKoeff();
            } else if (currentMathCmd == CMD_CALC_SYMMETRIE) {
                calculateSymmetrie();
            } else if (currentMathCmd == CMD_CALC_EXTREM) {
                calculateExtrem();
            } else if (currentMathCmd == CMD_CALC_WENDEPUNKTE) {
                calculateWendepunkte();
            } else if (currentMathCmd == CMD_CALC_KRUEMMUNG) {
                calculateKruemmung();
            } else if (currentMathCmd == CMD_CALC_MONOTONIE) {
                calculateMonotonie();
            } else if (currentMathCmd == CMD_CALC_UNENDLICH) {
                calculateUnendlich();
            }

            currentMathCmd = CMD_IDLE;
            mathBusy = false;
            showBusyOverlay = false;
            needRedraw = true;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}