#include "logic.h"
#include "ui.h"
#include <stack>
#include <math.h>

volatile MathCommand currentMathCmd = CMD_IDLE;
volatile bool mathBusy = false;
volatile bool f1_error = false;
volatile bool f2_error = false;
volatile bool needsRecalc = true;
volatile int mathProgress = 0;

MathExpression expr1, expr2;
std::vector<Point> f1_roots, f2_roots, f1_f2_intersects;
Point f1_y_int, f2_y_int;
bool f1_has_y_int = false;
bool f2_has_y_int = false;

int getPrecedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '^') return 3;
    return 0;
}

double applyOp(double a, double b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return (b != 0) ? a / b : NAN;
        case '^': return pow(a, b);
    }
    return 0;
}

double MathExpression::evaluate(double x) const {
    if (rawStr.length() == 0) return NAN;

    String s = rawStr;
    s.replace(" ", "");
    s.toLowerCase();
    
    std::stack<double> values;
    std::stack<char> ops;

    for (int i = 0; i < s.length(); i++) {
        if (s[i] == ' ') continue;

        if (s[i] == 'x') {
            values.push(x);
        } else if (isdigit(s[i]) || (s[i] == '.')) {
            String val = "";
            while (i < s.length() && (isdigit(s[i]) || s[i] == '.')) {
                val += s[i++];
            }
            values.push(val.toDouble());
            i--;
        } else if (s[i] == '(') {
            ops.push('(');
        } else if (s[i] == ')') {
            while (!ops.empty() && ops.top() != '(') {
                double val2 = values.top(); values.pop();
                double val1 = values.top(); values.pop();
                char op = ops.top(); ops.pop();
                values.push(applyOp(val1, val2, op));
            }
            if (!ops.empty()) ops.pop();
        } else {
            if (s[i] == '-' && (i == 0 || s[i-1] == '(' || getPrecedence(s[i-1]) > 0)) {
                values.push(0); 
            }
            while (!ops.empty() && getPrecedence(ops.top()) >= getPrecedence(s[i])) {
                double val2 = values.top(); values.pop();
                double val1 = values.top(); values.pop();
                char op = ops.top(); ops.pop();
                values.push(applyOp(val1, val2, op));
            }
            ops.push(s[i]);
        }
    }

    while (!ops.empty()) {
        double val2 = values.top(); values.pop();
        double val1 = values.top(); values.pop();
        char op = ops.top(); ops.pop();
        values.push(applyOp(val1, val2, op));
    }

    return values.empty() ? NAN : values.top();
}

void findRootsUniversal(const MathExpression& e, std::vector<Point>& roots, int startPct, int endPct) {
    if (!e.valid) return;
    double step = 0.02;
    for (double x = -100; x <= 100; x += step) {
        int newProg = startPct + (int)((x + 100) * (endPct - startPct) / 200);
        if (newProg != mathProgress) {
            mathProgress = newProg;
            needRedraw = true;
        }

        double y1 = e.evaluate(x);
        double y2 = e.evaluate(x + step);
        
        if (isnan(y1) || isnan(y2) || isinf(y1) || isinf(y2)) continue;

        if (y1 * y2 <= 0) {
            if (abs(y1 - y2) > 50) continue; 

            double low = x, high = x + step;
            for (int k = 0; k < 30; k++) {
                double mid = (low + high) / 2.0;
                double yMid = e.evaluate(mid);
                if (y1 * yMid <= 0) high = mid;
                else low = mid;
            }
            double resX = (low + high) / 2.0;
            roots.push_back({ resX, 0.0 });
        }
    }
}

void calculateIntersections() {
    if (!needsRecalc) {
        mathProgress = 100;
        return;
    }

    mathBusy = true;
    mathProgress = 0;
    f1_roots.clear(); 
    f2_roots.clear(); 
    f1_f2_intersects.clear();

    findRootsUniversal(expr1, f1_roots, 0, 30);
    f1_y_int = {0, expr1.evaluate(0)};
    f1_has_y_int = !isnan(f1_y_int.y) && !isinf(f1_y_int.y);

    findRootsUniversal(expr2, f2_roots, 30, 60);
    f2_y_int = {0, expr2.evaluate(0)};
    f2_has_y_int = !isnan(f2_y_int.y) && !isinf(f2_y_int.y);

    if (expr1.valid && expr2.valid) {
        double step = 0.02; 
        for (double x = -100; x <= 100; x += step) {
            mathProgress = 60 + (int)((x + 100) * 40 / 200);
            if ((int)(x * 10) % 50 == 0) needRedraw = true;

            double h1 = expr1.evaluate(x) - expr2.evaluate(x);
            double h2 = expr1.evaluate(x + step) - expr2.evaluate(x + step);
            
            if (isnan(h1) || isnan(h2) || isinf(h1) || isinf(h2)) continue;

            if (h1 * h2 <= 0) {
                if (abs(h1 - h2) > 50) continue;
                double low = x, high = x + step;
                for(int k = 0; k < 30; k++) {
                    double mid = (low + high) / 2.0;
                    double hMid = expr1.evaluate(mid) - expr2.evaluate(mid);
                    if (h1 * hMid <= 0) high = mid; else low = mid;
                }
                double resX = (low + high) / 2.0;
                f1_f2_intersects.push_back({resX, expr1.evaluate(resX)});
            }
        }
    }
    needsRecalc = false;
    mathProgress = 100;
    mathBusy = false;
}

void Task_Logic(void *pvParameters) {
    while (true) {
        if (currentMathCmd != CMD_IDLE) {
            mathBusy = true;
            needRedraw = true;

            if (currentMathCmd == CMD_VALIDATE_F1) {
                expr1.valid = (expr1.rawStr.length() > 0); 
                f1_error = false;
            } else if (currentMathCmd == CMD_VALIDATE_F2) {
                expr2.valid = (expr2.rawStr.length() > 0); 
                f2_error = false;
            } else if (currentMathCmd == CMD_CALC_INTERSECTS) {
                calculateIntersections();
            }

            currentMathCmd = CMD_IDLE;
            mathBusy = false;
            needRedraw = true;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}