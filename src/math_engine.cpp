#include "math_engine.h"
#include <stack>
#include <math.h>

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

    String processed = "";
    for (int i = 0; i < s.length(); i++) {
        processed += s[i];
        if (i < s.length() - 1) {
            char c1 = s[i];
            char c2 = s[i+1];
            if ((isdigit(c1) && (c2 == 'x' || c2 == '(')) ||
                (c1 == 'x' && (isdigit(c2) || c2 == '(' || c2 == 'x')) ||
                (c1 == ')' && (isdigit(c2) || c2 == 'x' || c2 == '('))) {
                processed += '*';
            }
        }
    }
    s = processed;
    
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

void findRootsUniversal(const MathExpression& e, std::vector<Point>& roots, int startPct, int endPct, volatile int& progressRef, volatile bool& redrawRef) {
    if (!e.valid) return;
    double step = 0.05;
    
    for (double x = -100; x <= 100; x += step) {
        int newProg = startPct + (int)((x + 100) * (endPct - startPct) / 200);
        if (newProg != progressRef) {
            progressRef = newProg;
            redrawRef = true;
        }

        double y1 = e.evaluate(x);
        double y2 = e.evaluate(x + step);
        
        if (isnan(y1) || isnan(y2) || isinf(y1) || isinf(y2)) continue;

        if (y1 * y2 <= 0) {
            double low = x, high = x + step;
            for (int k = 0; k < 40; k++) {
                double mid = (low + high) / 2.0;
                double yMid = e.evaluate(mid);
                if (y1 * yMid <= 0) high = mid;
                else low = mid;
            }

            double resX = (low + high) / 2.0;
            double resY = e.evaluate(resX);

            if (abs(resY) < 0.0001) {
                bool isDup = false;
                for (auto& r : roots) {
                    if (abs(r.x - resX) < 0.0001) { isDup = true; break; }
                }
                if (!isDup) {
                    roots.push_back({ resX, 0.0 });
                }
            }
        }
    }
}