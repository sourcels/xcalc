#include "ui.h"
#include "app_logic.h"

UIState currentState = STATE_MENU;
String func1 = "";
String func2 = "";
String funcTemp = "";

String punktProbeResult = "";
uint16_t punktProbeColor = TFT_WHITE;

const char* menuItemsAll[] = {
    "1. f(x) definieren",
    "2. g(x) definieren",
    "3. Schnittpunkte finden",
    "4. Punktprobe (x,y)",
    "5. Grad & Koeffizienten",
    "6. Symmetrie",
    "7. Extrempunkte",
    "8. Wendepunkte",
    "9. Kruemmungsverhalten",
    "10. Monotonie",
    "11. Verhalten Unendlichen",
    "12. Wertetabelle",
    "13. Graph zeichnen"
};

int numMenuItems = 2;
int selectedMenuItem = 0;
int resultScrollY = 0;
int maxScrollHeight = 0;

static int gradScrollY = 0;
static int gradMaxScrollHeight = 0;

static int genScrollY = 0;
static int genMaxScrollHeight = 0;

String tabStartStr = "", tabEndStr = "", tabStepStr = "";
double tStart = 0, tEnd = 0, tStep = 1;
static int tabScrollY = 0;

double panX = 0.0, panY = 0.0; 
double scaleP = 15.0;

static const char* funcPickerItems[] = {
    "sin()", "cos()", "tan()", "ln()",
    "arcsin()", "arccos()", "arctan()", "sqrt()",
    "abs()", "log()", "sinh()", "cosh()",
    "tanh()", "exp()", "1/", "x^2",
    "x^3", "x^(-1)", "pi", "e"
};
static const int FUNC_PICKER_COUNT = 20;
static const int FUNC_PICKER_COLS  = 4;
static int funcPickerCursorIdx = 0;
static int funcPickerScrollRow = 0;
static UIState funcPickerReturnState = STATE_INPUT_F1;

static M5Canvas sprite(&M5Cardputer.Display);

static const uint16_t COLOR_BG     = TFT_BLACK;
static const uint16_t COLOR_BORDER = 0x4208;
static const uint16_t COLOR_TEXT   = TFT_WHITE;
static const uint16_t COLOR_GREEN  = TFT_GREEN;
static const uint16_t COLOR_YELLOW = TFT_YELLOW;
static const uint16_t COLOR_RED    = TFT_RED;
static const uint16_t COLOR_GRAY   = 0x8410;
static const uint16_t COLOR_CYAN   = TFT_CYAN;
static const uint16_t COLOR_ORANGE = 0xFBE0;

static String fmtX(double v) {
    String s = String(v, 4);
    while (s.endsWith("0") && s.indexOf('.') != -1) s.remove(s.length()-1);
    if (s.endsWith(".")) s.remove(s.length()-1);
    return s;
}

static void drawScrollHeader(const char* title) {
    sprite.fillRect(0, 0, sprite.width(), 20, COLOR_BG);
    sprite.drawRect(5, 2, sprite.width() - 10, 16, COLOR_BORDER);
    sprite.fillRect(6, 3, sprite.width() - 12, 14, 0x2104);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(COLOR_GREEN);
    sprite.drawString(title, sprite.width() / 2, 10);
    sprite.setTextDatum(TL_DATUM);
}

static void drawScrollBar(int scrollY, int maxH, int yTop, int height) {
    if (maxH <= 0) return;
    int thumbH = std::max(8, height * height / (height + maxH));
    int thumbY = yTop + (height - thumbH) * scrollY / maxH;
    sprite.fillRect(sprite.width() - 4, yTop, 3, height, 0x2104);
    sprite.fillRect(sprite.width() - 4, thumbY, 3, thumbH, COLOR_GRAY);
}

static void drawScrollFooter() {
    sprite.fillRect(0, sprite.height() - 18, sprite.width(), 18, COLOR_BG);
    sprite.setTextColor(COLOR_GRAY);
    sprite.setTextDatum(TL_DATUM);
    sprite.drawString("W/S: Scroll | ESC: Zurueck", 10, sprite.height() - 15);
}

void drawMenu() {
    sprite.setTextDatum(TL_DATUM);
    sprite.setTextSize(1);
    sprite.setTextColor(COLOR_GREEN, COLOR_BG);
    sprite.drawString("=== xcalc (by shimaper) ===", 10, 5);

    numMenuItems = (expr1.valid || expr2.valid) ? 13 : 2;

    int y = 25;
    int startIdx = (selectedMenuItem > 3) ? selectedMenuItem - 3 : 0;
    int endIdx = std::min(startIdx + 5, numMenuItems);

    for (int i = startIdx; i < endIdx; i++) {
        if (i == selectedMenuItem) {
            sprite.setTextColor(TFT_BLACK, TFT_WHITE);
            sprite.fillRect(5, y - 2, sprite.width() - 10, 15, TFT_WHITE);
        } else {
            sprite.setTextColor(TFT_WHITE, COLOR_BG);
        }
        
        String itemText = menuItemsAll[i];
        if (i == 0 && expr1.valid) itemText += " [OK]";
        if (i == 1 && expr2.valid) itemText += " [OK]";

        sprite.drawString(itemText, 10, y);
        y += 18;
    }

    if (f1_error || f2_error) {
        sprite.setTextColor(COLOR_RED, COLOR_BG);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("Syntax-Fehler!", sprite.width()/2, 110);
    }
    
    sprite.setTextDatum(TL_DATUM);
    sprite.setTextColor(COLOR_GRAY, COLOR_BG);
    sprite.drawString("W/S: Hoch/Runter | ENTER: Ausw.", 10, sprite.height() - 15);
}

void drawInput(const char* title, const char* prefix, String currentInput, bool showFuncHint = false) {
    sprite.setTextColor(COLOR_GREEN, COLOR_BG);
    sprite.drawString(title, 10, 10);
    sprite.setTextColor(TFT_WHITE, COLOR_BG);
    sprite.drawString("Eingabe:", 10, 25);
    sprite.drawRect(8, 40, sprite.width() - 16, 45, COLOR_BORDER);

    sprite.setCursor(12, 45);
    sprite.print(prefix + currentInput + "_");

    sprite.setTextColor(COLOR_GRAY, COLOR_BG);
    if (showFuncHint) {
        sprite.drawString("ENTER:OK  ESC:Abbruch  `:Funktionen", 10, sprite.height() - 15);
    } else {
        sprite.drawString("ENTER: OK | ESC: Abbruch", 10, sprite.height() - 15);
    }
}

void drawResultsAchsen() {
    sprite.setTextDatum(TL_DATUM);
    sprite.fillSprite(COLOR_BG);
    int yStart = 25;
    int curY = yStart - resultScrollY;
    char buf[64];

    auto drawSection = [&](const char* title, const std::vector<Point>& roots, Point yInt, bool hasY, bool isIntersect = false) {
        sprite.setTextColor(COLOR_YELLOW);
        sprite.drawString(title, 10, curY); curY += 15;

        sprite.setTextColor(TFT_WHITE);

        if (!isIntersect) {
            if (hasY) {
                sprintf(buf, "Sy(0.00 | %.2f)", yInt.y);
                sprite.drawString(buf, 20, curY); curY += 12;
            } else {
                sprite.drawString("Kein y-Achsenabschnitt!", 20, curY); curY += 12;
            }
        }

        if (roots.empty()) {
            if (isIntersect) {
                sprite.drawString("Keine Schnittpunkte!", 20, curY); curY += 12;
            } else {
                sprite.drawString("Keine x-Achsenabschnitte!", 20, curY); curY += 12;
            }
        } else {
            for (size_t i = 0; i < roots.size(); i++) {
                String prefix = isIntersect ? "S" : "Sx";
                if (roots.size() > 1) prefix += String(i + 1);
                
                String xStr = String(roots[i].x, 4);
                String yStr = String(isIntersect ? roots[i].y : 0.0, 4);

                while(xStr.endsWith("0") && xStr.indexOf('.') != -1) xStr.remove(xStr.length()-1);
                if(xStr.endsWith(".")) xStr.remove(xStr.length()-1);
                while(yStr.endsWith("0") && yStr.indexOf('.') != -1) yStr.remove(yStr.length()-1);
                if(yStr.endsWith(".")) yStr.remove(yStr.length()-1);

                String resLine = prefix + "(" + xStr + " | " + yStr + ")";
                sprite.drawString(resLine, 20, curY); 
                curY += 12;
            }
        }
        curY += 10;
    };

    bool bothValid = expr1.valid && expr2.valid;
    bool sameFunc  = bothValid && f1_equals_f2;

    if (expr1.valid && !sameFunc) drawSection("f(x) Achsen:", f1_roots, f1_y_int, f1_has_y_int);
    if (expr2.valid && !sameFunc) drawSection("g(x) Achsen:", f2_roots, f2_y_int, f2_has_y_int);
    if (sameFunc) drawSection("f(x) = g(x) Achsen:", f1_roots, f1_y_int, f1_has_y_int);

    if (bothValid && !sameFunc) {
        if (f1_equals_f2) {
            sprite.setTextColor(COLOR_CYAN);
            sprite.drawString("f(x) & g(x) Schnitt:", 10, curY); curY += 15;
            sprite.setTextColor(TFT_WHITE);
            sprite.drawString("f(x) und g(x) sind identisch!", 20, curY); curY += 12;
            sprite.drawString("Unendlich viele Schnittpunkte.", 20, curY); curY += 10;
        } else {
            drawSection("f(x) & g(x) Schnitt:", f1_f2_intersects, {0,0}, false, true);
        }
    }

    int totalContentH = (curY + resultScrollY) - yStart;
    maxScrollHeight = std::max(0, totalContentH - (sprite.height() - 40));

    sprite.fillRect(0, 0, sprite.width(), 20, COLOR_BG);
    sprite.drawRect(5, 2, sprite.width() - 10, 16, COLOR_BORDER);
    sprite.fillRect(6, 3, sprite.width() - 12, 14, 0x2104);
    
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(COLOR_GREEN);
    sprite.drawString("--- Schnittpunkte ---", sprite.width() / 2, 10);

    sprite.fillRect(0, sprite.height() - 18, sprite.width(), 18, COLOR_BG);
    sprite.setTextColor(COLOR_GRAY);
    sprite.setTextDatum(TL_DATUM);
    sprite.drawString(";/. : Scroll | ESC: Zurueck", 10, sprite.height() - 15);
}

void drawPunktProbe() {
    sprite.fillSprite(COLOR_BG);
    sprite.drawRect(5, 5, 230, 125, COLOR_BORDER);
    
    sprite.setTextColor(COLOR_YELLOW);
    sprite.setTextDatum(TC_DATUM);
    sprite.drawString("Punktprobe P(x,y)", 120, 12);

    sprite.setTextDatum(TL_DATUM);
    sprite.setTextColor(COLOR_TEXT);
    sprite.drawString("Eingabe P = ", 15, 40);
    sprite.drawString(funcTemp + "_", 90, 40);

    int startY = 70;
    if (probeFormatError) {
        sprite.setTextColor(COLOR_RED);
        sprite.drawString("Fehler: Format 'x y' nutzen!", 15, startY);
    } else {
        if (resF1.defined) {
            sprite.setTextColor(resF1.isOnGraph ? COLOR_GREEN : COLOR_RED);
            sprite.drawString("f(x): " + String(resF1.isOnGraph ? "Liegt darauf" : "Liegt NICHT darauf"), 15, startY);
        }
        if (resF2.defined) {
            sprite.setTextColor(resF2.isOnGraph ? COLOR_GREEN : COLOR_RED);
            sprite.drawString("g(x): " + String(resF2.isOnGraph ? "Liegt darauf" : "Liegt NICHT darauf"), 15, startY + 20);
        }
    }
}

static String fmtCoeff(double v) {
    String s = String(v, 4);
    while (s.endsWith("0") && s.indexOf('.') != -1) s.remove(s.length() - 1);
    if (s.endsWith(".")) s.remove(s.length() - 1);
    return s;
}

static void drawGradBlock(const char* label, const GradKoeffResult& res, const String& rawStr, int& curY) {
    sprite.setTextColor(COLOR_YELLOW);
    sprite.drawString(label, 10, curY); curY += 14;

    if (!res.success) {
        if (rawStr.length() == 0) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("(nicht definiert)", 20, curY); curY += 12;
        } else {
            sprite.setTextColor(COLOR_RED);
            sprite.drawString(res.errorMsg.length() ? res.errorMsg : "Fehler!", 20, curY); curY += 12;
        }
        curY += 6;
        return;
    }

    String expanded = "= ";
    bool first = true;
    for (auto it = res.coeffs.rbegin(); it != res.coeffs.rend(); ++it) {
        int deg = it->first;
        double coef = it->second;
        if (coef == 0.0) continue;

        if (!first) {
            expanded += (coef < 0) ? " - " : " + ";
            coef = fabs(coef);
        } else {
            if (coef < 0) { expanded += "-"; coef = fabs(coef); }
        }
        first = false;

        if (deg == 0) {
            expanded += fmtCoeff(coef);
        } else if (deg == 1) {
            if (coef != 1.0) expanded += fmtCoeff(coef);
            expanded += "x";
        } else {
            if (coef != 1.0) expanded += fmtCoeff(coef);
            expanded += "x^" + String(deg);
        }
    }
    if (first) expanded += "0";

    sprite.setTextColor(COLOR_CYAN);
    sprite.drawString(expanded, 20, curY); curY += 14;

    sprite.setTextColor(TFT_WHITE);
    if (res.grad >= 0) {
        sprite.drawString("n = " + String(res.grad), 20, curY); curY += 12;
    }

    bool hasNegDeg = false;
    for (auto& kv : res.coeffs) if (kv.first < 0) { hasNegDeg = true; break; }

    if (hasNegDeg) {
        for (auto it = res.coeffs.rbegin(); it != res.coeffs.rend(); ++it) {
            int d = it->first;
            double coef = it->second;
            String degStr = (d == 0) ? "0" : (d == 1 ? "1" : String(d));
            String line = "a" + degStr + " = " + fmtCoeff(coef);
            sprite.setTextColor(TFT_WHITE);
            sprite.drawString(line, 20, curY); curY += 12;
        }
    } else {
        for (int d = res.grad; d >= 0; d--) {
            double coef = 0.0;
            auto it = res.coeffs.find(d);
            if (it != res.coeffs.end()) coef = it->second;
            String line = "a" + String(d) + " = " + fmtCoeff(coef);
            sprite.setTextColor(coef != 0.0 ? TFT_WHITE : COLOR_GRAY);
            sprite.drawString(line, 20, curY); curY += 12;
        }
    }
    curY += 8;
}

void drawResultsGradKoeff() {
    sprite.fillSprite(COLOR_BG);

    const int HEADER_H = 22;
    const int FOOTER_H = 18;
    const int CLIP_TOP = HEADER_H;
    const int CLIP_BOT = sprite.height() - FOOTER_H;

    drawScrollHeader("- Grad & Koeffizienten -");
    drawScrollFooter();

    if (mathBusy) { sprite.pushSprite(0, 0); return; }

    int yStart = HEADER_H;
    int curY = yStart - gradScrollY;

    sprite.setClipRect(0, CLIP_TOP, sprite.width(), CLIP_BOT - CLIP_TOP);

    bool bothValid_gk = expr1.valid && expr2.valid;
    bool sameFunc_gk  = bothValid_gk && f1_equals_f2;

    if (sameFunc_gk) {
        drawGradBlock("f(x) = g(x) Grad & Koeff:", gradKoeffF1, expr1.rawStr, curY);
    } else {
        if (expr1.valid) drawGradBlock("f(x) Grad & Koeff:", gradKoeffF1, expr1.rawStr, curY);
        if (expr2.valid) drawGradBlock("g(x) Grad & Koeff:", gradKoeffF2, expr2.rawStr, curY);
    }
    if (!expr1.valid && !expr2.valid) {
        sprite.setTextColor(COLOR_GRAY);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("Keine Funktion definiert!", sprite.width()/2, 70);
        sprite.setTextDatum(TL_DATUM);
    }

    sprite.clearClipRect();

    int totalContentH = (curY + gradScrollY) - yStart;
    gradMaxScrollHeight = std::max(0, totalContentH - (sprite.height() - 40));

    drawScrollBar(gradScrollY, gradMaxScrollHeight, CLIP_TOP, CLIP_BOT - CLIP_TOP);
    drawScrollHeader("- Grad & Koeffizienten -");
    drawScrollFooter();
}

void drawResultsSymmetrie() {
    sprite.fillSprite(COLOR_BG);

    sprite.fillRect(0, 0, sprite.width(), 20, COLOR_BG);
    sprite.drawRect(5, 2, sprite.width() - 10, 16, COLOR_BORDER);
    sprite.fillRect(6, 3, sprite.width() - 12, 14, 0x2104);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(COLOR_GREEN);
    sprite.drawString("--- Symmetrie ---", sprite.width() / 2, 10);
    sprite.setTextDatum(TL_DATUM);

    int curY = 28;

    auto drawSymBlock = [&](const char* label, const SymmetrieResult& res, bool exprValid) {
        sprite.setTextColor(COLOR_YELLOW);
        sprite.drawString(label, 10, curY); curY += 14;

        if (!res.defined) {
            sprite.setTextColor(COLOR_RED);
            sprite.drawString("Fehler!", 20, curY); curY += 12;
            curY += 6;
            return;
        }

        switch (res.type) {
            case SYM_ACHSE:
                sprite.setTextColor(COLOR_GREEN);
                sprite.drawString("Achsensymmetrie", 20, curY); curY += 13;
                sprite.setTextColor(TFT_WHITE);
                sprite.drawString("zur y-Achse", 20, curY); curY += 13;
                sprite.setTextColor(COLOR_CYAN);
                sprite.drawString("f(-x) = f(x)", 20, curY); curY += 13;
                break;
            case SYM_PUNKT:
                sprite.setTextColor(COLOR_GREEN);
                sprite.drawString("Punktsymmetrie", 20, curY); curY += 13;
                sprite.setTextColor(TFT_WHITE);
                sprite.drawString("zum Ursprung (0|0)", 20, curY); curY += 13;
                sprite.setTextColor(COLOR_CYAN);
                sprite.drawString("f(-x) = -f(x)", 20, curY); curY += 13;
                break;
            case SYM_KEINE:
                sprite.setTextColor(COLOR_RED);
                sprite.drawString("Keine Symmetrie", 20, curY); curY += 13;
                break;
            default:
                sprite.setTextColor(COLOR_GRAY);
                sprite.drawString("Unbestimmt", 20, curY); curY += 13;
                break;
        }
        curY += 8;
    };

    bool bothValid_sym = expr1.valid && expr2.valid;
    bool sameFunc_sym  = bothValid_sym && f1_equals_f2;

    if (sameFunc_sym) {
        drawSymBlock("f(x) = g(x) Symmetrie:", symF1, true);
    } else {
        if (expr1.valid) drawSymBlock("f(x) Symmetrie:", symF1, expr1.valid);
        if (expr2.valid) drawSymBlock("g(x) Symmetrie:", symF2, expr2.valid);
    }

    if (!expr1.valid && !expr2.valid) {
        sprite.setTextColor(COLOR_GRAY);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("Keine Funktion definiert!", sprite.width()/2, 70);
        sprite.setTextDatum(TL_DATUM);
    }

    sprite.fillRect(0, sprite.height() - 18, sprite.width(), 18, COLOR_BG);
    sprite.setTextColor(COLOR_GRAY);
    sprite.setTextDatum(TL_DATUM);
    sprite.drawString("ESC: Zurueck", 10, sprite.height() - 15);
}

void drawResultsExtrem() {
    sprite.fillSprite(COLOR_BG);

    const int HEADER_H  = 22;
    const int FOOTER_H  = 18;
    const int CLIP_TOP  = HEADER_H;
    const int CLIP_BOT  = sprite.height() - FOOTER_H;

    int yStart = HEADER_H;
    int curY   = yStart - genScrollY;

    drawScrollHeader("--- Extrempunkte ---");
    drawScrollFooter();

    if (mathBusy) {
        sprite.pushSprite(0, 0);
        return;
    }

    sprite.setClipRect(0, CLIP_TOP, sprite.width(), CLIP_BOT - CLIP_TOP);

    auto drawExtBlock = [&](const char* label, const ExtremResult& res, bool exprValid) {
        sprite.setTextColor(COLOR_YELLOW);
        sprite.drawString(label, 10, curY); curY += 14;

        if (!res.defined) {
            sprite.setTextColor(COLOR_RED);
            sprite.drawString("Fehler!", 20, curY); curY += 12;
            curY += 6; return;
        }

        int hIdx = 1, tIdx = 1;
        bool anyFound = false;

        for (auto& p : res.points) {
            anyFound = true;
            String lbl;
            uint16_t col;
            if (p.type == EXT_HOCH) {
                lbl = "H" + String(hIdx++) + "(";
                col = COLOR_RED;
            } else if (p.type == EXT_TIEF) {
                lbl = "T" + String(tIdx++) + "(";
                col = COLOR_CYAN;
            } else {
                lbl = "SP(";
                col = COLOR_ORANGE;
            }
            lbl += fmtX(p.x) + " | " + fmtX(p.y) + ")";
            sprite.setTextColor(col);
            sprite.drawString(lbl, 20, curY); curY += 13;
        }

        if (!anyFound) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("Keine Extrempunkte!", 20, curY); curY += 12;
        }
        curY += 8;
    };

    bool bothValid_ex = expr1.valid && expr2.valid;
    bool sameFunc_ex  = bothValid_ex && f1_equals_f2;

    if (sameFunc_ex) {
        drawExtBlock("f(x) = g(x) Extrempunkte:", extremF1, true);
    } else {
        if (expr1.valid) drawExtBlock("f(x) Extrempunkte:", extremF1, expr1.valid);
        if (expr2.valid) drawExtBlock("g(x) Extrempunkte:", extremF2, expr2.valid);
    }

    sprite.clearClipRect();

    int totalH = (curY + genScrollY) - yStart;
    genMaxScrollHeight = std::max(0, totalH - (sprite.height() - 40));

    drawScrollBar(genScrollY, genMaxScrollHeight, CLIP_TOP, CLIP_BOT - CLIP_TOP);
    drawScrollHeader("--- Extrempunkte ---");
    drawScrollFooter();
}

void drawResultsWendepunkte() {
    sprite.fillSprite(COLOR_BG);

    const int HEADER_H = 22;
    const int FOOTER_H = 18;
    const int CLIP_TOP = HEADER_H;
    const int CLIP_BOT = sprite.height() - FOOTER_H;

    int yStart = HEADER_H;
    int curY   = yStart - genScrollY;

    drawScrollHeader("--- Wendepunkte ---");
    drawScrollFooter();

    if (mathBusy) { sprite.pushSprite(0, 0); return; }

    sprite.setClipRect(0, CLIP_TOP, sprite.width(), CLIP_BOT - CLIP_TOP);

    auto drawWendeBlock = [&](const char* label, const WendeResult& res, bool exprValid) {
        sprite.setTextColor(COLOR_YELLOW);
        sprite.drawString(label, 10, curY); curY += 14;

        if (!res.defined) {
            sprite.setTextColor(COLOR_RED);
            sprite.drawString("Fehler!", 20, curY); curY += 12;
            curY += 6; return;
        }

        if (res.points.empty()) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("Keine Wendepunkte!", 20, curY); curY += 12;
            curY += 8; return;
        }

        for (size_t i = 0; i < res.points.size(); i++) {
            auto& wp = res.points[i];
            String lbl = "W" + String(i+1) + "(" + fmtX(wp.x) + " | " + fmtX(wp.y) + ")";
            sprite.setTextColor(COLOR_CYAN);
            sprite.drawString(lbl, 20, curY); curY += 13;

            sprite.setTextColor(TFT_WHITE);
            if (wp.concaveToConvex) {
                sprite.drawString(" Linkskurve -> Rechtskurve", 20, curY);
            } else {
                sprite.drawString(" Rechtskurve -> Linkskurve", 20, curY);
            }
            curY += 12;
        }
        curY += 8;
    };

    bool bothValid_wp = expr1.valid && expr2.valid;
    bool sameFunc_wp  = bothValid_wp && f1_equals_f2;

    if (sameFunc_wp) {
        drawWendeBlock("f(x) = g(x) Wendepunkte:", wendeF1, true);
    } else {
        if (expr1.valid) drawWendeBlock("f(x) Wendepunkte:", wendeF1, expr1.valid);
        if (expr2.valid) drawWendeBlock("g(x) Wendepunkte:", wendeF2, expr2.valid);
    }

    sprite.clearClipRect();

    int totalH = (curY + genScrollY) - yStart;
    genMaxScrollHeight = std::max(0, totalH - (sprite.height() - 40));

    drawScrollBar(genScrollY, genMaxScrollHeight, CLIP_TOP, CLIP_BOT - CLIP_TOP);
    drawScrollHeader("--- Wendepunkte ---");
    drawScrollFooter();
}

static String fmtInfX(double x) {
    if (x <= -99.0) return "-oo";
    if (x >= 99.0)  return "+oo";
    return fmtX(x);
}

void drawResultsKruemmung() {
    sprite.fillSprite(COLOR_BG);

    const int HEADER_H = 22;
    const int FOOTER_H = 18;
    const int CLIP_TOP = HEADER_H;
    const int CLIP_BOT = sprite.height() - FOOTER_H;

    int yStart = HEADER_H;
    int curY   = yStart - genScrollY;

    drawScrollHeader("- Kruemmungsverhalten -");
    drawScrollFooter();

    if (mathBusy) { sprite.pushSprite(0, 0); return; }

    sprite.setClipRect(0, CLIP_TOP, sprite.width(), CLIP_BOT - CLIP_TOP);

    auto drawKrBlock = [&](const char* label, const KruemmungsResult& res, bool exprValid) {
        sprite.setTextColor(COLOR_YELLOW);
        sprite.drawString(label, 10, curY); curY += 14;

        if (!res.defined || res.intervals.empty()) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("Keine Daten!", 20, curY); curY += 12;
            curY += 6; return;
        }

        for (auto& iv : res.intervals) {
            sprite.setTextColor(TFT_WHITE);
            String iStr = "(" + fmtInfX(iv.xFrom) + "; " + fmtInfX(iv.xTo) + ")";
            sprite.drawString(iStr, 20, curY); curY += 12;

            if (iv.isConvex) {
                sprite.setTextColor(COLOR_GREEN);
                sprite.drawString(" linksgekr. (konvex)", 20, curY);
            } else {
                sprite.setTextColor(COLOR_ORANGE);
                sprite.drawString(" rechtsgekr. (konkav)", 20, curY);
            }
            curY += 13;
        }
        curY += 8;
    };

    bool bothValid_kr = expr1.valid && expr2.valid;
    bool sameFunc_kr  = bothValid_kr && f1_equals_f2;

    if (sameFunc_kr) {
        drawKrBlock("f(x) = g(x) Kruemmung:", kruemmungF1, true);
    } else {
        if (expr1.valid) drawKrBlock("f(x) Kruemmung:", kruemmungF1, expr1.valid);
        if (expr2.valid)drawKrBlock("g(x) Kruemmung:", kruemmungF2, expr2.valid);
    }

    sprite.clearClipRect();

    int totalH = (curY + genScrollY) - yStart;
    genMaxScrollHeight = std::max(0, totalH - (sprite.height() - 40));

    drawScrollBar(genScrollY, genMaxScrollHeight, CLIP_TOP, CLIP_BOT - CLIP_TOP);
    drawScrollHeader("- Kruemmungsverhalten -");
    drawScrollFooter();
}

void drawResultsMonotonie() {
    sprite.fillSprite(COLOR_BG);

    const int HEADER_H = 22;
    const int FOOTER_H = 18;
    const int CLIP_TOP = HEADER_H;
    const int CLIP_BOT = sprite.height() - FOOTER_H;

    int yStart = HEADER_H;
    int curY   = yStart - genScrollY;

    drawScrollHeader("---- Monotonie ----");
    drawScrollFooter();

    if (mathBusy) { sprite.pushSprite(0, 0); return; }

    sprite.setClipRect(0, CLIP_TOP, sprite.width(), CLIP_BOT - CLIP_TOP);

    auto drawMonBlock = [&](const char* label, const MonotonieResult& res,
                             const ExtremResult& ext, bool exprValid) {
        sprite.setTextColor(COLOR_YELLOW);
        sprite.drawString(label, 10, curY); curY += 14;

        if (!res.defined || res.intervals.empty()) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("Keine Daten!", 20, curY); curY += 12;
            curY += 6; return;
        }

        int hIdx = 1, tIdx = 1;

        auto extLabelAt = [&](double x) -> String {
            for (auto& p : ext.points) {
                if (fabs(p.x - x) < 0.02) {
                    if (p.type == EXT_HOCH) return "H" + String(hIdx++);
                    if (p.type == EXT_TIEF) return "T" + String(tIdx++);
                    return "SP";
                }
            }
            return "";
        };

        for (auto& iv : res.intervals) {
            String iStr = "(" + fmtInfX(iv.xFrom) + "; " + fmtInfX(iv.xTo) + ")";
            sprite.setTextColor(TFT_WHITE);
            sprite.drawString(iStr, 20, curY); curY += 12;

            if (iv.isIncreasing) {
                sprite.setTextColor(COLOR_GREEN);
                sprite.drawString(" streng monot. steig.", 20, curY);
            } else {
                sprite.setTextColor(COLOR_RED);
                sprite.drawString(" streng monot. fall.", 20, curY);
            }
            curY += 12;

            if (iv.xTo < 99.0) {
                String lbl = extLabelAt(iv.xTo);
                if (lbl.length() > 0) {
                    sprite.setTextColor(COLOR_YELLOW);
                    String pStr = " -> " + lbl + "(" + fmtX(iv.xTo) + ")";
                    sprite.drawString(pStr, 20, curY); curY += 12;
                }
            }
        }
        curY += 8;
    };

    bool bothValid_mo = expr1.valid && expr2.valid;
    bool sameFunc_mo  = bothValid_mo && f1_equals_f2;

    if (sameFunc_mo) {
        drawMonBlock("f(x) = g(x) Monotonie:", monotF1, extremF1, true);
    } else {
        if (expr1.valid) drawMonBlock("f(x) Monotonie:", monotF1, extremF1, expr1.valid);
        if (expr2.valid) drawMonBlock("g(x) Monotonie:", monotF2, extremF2, expr2.valid);
    }

    sprite.clearClipRect();

    int totalH = (curY + genScrollY) - yStart;
    genMaxScrollHeight = std::max(0, totalH - (sprite.height() - 40));

    drawScrollBar(genScrollY, genMaxScrollHeight, CLIP_TOP, CLIP_BOT - CLIP_TOP);
    drawScrollHeader("---- Monotonie ----");
    drawScrollFooter();
}

static String infBehaviorStr(InfBehavior b, double constVal) {
    switch (b) {
        case INF_PLUS_INF:  return "-> +oo";
        case INF_MINUS_INF: return "-> -oo";
        case INF_ZERO:      return "-> 0";
        case INF_CONST:     return "-> " + fmtX(constVal);
        case INF_OSCILLATE: return "unbestimmt";
        default:            return "?";
    }
}

void drawResultsUnendlich() {
    sprite.fillSprite(COLOR_BG);

    const int HEADER_H = 22;
    const int FOOTER_H = 18;
    const int CLIP_TOP = HEADER_H;
    const int CLIP_BOT = sprite.height() - FOOTER_H;

    drawScrollHeader("-- Verh. Unendlich --");
    drawScrollFooter();

    if (mathBusy) { sprite.pushSprite(0, 0); return; }

    int yStart = HEADER_H;
    int curY   = yStart - genScrollY;

    sprite.setClipRect(0, CLIP_TOP, sprite.width(), CLIP_BOT - CLIP_TOP);

    auto drawInfBlock = [&](const char* label, const char* funcName, const InfResult& res, bool exprValid) {
        sprite.setTextColor(COLOR_YELLOW);
        sprite.drawString(label, 10, curY); curY += 14;

        if (!res.defined) {
            sprite.setTextColor(COLOR_RED);
            sprite.drawString("Fehler!", 20, curY); curY += 12;
            curY += 8; return;
        }

        sprite.setTextColor(TFT_WHITE);
        sprite.drawString("x -> +oo:", 20, curY); curY += 12;

        String sPlus = String(funcName) + " " + infBehaviorStr(res.atPlusInf, res.constValPlus);
        uint16_t colP = (res.atPlusInf == INF_PLUS_INF)  ? COLOR_GREEN  :
                        (res.atPlusInf == INF_MINUS_INF) ? COLOR_RED    :
                        (res.atPlusInf == INF_ZERO)      ? COLOR_CYAN   :
                        (res.atPlusInf == INF_CONST)     ? COLOR_YELLOW : COLOR_GRAY;
        sprite.setTextColor(colP);
        sprite.drawString(sPlus, 30, curY); curY += 14;

        sprite.setTextColor(TFT_WHITE);
        sprite.drawString("x -> -oo:", 20, curY); curY += 12;

        String sMinus = String(funcName) + " " + infBehaviorStr(res.atMinusInf, res.constValMinus);
        uint16_t colM = (res.atMinusInf == INF_PLUS_INF)  ? COLOR_GREEN  :
                        (res.atMinusInf == INF_MINUS_INF) ? COLOR_RED    :
                        (res.atMinusInf == INF_ZERO)      ? COLOR_CYAN   :
                        (res.atMinusInf == INF_CONST)     ? COLOR_YELLOW : COLOR_GRAY;
        sprite.setTextColor(colM);
        sprite.drawString(sMinus, 30, curY); curY += 14;

        curY += 8;
    };

    bool bothValid_inf = expr1.valid && expr2.valid;
    bool sameFunc_inf  = bothValid_inf && f1_equals_f2;

    if (sameFunc_inf) {
        drawInfBlock("f(x) = g(x) Verhalten:", "f(x)", infF1, true);
    } else {
        if (expr1.valid) drawInfBlock("f(x) Verhalten:", "f(x)", infF1, expr1.valid);
        if (expr2.valid) drawInfBlock("g(x) Verhalten:", "g(x)", infF2, expr2.valid);
    }

    sprite.clearClipRect();

    int totalH = (curY + genScrollY) - yStart;
    genMaxScrollHeight = std::max(0, totalH - (sprite.height() - 40));

    drawScrollBar(genScrollY, genMaxScrollHeight, CLIP_TOP, CLIP_BOT - CLIP_TOP);
    drawScrollHeader("-- Verh. Unendlich --");
    drawScrollFooter();
}

void drawResultsTabelle() {
    sprite.fillSprite(COLOR_BG);

    const int HEADER_H = 22;
    const int FOOTER_H = 18;
    const int CLIP_TOP = HEADER_H;
    const int CLIP_BOT = sprite.height() - FOOTER_H;

    drawScrollHeader("--- Wertetabelle ---");
    drawScrollFooter();

    int yStart = HEADER_H;
    int curY = yStart - tabScrollY;

    int steps = (tEnd - tStart) / tStep + 1;
    int totalH = steps * 15 + 15;
    genMaxScrollHeight = std::max(0, totalH - (sprite.height() - 40));

    sprite.setClipRect(0, CLIP_TOP, sprite.width(), CLIP_BOT - CLIP_TOP);

    sprite.setTextColor(COLOR_YELLOW);
    sprite.drawString("x", 10, curY);
    if (expr1.valid) sprite.drawString("f(x)", 70, curY);
    if (expr2.valid) sprite.drawString("g(x)", 150, curY);
    curY += 15;

    for (int i = 0; i <= steps; i++) {
        double x = tStart + i * tStep;
        if (x > tEnd + 0.0001) break;

        if (curY > 10 && curY < sprite.height() - 10) {
            sprite.setTextColor(COLOR_CYAN);
            sprite.drawString(fmtX(x), 10, curY);
            if (expr1.valid) {
                double y1 = expr1.evaluate(x);
                sprite.setTextColor(TFT_WHITE);
                sprite.drawString(isnan(y1) ? "-" : fmtX(y1), 70, curY);
            }
            if (expr2.valid) {
                double y2 = expr2.evaluate(x);
                sprite.setTextColor(TFT_WHITE);
                sprite.drawString(isnan(y2) ? "-" : fmtX(y2), 150, curY);
            }
        }
        curY += 15;
    }

    sprite.clearClipRect();
    drawScrollBar(tabScrollY, genMaxScrollHeight, CLIP_TOP, CLIP_BOT - CLIP_TOP);
    drawScrollHeader("--- Wertetabelle ---");
    drawScrollFooter();
}

void drawPlot() {
    sprite.fillSprite(COLOR_BG);
    int w = sprite.width();
    int h = sprite.height();

    int cx = w / 2 - (int)(panX * scaleP);
    int cy = h / 2 + (int)(panY * scaleP);

    int sp = (int)scaleP;
    if (sp < 1) sp = 1;

    int xGridStart = cx % sp;
    if (xGridStart < 0) xGridStart += sp;
    for (int i = xGridStart; i < w; i += sp) {
        sprite.drawFastVLine(i, 0, h, 0x2104);
    }
    int yGridStart = cy % sp;
    if (yGridStart < 0) yGridStart += sp;
    for (int i = yGridStart; i < h; i += sp) {
        sprite.drawFastHLine(0, i, w, 0x2104);
    }

    int drawCX = std::max(0, std::min(w - 1, cx));
    int drawCY = std::max(0, std::min(h - 1, cy));
    sprite.drawFastVLine(drawCX, 0, h, TFT_WHITE);
    sprite.drawFastHLine(0, drawCY, w, TFT_WHITE);

    sprite.setTextSize(1);
    sprite.setTextDatum(TC_DATUM);

    {
        int firstGridPx = cx % sp;
        if (firstGridPx < 0) firstGridPx += sp;

        for (int px = firstGridPx; px < w; px += sp) {
            double mathX = (double)(px - cx) / scaleP;
            int gridIdx = (int)round(mathX * 10);
            if (abs(gridIdx) % 20 != 0) continue;
            if (fabs(mathX) < 0.01) continue;

            String s = String((int)round(mathX));
            int labelY = (cy >= 2 && cy <= h - 14) ? drawCY + 2 : h - 12;
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString(s, px, labelY);
        }
    }

    {
        sprite.setTextDatum(MR_DATUM);
        int firstGridPy = cy % sp;
        if (firstGridPy < 0) firstGridPy += sp;

        for (int py = firstGridPy; py < h; py += sp) {
            double mathY = (double)(cy - py) / scaleP;
            int gridIdx = (int)round(mathY * 10);
            if (abs(gridIdx) % 20 != 0) continue;
            if (fabs(mathY) < 0.01) continue;

            String s = String((int)round(mathY));
            int labelX = (cx >= 20 && cx <= w - 4) ? drawCX - 2 : (cx < 20 ? 24 : w - 4);
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString(s, labelX, py);
        }
    }

    const int Y_MARGIN = 4;
    auto drawFunc = [&](const MathExpression& expr, uint16_t color) {
        if (!expr.valid) return;
        int lastPy = -9999;
        for (int px = 0; px < w; px++) {
            double mathX = (double)(px - cx) / scaleP;
            double mathY = expr.evaluate(mathX);
            if (isnan(mathY) || isinf(mathY) || fabs(mathY) > 1e6) {
                lastPy = -9999;
                continue;
            }
            int py = cy - (int)(mathY * scaleP);
            py = std::max(-Y_MARGIN, std::min(h + Y_MARGIN, py));

            if (lastPy != -9999) {
                if (abs(py - lastPy) < h * 3) {
                    sprite.drawLine(px - 1, lastPy, px, py, color);
                }
            } else {
                sprite.drawPixel(px, py, color);
            }
            lastPy = py;
        }
    };

    drawFunc(expr1, TFT_GREEN);
    if (expr2.valid && !f1_equals_f2) drawFunc(expr2, 0x001F);

    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE);

    {
        int lx = w - 10;
        int ly = std::max(4, std::min(h - 10, drawCY - 8));
        sprite.setTextDatum(TC_DATUM);
        sprite.drawString("X", lx, ly);
    }
    {
        int lx = std::max(4, std::min(w - 12, drawCX + 4));
        int ly = 2;
        sprite.setTextDatum(TL_DATUM);
        sprite.drawString("Y", lx, ly);
    }

    sprite.fillRect(0, 0, w, 14, COLOR_BG);
    sprite.setTextColor(0x8410);
    sprite.setTextDatum(TL_DATUM);
    sprite.setTextSize(1);
    sprite.drawString("WASD:Bewegen Q/E:Zoom ESC:Menu", 4, 2);
    sprite.drawFastHLine(0, 13, w, COLOR_BORDER);
}

void drawFuncPicker() {
    sprite.fillSprite(COLOR_BG);

    sprite.fillRect(0, 0, sprite.width(), 18, COLOR_BG);
    sprite.drawRect(3, 1, sprite.width() - 6, 16, COLOR_BORDER);
    sprite.fillRect(4, 2, sprite.width() - 8, 14, 0x2104);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(COLOR_GREEN);
    sprite.drawString("Funktion einfuegen", sprite.width() / 2, 9);
    sprite.setTextDatum(TL_DATUM);

    const int TILE_W    = 55;
    const int TILE_H    = 18;
    const int TILE_PAD  = 2;
    const int START_Y   = 22;
    const int ROWS_VIS  = 5;

    int totalRows = (FUNC_PICKER_COUNT + FUNC_PICKER_COLS - 1) / FUNC_PICKER_COLS;

    for (int row = 0; row < ROWS_VIS; row++) {
        int absRow = row + funcPickerScrollRow;
        if (absRow >= totalRows) break;

        for (int col = 0; col < FUNC_PICKER_COLS; col++) {
            int idx = absRow * FUNC_PICKER_COLS + col;
            if (idx >= FUNC_PICKER_COUNT) break;

            int tx = TILE_PAD + col * (TILE_W + TILE_PAD);
            int ty = START_Y  + row * (TILE_H + TILE_PAD);

            bool selected = (idx == funcPickerCursorIdx);

            if (selected) {
                sprite.fillRect(tx, ty, TILE_W, TILE_H, TFT_WHITE);
                sprite.setTextColor(TFT_BLACK);
            } else {
                sprite.fillRect(tx, ty, TILE_W, TILE_H, 0x2945);
                sprite.drawRect(tx, ty, TILE_W, TILE_H, COLOR_BORDER);
                sprite.setTextColor(COLOR_CYAN);
            }
            sprite.setTextDatum(MC_DATUM);
            sprite.drawString(funcPickerItems[idx], tx + TILE_W / 2, ty + TILE_H / 2);
        }
    }

    if (totalRows > ROWS_VIS) {
        int barH = sprite.height() - START_Y - 20;
        int thumbH = std::max(6, barH * ROWS_VIS / totalRows);
        int thumbY = START_Y + (barH - thumbH) * funcPickerScrollRow / std::max(1, totalRows - ROWS_VIS);
        sprite.fillRect(sprite.width() - 4, START_Y, 3, barH, 0x2104);
        sprite.fillRect(sprite.width() - 4, thumbY, 3, thumbH, COLOR_GRAY);
    }

    sprite.fillRect(0, sprite.height() - 16, sprite.width(), 16, COLOR_BG);
    sprite.setTextColor(COLOR_GRAY);
    sprite.setTextDatum(TL_DATUM);
    sprite.drawString("Pfeile:Bewegen ENTER:Einfuegen ESC:Zurueck", 4, sprite.height() - 13);
}

void draw() {
    sprite.fillSprite(COLOR_BG);

    if      (currentState == STATE_MENU)                  drawMenu();
    else if (currentState == STATE_INPUT_F1)              drawInput("f(x) definieren", "f(x) = ", funcTemp, true);
    else if (currentState == STATE_INPUT_F2)              drawInput("g(x) definieren", "g(x) = ", funcTemp, true);
    else if (currentState == STATE_RESULTS_ACHSEN)        drawResultsAchsen();
    else if (currentState == STATE_INPUT_PUNKTPROBE)      drawPunktProbe();
    else if (currentState == STATE_RESULTS_GRADKOEFF)     drawResultsGradKoeff();
    else if (currentState == STATE_RESULTS_SYMMETRIE)     drawResultsSymmetrie();
    else if (currentState == STATE_RESULTS_EXTREM)        drawResultsExtrem();
    else if (currentState == STATE_RESULTS_WENDEPUNKTE)   drawResultsWendepunkte();
    else if (currentState == STATE_RESULTS_KRUEMMUNG)     drawResultsKruemmung();
    else if (currentState == STATE_RESULTS_MONOTONIE)     drawResultsMonotonie();
    else if (currentState == STATE_RESULTS_UNENDLICH)     drawResultsUnendlich();
    else if (currentState == STATE_INPUT_TAB_START)       drawInput("Tabelle: Startwert", "Start = ", funcTemp);
    else if (currentState == STATE_INPUT_TAB_END)         drawInput("Tabelle: Endwert", "Ende = ", funcTemp);
    else if (currentState == STATE_INPUT_TAB_STEP)        drawInput("Tabelle: Schrittweite", "Schritt = ", funcTemp);
    else if (currentState == STATE_RESULTS_TABELLE)       drawResultsTabelle();
    else if (currentState == STATE_PLOT)                  drawPlot();
    else if (currentState == STATE_FUNC_PICKER)           drawFuncPicker();

    if (showBusyOverlay) {
        int bw = 160, bh = 50;
        int bx = (sprite.width() - bw) / 2;
        int by = (sprite.height() - bh) / 2;

        sprite.fillRect(bx, by, bw, bh, COLOR_BG); 
        sprite.drawRect(bx, by, bw, bh, COLOR_YELLOW);
        
        sprite.setTextColor(COLOR_YELLOW);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("Berechnung läuft...", sprite.width() / 2, by + 15);

        int barW = bw - 30;
        sprite.drawRect(bx + 15, by + 32, barW, 10, COLOR_GRAY);
        sprite.fillRect(bx + 15, by + 32, (barW * mathProgress) / 100, 10, COLOR_GREEN);
    }
    sprite.pushSprite(0, 0);
}

static void handleScrollInput(char ch, bool escBack) {
    if (ch == 'w' || ch == ';') {
        genScrollY = std::max(0, genScrollY - 12);
        needRedraw = true;
    } else if (ch == 's' || ch == '.') {
        genScrollY = std::min(genMaxScrollHeight, genScrollY + 12);
        needRedraw = true;
    } else if (escBack) {
        currentState = STATE_MENU;
        needRedraw = true;
    }
}

void handleInput(char ch, Keyboard_Class::KeysState ks, bool isSpecial) {
    if (mathBusy || showBusyOverlay) return;

    if (currentState == STATE_MENU) {
        if (ch == 'w' || ch == ';') { 
            selectedMenuItem = (selectedMenuItem <= 0) ? numMenuItems - 1 : selectedMenuItem - 1;
            needRedraw = true;
        } else if (ch == 's' || ch == '.') { 
            selectedMenuItem = (selectedMenuItem >= numMenuItems - 1) ? 0 : selectedMenuItem + 1;
            needRedraw = true;
        } else if (ks.enter) {
            if (selectedMenuItem == 0) {
                funcTemp = expr1.rawStr; currentState = STATE_INPUT_F1;
            } else if (selectedMenuItem == 1) {
                funcTemp = expr2.rawStr; currentState = STATE_INPUT_F2;
            } else if (selectedMenuItem == 2) {
                currentMathCmd = CMD_CALC_INTERSECTS; 
                currentState = STATE_RESULTS_ACHSEN; 
                resultScrollY = 0;
            } else if (selectedMenuItem == 3) {
                funcTemp = "";
                punktProbeResult = "";
                currentState = STATE_INPUT_PUNKTPROBE;
            } else if (selectedMenuItem == 4) {
                currentMathCmd = CMD_CALC_GRADKOEFF;
                gradScrollY = 0;
                currentState = STATE_RESULTS_GRADKOEFF;
            } else if (selectedMenuItem == 5) {
                currentMathCmd = CMD_CALC_SYMMETRIE;
                currentState = STATE_RESULTS_SYMMETRIE;
            } else if (selectedMenuItem == 6) {
                currentMathCmd = CMD_CALC_EXTREM;
                genScrollY = 0;
                currentState = STATE_RESULTS_EXTREM;
            } else if (selectedMenuItem == 7) {
                currentMathCmd = CMD_CALC_WENDEPUNKTE;
                genScrollY = 0;
                currentState = STATE_RESULTS_WENDEPUNKTE;
            } else if (selectedMenuItem == 8) {
                currentMathCmd = CMD_CALC_KRUEMMUNG;
                genScrollY = 0;
                currentState = STATE_RESULTS_KRUEMMUNG;
            } else if (selectedMenuItem == 9) {
                currentMathCmd = CMD_CALC_MONOTONIE;
                genScrollY = 0;
                currentState = STATE_RESULTS_MONOTONIE;
            } else if (selectedMenuItem == 10) {
                currentMathCmd = CMD_CALC_UNENDLICH;
                genScrollY = 0;
                currentState = STATE_RESULTS_UNENDLICH;
            } else if (selectedMenuItem == 11) {
                funcTemp = "";
                currentState = STATE_INPUT_TAB_START;
            } else if (selectedMenuItem == 12) {
                panX = 0; panY = 0;
                currentState = STATE_PLOT;
            }

            needRedraw = true;
        }
    }
    else if (currentState == STATE_INPUT_F1 || currentState == STATE_INPUT_F2) {
        if (ch == '`') {
            currentState = STATE_MENU;
            needRedraw = true;
        } else if (ks.fn) {
            funcPickerReturnState = currentState;
            funcPickerCursorIdx = 0;
            funcPickerScrollRow = 0;
            currentState = STATE_FUNC_PICKER;
            needRedraw = true;
        } else if (ks.enter) {
            if (currentState == STATE_INPUT_F1) {
                expr1.rawStr = funcTemp;
                currentMathCmd = CMD_VALIDATE_F1;
            } else {
                expr2.rawStr = funcTemp;
                currentMathCmd = CMD_VALIDATE_F2;
            }
            needsRecalc = true;
            currentState = STATE_MENU;
            needRedraw = true;
        } else if (ks.del) {
            if (funcTemp.length() > 0) { funcTemp.remove(funcTemp.length() - 1); needRedraw = true; }
        } else if (ks.tab) {
            funcTemp += "^"; needRedraw = true;
        } else if (ch == 27) {
            currentState = STATE_MENU; needRedraw = true;
        } else if (!isSpecial && ch >= 32 && ch <= 126 && ch != '`') {
            funcTemp += ch; needRedraw = true;
        }
    }
    else if (currentState == STATE_FUNC_PICKER) {
        const int ROWS_VIS  = 5;
        int totalRows = (FUNC_PICKER_COUNT + FUNC_PICKER_COLS - 1) / FUNC_PICKER_COLS;

        if (ch == 'a' || ch == ',') {
            if (funcPickerCursorIdx % FUNC_PICKER_COLS > 0) {
                funcPickerCursorIdx--;
                needRedraw = true;
            }
        } else if (ch == 'd' || ch == '/') {
            if (funcPickerCursorIdx % FUNC_PICKER_COLS < FUNC_PICKER_COLS - 1
                && funcPickerCursorIdx + 1 < FUNC_PICKER_COUNT) {
                funcPickerCursorIdx++;
                needRedraw = true;
            }
        } else if (ch == 'w' || ch == ';') {
            if (funcPickerCursorIdx >= FUNC_PICKER_COLS) {
                funcPickerCursorIdx -= FUNC_PICKER_COLS;
                int curRow = funcPickerCursorIdx / FUNC_PICKER_COLS;
                if (curRow < funcPickerScrollRow) funcPickerScrollRow = curRow;
                needRedraw = true;
            }
        } else if (ch == 's' || ch == '.') {
            if (funcPickerCursorIdx + FUNC_PICKER_COLS < FUNC_PICKER_COUNT) {
                funcPickerCursorIdx += FUNC_PICKER_COLS;
                int curRow = funcPickerCursorIdx / FUNC_PICKER_COLS;
                if (curRow >= funcPickerScrollRow + ROWS_VIS)
                    funcPickerScrollRow = curRow - ROWS_VIS + 1;
                needRedraw = true;
            }
        } else if (ks.enter) {
            String ins = String(funcPickerItems[funcPickerCursorIdx]);
            if (ins == "pi") {
                funcTemp += "3.14159265";
            } else if (ins == "e") {
                funcTemp += "2.71828182";
            } else if (ins == "1/") {
                funcTemp += "1/(";
            } else {
                int paren = ins.indexOf("()");
                if (paren != -1) {
                    funcTemp += ins.substring(0, paren + 1);
                } else {
                    funcTemp += ins;
                }
            }
            currentState = funcPickerReturnState;
            needRedraw = true;
        } else if (ch == 27 || ch == '`' || ks.del) {
            currentState = funcPickerReturnState;
            needRedraw = true;
        }
    }
    else if (currentState == STATE_INPUT_TAB_START || currentState == STATE_INPUT_TAB_END || currentState == STATE_INPUT_TAB_STEP) {
        if (ks.enter) {
            if (currentState == STATE_INPUT_TAB_START) {
                tabStartStr = funcTemp; funcTemp = ""; currentState = STATE_INPUT_TAB_END;
            } else if (currentState == STATE_INPUT_TAB_END) {
                tabEndStr = funcTemp; funcTemp = ""; currentState = STATE_INPUT_TAB_STEP;
            } else if (currentState == STATE_INPUT_TAB_STEP) {
                tabStepStr = funcTemp; funcTemp = "";
                
                tStart = tabStartStr.toDouble();
                tEnd = tabEndStr.toDouble();
                tStep = fabs(tabStepStr.toDouble());
                if (tStep < 0.001) tStep = 1.0;
                if (tStart > tEnd) std::swap(tStart, tEnd);
                
                tabScrollY = 0;
                currentState = STATE_RESULTS_TABELLE;
            }
            needRedraw = true;
        } else if (ks.del) {
            if (funcTemp.length() > 0) { funcTemp.remove(funcTemp.length() - 1); needRedraw = true; }
        } else if (ch == 27 || ch == '`') {
            currentState = STATE_MENU; needRedraw = true;
        } else if (!isSpecial && ((ch >= 48 && ch <= 57) || ch == 46 || ch == 45)) {
            funcTemp += ch; needRedraw = true;
        }
    }
    else if (currentState == STATE_RESULTS_TABELLE) {
        if (ch == 'w' || ch == ';') { tabScrollY = std::max(0, tabScrollY - 15); needRedraw = true; }
        else if (ch == 's' || ch == '.') { tabScrollY = std::min(genMaxScrollHeight, tabScrollY + 15); needRedraw = true; }
        else if (ch == 27 || ch == '`' || ks.del || ks.enter) { currentState = STATE_MENU; needRedraw = true; }
    }
    else if (currentState == STATE_RESULTS_ACHSEN) {
        if (ch == 'w' || ch == ';') {
            resultScrollY = std::max(0, resultScrollY - 12);
            needRedraw = true;
        } else if (ch == 's' || ch == '.') {
            resultScrollY = std::min(maxScrollHeight, resultScrollY + 12);
            needRedraw = true;
        } else if (ch == 27 || ch == '`' || ks.del) {
            currentState = STATE_MENU;
            needRedraw = true;
        }
    }
    else if (currentState == STATE_INPUT_PUNKTPROBE) {
        if (ks.enter) {
            probeInput = funcTemp;
            currentMathCmd = CMD_CALC_PUNKTPROBE;
            needRedraw = true;
        } else if (ch == 27 || ch == '`') { 
            currentState = STATE_MENU;
            needRedraw = true;
        } else if (ks.del) {
            if (funcTemp.length() > 0) {
                funcTemp.remove(funcTemp.length() - 1);
                resF1.defined = false; 
                resF2.defined = false;
                needRedraw = true;
            }
        } else if (!isSpecial && ch >= 32 && ch <= 126) {
            funcTemp += ch;
            needRedraw = true;
        }
    }
    else if (currentState == STATE_RESULTS_GRADKOEFF) {
        if (ch == 'w' || ch == ';') {
            gradScrollY = std::max(0, gradScrollY - 12);
            needRedraw = true;
        } else if (ch == 's' || ch == '.') {
            gradScrollY = std::min(gradMaxScrollHeight, gradScrollY + 12);
            needRedraw = true;
        } else if (ch == 27 || ch == '`' || ks.del) {
            currentState = STATE_MENU;
            needRedraw = true;
        }
    }
    else if (currentState == STATE_RESULTS_SYMMETRIE) {
        if (ch == 27 || ch == '`' || ks.del || ks.enter) {
            currentState = STATE_MENU;
            needRedraw = true;
        }
    }
    else if (currentState == STATE_PLOT) {
        double moveStep = 15.0 / scaleP;
        if (ch == 'w' || ch == ';') { panY += moveStep; needRedraw = true; }
        else if (ch == 's' || ch == '.') { panY -= moveStep; needRedraw = true; }
        else if (ch == 'a' || ch == ',') { panX -= moveStep; needRedraw = true; }
        else if (ch == 'd' || ch == '/') { panX += moveStep; needRedraw = true; }
        else if (ch == 'q') { scaleP = std::max(2.0, scaleP - 2.0); needRedraw = true; }
        else if (ch == 'e') { scaleP = std::min(50.0, scaleP + 2.0); needRedraw = true; }
        else if (ch == 27 || ch == '`' || ch == 'm' || ch == 'M' || ks.enter || ks.del) { 
            currentState = STATE_MENU; needRedraw = true; 
        }
    }
    else if (currentState == STATE_RESULTS_EXTREM     ||
             currentState == STATE_RESULTS_WENDEPUNKTE ||
             currentState == STATE_RESULTS_KRUEMMUNG   ||
             currentState == STATE_RESULTS_MONOTONIE   ||
             currentState == STATE_RESULTS_UNENDLICH) {
        bool goBack = (ch == 27 || ch == '`' || ks.del || ks.enter);
        handleScrollInput(ch, goBack);
    }
}

void Task_TFT(void *pvParameters) {
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(COLOR_BG);
    sprite.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());
    needRedraw = true;
    bool lastMathBusy = false;

    while (true) {
        M5Cardputer.update();

        if (lastMathBusy && !mathBusy) {
            needRedraw = true;
        }
        lastMathBusy = mathBusy;
        
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState ks = M5Cardputer.Keyboard.keysState();
            if (ks.enter) {
                handleInput('\n', ks, true);
            } else if (ks.del) {
                handleInput('\b', ks, true);
            } else if (ks.tab) {
                handleInput('\t', ks, true);
            } else if (ks.fn) {
                handleInput('\f', ks, true);
            } else {
                for (auto ch : ks.word) {
                    handleInput(ch, ks, false);
                }
            }
        }

        if (needRedraw) {
            draw();
            needRedraw = false;
        }
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
}