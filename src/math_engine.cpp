#include "math_engine.h"
#include <stack>
#include <math.h>

static Polynomial polyAdd(const Polynomial& a, const Polynomial& b) {
    Polynomial res = a;
    for (auto& kv : b) res[kv.first] += kv.second;
    return res;
}

static Polynomial polySub(const Polynomial& a, const Polynomial& b) {
    Polynomial res = a;
    for (auto& kv : b) res[kv.first] -= kv.second;
    return res;
}

static Polynomial polyMul(const Polynomial& a, const Polynomial& b) {
    Polynomial res;
    for (auto& ka : a)
        for (auto& kb : b)
            res[ka.first + kb.first] += ka.second * kb.second;
    return res;
}

static Polynomial polyDiv(const Polynomial& a, double divisor, bool& ok) {
    if (divisor == 0.0) { ok = false; return {}; }
    Polynomial res = a;
    for (auto& kv : res) kv.second /= divisor;
    return res;
}

static Polynomial polyPow(const Polynomial& base, int exp, bool& ok) {
    if (exp < 0) {
        if (base.size() == 1) {
            auto it = base.begin();
            if (it->second == 1.0) {
                return {{it->first * exp, 1.0}};
            }
        }
        ok = false; return {};
    }
    Polynomial res = {{0, 1.0}};
    for (int i = 0; i < exp; i++) res = polyMul(res, base);
    return res;
}

static Polynomial parseExpr(const String& s, int& pos, bool& ok);
static Polynomial parseTerm(const String& s, int& pos, bool& ok);
static Polynomial parseFactor(const String& s, int& pos, bool& ok);
static Polynomial parsePrimary(const String& s, int& pos, bool& ok);

static Polynomial parseExpr(const String& s, int& pos, bool& ok) {
    Polynomial result = parseTerm(s, pos, ok);
    if (!ok) return {};
    while (pos < (int)s.length() && (s[pos] == '+' || s[pos] == '-')) {
        char op = s[pos++];
        Polynomial right = parseTerm(s, pos, ok);
        if (!ok) return {};
        result = (op == '+') ? polyAdd(result, right) : polySub(result, right);
    }
    return result;
}

static Polynomial parseTerm(const String& s, int& pos, bool& ok) {
    Polynomial result = parseFactor(s, pos, ok);
    if (!ok) return {};
    while (pos < (int)s.length() && (s[pos] == '*' || s[pos] == '/')) {
        char op = s[pos++];
        Polynomial right = parseFactor(s, pos, ok);
        if (!ok) return {};
        if (op == '*') {
            result = polyMul(result, right);
        } else {
            if (right.size() != 1 || right.count(0) == 0) { ok = false; return {}; }
            result = polyDiv(result, right.at(0), ok);
        }
    }
    return result;
}

static Polynomial parseFactor(const String& s, int& pos, bool& ok) {
    Polynomial base = parsePrimary(s, pos, ok);
    if (!ok) return {};
    if (pos < (int)s.length() && s[pos] == '^') {
        pos++;
        bool neg = false;
        if (pos < (int)s.length() && s[pos] == '-') { neg = true; pos++; }
        if (pos >= (int)s.length() || !isdigit(s[pos])) { ok = false; return {}; }
        int exp = 0;
        while (pos < (int)s.length() && isdigit(s[pos])) exp = exp * 10 + (s[pos++] - '0');
        if (neg) exp = -exp;
        if (exp < -20 || exp > 20) { ok = false; return {}; }
        base = polyPow(base, exp, ok);
    }
    return base;
}

static Polynomial parsePrimary(const String& s, int& pos, bool& ok) {
    if (!ok) return {};
    while (pos < (int)s.length() && s[pos] == ' ') pos++;

    if (pos < (int)s.length() && s[pos] == '-') {
        pos++;
        Polynomial inner = parsePrimary(s, pos, ok);
        if (!ok) return {};
        Polynomial neg;
        for (auto& kv : inner) neg[kv.first] = -kv.second;
        return neg;
    }

    if (pos < (int)s.length() && s[pos] == '+') {
        pos++;
        return parsePrimary(s, pos, ok);
    }

    if (pos < (int)s.length() && s[pos] == '(') {
        pos++;
        Polynomial inner = parseExpr(s, pos, ok);
        if (!ok) return {};
        if (pos >= (int)s.length() || s[pos] != ')') { ok = false; return {}; }
        pos++;
        return inner;
    }

    if (pos < (int)s.length() && s[pos] == 'x') {
        pos++;
        return {{1, 1.0}};
    }

    if (pos < (int)s.length() && (isdigit(s[pos]) || s[pos] == '.')) {
        String numStr = "";
        while (pos < (int)s.length() && (isdigit(s[pos]) || s[pos] == '.')) numStr += s[pos++];
        return {{0, numStr.toDouble()}};
    }

    ok = false;
    return {};
}

GradKoeffResult expandToPolynomial(const String& rawStr) {
    GradKoeffResult res;
    if (rawStr.length() == 0) {
        res.errorMsg = "Kein Ausdruck!";
        return res;
    }

    String s = rawStr;
    s.replace(" ", "");
    s.toLowerCase();
 
    String processed = "";
    for (int i = 0; i < (int)s.length(); i++) {
        processed += s[i];
        if (i < (int)s.length() - 1) {
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

    bool ok = true;
    int pos = 0;
    Polynomial poly = parseExpr(s, pos, ok);
 
    if (!ok || pos < (int)s.length()) {
        res.errorMsg = "Parse-Fehler!";
        return res;
    }

    Polynomial cleaned;
    for (auto& kv : poly) {
        if (fabs(kv.second) > 1e-10) cleaned[kv.first] = kv.second;
    }

    int maxDeg = 0;
    for (auto& kv : cleaned) if (kv.first > maxDeg) maxDeg = kv.first;
 
    res.success = true;
    res.grad = maxDeg;
    res.coeffs = cleaned;
    return res;
}

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

SymmetrieResult checkSymmetrie(const MathExpression& e) {
    SymmetrieResult res;
    if (!e.valid || e.rawStr.length() == 0) return res;
    res.defined = true;

    const double testXs[] = {0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0,
                              0.25, 0.75, 1.25, 1.75, 6.0, 7.0, 8.0,
                              0.1, 0.3, 9.0, 10.0, 3.5};
    const int N = 20;

    double achseSum = 0, punktSum = 0;
    int validCount = 0;

    for (int i = 0; i < N; i++) {
        double x = testXs[i];
        double fx  = e.evaluate(x);
        double f_x = e.evaluate(-x);
        if (isnan(fx) || isnan(f_x) || isinf(fx) || isinf(f_x)) continue;

        double scale = std::max(1.0, fabs(fx));
        achseSum += fabs(f_x - fx) / scale;
        punktSum += fabs(f_x + fx) / scale;
        validCount++;
    }

    if (validCount == 0) { res.type = SYM_UNKNOWN; return res; }

    res.achseErr = achseSum / validCount;
    res.punktErr = punktSum / validCount;

    double eps = 1e-4;
    if (res.achseErr < eps)      res.type = SYM_ACHSE;
    else if (res.punktErr < eps) res.type = SYM_PUNKT;
    else                         res.type = SYM_KEINE;
    return res;
}

ProbeResult checkPoint(const MathExpression& e, double px, double py) {
    ProbeResult res;
    if (!e.valid || e.rawStr.length() == 0) {
        res.defined = false;
        return res;
    }
    res.defined = true;
    double val = e.evaluate(px);
    res.diff = abs(val - py);
    res.isOnGraph = (res.diff < 0.01);
    return res;
}