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
    sprite.drawString("=== Kurvendiskussion ===", 10, 5);

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

void drawInput(const char* title, const char* prefix, String currentInput) {
    sprite.setTextColor(COLOR_GREEN, COLOR_BG);
    sprite.drawString(title, 10, 10);
    sprite.setTextColor(TFT_WHITE, COLOR_BG);
    sprite.drawString("Eingabe:", 10, 25);
    sprite.drawRect(8, 40, sprite.width() - 16, 45, COLOR_BORDER);

    sprite.setCursor(12, 45);
    sprite.print(prefix + currentInput + "_");

    sprite.setTextColor(COLOR_YELLOW, COLOR_BG);
    sprite.drawString("ENTER: OK | ESC: Abbruch", 10, sprite.height() - 15);
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

    if (expr1.valid) drawSection("f(x) Achsen:", f1_roots, f1_y_int, f1_has_y_int);
    if (expr2.valid) drawSection("g(x) Achsen:", f2_roots, f2_y_int, f2_has_y_int);
    if (expr1.valid && expr2.valid) {
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

    if (expr1.valid) drawGradBlock("f(x) Grad & Koeff:", gradKoeffF1, expr1.rawStr, curY);
    if (expr2.valid) drawGradBlock("g(x) Grad & Koeff:", gradKoeffF2, expr2.rawStr, curY);
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

        if (!exprValid) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("(nicht definiert)", 20, curY); curY += 12;
            curY += 6;
            return;
        }
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

    drawSymBlock("f(x) Symmetrie:", symF1, expr1.valid);
    drawSymBlock("g(x) Symmetrie:", symF2, expr2.valid);

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

    // Draw header and footer first so they are always on top
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

        if (!exprValid) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("(nicht definiert)", 20, curY); curY += 12;
            curY += 6; return;
        }
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
                lbl = "SP(";  // Sattelpunkt
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

    drawExtBlock("f(x) Extrempunkte:", extremF1, expr1.valid);
    drawExtBlock("g(x) Extrempunkte:", extremF2, expr2.valid);

    sprite.clearClipRect();

    int totalH = (curY + genScrollY) - yStart;
    genMaxScrollHeight = std::max(0, totalH - (sprite.height() - 40));

    drawScrollBar(genScrollY, genMaxScrollHeight, CLIP_TOP, CLIP_BOT - CLIP_TOP);
    // Redraw header and footer on top of scrollbar/content
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

        if (!exprValid) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("(nicht definiert)", 20, curY); curY += 12;
            curY += 6; return;
        }
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
                sprite.drawString(" kk->kx (links-/rechts)", 20, curY);
            } else {
                sprite.drawString(" kx->kk (rechts-/links)", 20, curY);
            }
            curY += 12;
        }
        curY += 8;
    };

    drawWendeBlock("f(x) Wendepunkte:", wendeF1, expr1.valid);
    drawWendeBlock("g(x) Wendepunkte:", wendeF2, expr2.valid);

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

        if (!exprValid) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("(nicht definiert)", 20, curY); curY += 12;
            curY += 6; return;
        }
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

    drawKrBlock("f(x) Kruemmung:", kruemmungF1, expr1.valid);
    drawKrBlock("g(x) Kruemmung:", kruemmungF2, expr2.valid);

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

        if (!exprValid) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("(nicht definiert)", 20, curY); curY += 12;
            curY += 6; return;
        }
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

    drawMonBlock("f(x) Monotonie:", monotF1, extremF1, expr1.valid);
    drawMonBlock("g(x) Monotonie:", monotF2, extremF2, expr2.valid);

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

    auto drawInfBlock = [&](const char* label, const InfResult& res, bool exprValid) {
        sprite.setTextColor(COLOR_YELLOW);
        sprite.drawString(label, 10, curY); curY += 14;

        if (!exprValid) {
            sprite.setTextColor(COLOR_GRAY);
            sprite.drawString("(nicht definiert)", 20, curY); curY += 12;
            curY += 8; return;
        }
        if (!res.defined) {
            sprite.setTextColor(COLOR_RED);
            sprite.drawString("Fehler!", 20, curY); curY += 12;
            curY += 8; return;
        }

        sprite.setTextColor(TFT_WHITE);
        sprite.drawString("x -> +oo:", 20, curY); curY += 12;

        String sPlus = "f(x) " + infBehaviorStr(res.atPlusInf, res.constValPlus);
        uint16_t colP = (res.atPlusInf == INF_PLUS_INF)  ? COLOR_GREEN  :
                        (res.atPlusInf == INF_MINUS_INF) ? COLOR_RED    :
                        (res.atPlusInf == INF_ZERO)      ? COLOR_CYAN   :
                        (res.atPlusInf == INF_CONST)     ? COLOR_YELLOW : COLOR_GRAY;
        sprite.setTextColor(colP);
        sprite.drawString(sPlus, 30, curY); curY += 14;

        sprite.setTextColor(TFT_WHITE);
        sprite.drawString("x -> -oo:", 20, curY); curY += 12;

        String sMinus = "f(x) " + infBehaviorStr(res.atMinusInf, res.constValMinus);
        uint16_t colM = (res.atMinusInf == INF_PLUS_INF)  ? COLOR_GREEN  :
                        (res.atMinusInf == INF_MINUS_INF) ? COLOR_RED    :
                        (res.atMinusInf == INF_ZERO)      ? COLOR_CYAN   :
                        (res.atMinusInf == INF_CONST)     ? COLOR_YELLOW : COLOR_GRAY;
        sprite.setTextColor(colM);
        sprite.drawString(sMinus, 30, curY); curY += 14;

        curY += 8;
    };

    drawInfBlock("f(x) Verhalten:", infF1, expr1.valid);
    drawInfBlock("g(x) Verhalten:", infF2, expr2.valid);

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

    int cx = w / 2 - panX * scaleP;
    int cy = h / 2 + panY * scaleP;

    for (int i = 0; i < w; i++) {
        if ((i - cx) % (int)scaleP == 0) sprite.drawFastVLine(i, 0, h, 0x2104);
    }
    for (int i = 0; i < h; i++) {
        if ((i - cy) % (int)scaleP == 0) sprite.drawFastHLine(0, i, w, 0x2104);
    }

    if (cx >= 0 && cx < w) sprite.drawFastVLine(cx, 0, h, TFT_WHITE);
    if (cy >= 0 && cy < h) sprite.drawFastHLine(0, cy, w, TFT_WHITE);

    auto drawFunc = [&](const MathExpression& expr, uint16_t color) {
        if (!expr.valid) return;
        int lastY = -999;
        for (int px = 0; px < w; px++) {
            double mathX = (px - cx) / scaleP;
            double mathY = expr.evaluate(mathX);
            if (!isnan(mathY) && !isinf(mathY)) {
                int py = cy - mathY * scaleP;
                if (py >= -50 && py <= h + 50) {
                    if (lastY != -999 && px > 0) sprite.drawLine(px - 1, lastY, px, py, color);
                    else sprite.drawPixel(px, py, color);
                }
                lastY = py;
            } else lastY = -999;
        }
    };

    drawFunc(expr1, TFT_GREEN);
    drawFunc(expr2, TFT_BLUE);

    sprite.fillRect(0, 0, w, 15, COLOR_BG);
    sprite.setTextColor(TFT_WHITE);
    sprite.drawString("WASD/Arrows: Move | M: Menu", 5, 2);
    sprite.drawFastHLine(0, 14, w, COLOR_BORDER);
}

void draw() {
    sprite.fillSprite(COLOR_BG);

    if      (currentState == STATE_MENU)                  drawMenu();
    else if (currentState == STATE_INPUT_F1)              drawInput("f(x) definieren", "f(x) = ", funcTemp);
    else if (currentState == STATE_INPUT_F2)              drawInput("g(x) definieren", "g(x) = ", funcTemp);
    else if (currentState == STATE_RESULTS_ACHSEN)        drawResultsAchsen();
    else if (currentState == STATE_INPUT_PUNKTPROBE)      drawPunktProbe();
    else if (currentState == STATE_RESULTS_GRADKOEFF)     drawResultsGradKoeff();
    else if (currentState == STATE_RESULTS_SYMMETRIE)     drawResultsSymmetrie();
    else if (currentState == STATE_RESULTS_EXTREM)        drawResultsExtrem();
    else if (currentState == STATE_RESULTS_WENDEPUNKTE)   drawResultsWendepunkte();
    else if (currentState == STATE_RESULTS_KRUEMMUNG)     drawResultsKruemmung();
    else if (currentState == STATE_RESULTS_MONOTONIE)     drawResultsMonotonie();
    else if (currentState == STATE_RESULTS_UNENDLICH)     drawResultsUnendlich();
    else if (currentState == STATE_INPUT_TAB_START)       drawInput("Tabelle Start", "Start = ", funcTemp);
    else if (currentState == STATE_INPUT_TAB_END)         drawInput("Tabelle End", "End = ", funcTemp);
    else if (currentState == STATE_INPUT_TAB_STEP)        drawInput("Tabelle Step", "Step = ", funcTemp);
    else if (currentState == STATE_RESULTS_TABELLE)       drawResultsTabelle();
    else if (currentState == STATE_PLOT)                  drawPlot();

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
        if (ks.enter) {
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
        } else if (ch == '\t') {
            funcTemp += "^"; needRedraw = true;
        } else if (ch == 27 || ch == '`') {
            currentState = STATE_MENU; needRedraw = true;
        } else if (!isSpecial && ch >= 32 && ch <= 126) {
            funcTemp += ch; needRedraw = true;
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
        } else if (!isSpecial && ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-')) {
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