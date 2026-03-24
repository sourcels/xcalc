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

String probeInput = "";
ProbeResult resF1, resF2;
bool probeFormatError = false;

GradKoeffResult gradKoeffF1;
GradKoeffResult gradKoeffF2;

SymmetrieResult symF1;
SymmetrieResult symF2;

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
            } else if (currentMathCmd == CMD_VALIDATE_F2) {
                expr2.valid = (expr2.rawStr.length() > 0); 
                f2_error = false;
            } else if (currentMathCmd == CMD_CALC_INTERSECTS) {
                calculateIntersections();
            } else if (currentMathCmd == CMD_CALC_PUNKTPROBE) {
                calculatePunktprobe();
            } else if (currentMathCmd == CMD_CALC_GRADKOEFF) {
                calculateGradKoeff();
            } else if (currentMathCmd == CMD_CALC_SYMMETRIE) {
                calculateSymmetrie();
            }

            currentMathCmd = CMD_IDLE;
            mathBusy = false;
            showBusyOverlay = false;
            needRedraw = true;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}