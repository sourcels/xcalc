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
    "10. Monotonie"
};

int numMenuItems = 2;
int selectedMenuItem = 0;
int resultScrollY = 0;
int maxScrollHeight = 0;

static int gradScrollY = 0;
static int gradMaxScrollHeight = 0;

static M5Canvas sprite(&M5Cardputer.Display);

static const uint16_t COLOR_BG     = TFT_BLACK;
static const uint16_t COLOR_BORDER = 0x4208;
static const uint16_t COLOR_TEXT   = TFT_WHITE;
static const uint16_t COLOR_GREEN  = TFT_GREEN;
static const uint16_t COLOR_YELLOW = TFT_YELLOW;
static const uint16_t COLOR_RED    = TFT_RED;
static const uint16_t COLOR_GRAY   = 0x8410;
static const uint16_t COLOR_CYAN   = TFT_CYAN;

void drawMenu() {
    sprite.setTextDatum(TL_DATUM);
    sprite.setTextSize(1);
    sprite.setTextColor(COLOR_GREEN, COLOR_BG);
    sprite.drawString("=== Kurvendiskussion ===", 10, 5);

    numMenuItems = (expr1.valid || expr2.valid) ? 10 : 2;

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
                if (roots.size() > 1) {
                    prefix += String(i + 1);
                }
                
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
        drawSection("f(x) & g(x) Schnitt:", f1_f2_intersects, {0,0}, false, true);
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
    // Section header
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

    int yStart = 22;
    int curY = yStart - gradScrollY;

    if (expr1.valid) drawGradBlock("f(x) Grad & Koeff:", gradKoeffF1, expr1.rawStr, curY);
    if (expr2.valid) drawGradBlock("g(x) Grad & Koeff:", gradKoeffF2, expr2.rawStr, curY);
    if (!expr1.valid && !expr2.valid) {
        sprite.setTextColor(COLOR_GRAY);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString("Keine Funktion definiert!", sprite.width()/2, 70);
        sprite.setTextDatum(TL_DATUM);
    }

    int totalContentH = (curY + gradScrollY) - yStart;
    gradMaxScrollHeight = std::max(0, totalContentH - (sprite.height() - 40));

    sprite.fillRect(0, 0, sprite.width(), 20, COLOR_BG);
    sprite.drawRect(5, 2, sprite.width() - 10, 16, COLOR_BORDER);
    sprite.fillRect(6, 3, sprite.width() - 12, 14, 0x2104);
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(COLOR_GREEN);
    sprite.drawString("- Grad & Koeffizienten -", sprite.width() / 2, 10);

    if (gradMaxScrollHeight > 0) {
        int sbH = sprite.height() - 40;
        int thumbH = std::max(8, sbH * sbH / (sbH + gradMaxScrollHeight));
        int thumbY = 22 + (sbH - thumbH) * gradScrollY / gradMaxScrollHeight;
        sprite.fillRect(sprite.width() - 4, 22, 3, sbH, 0x2104);
        sprite.fillRect(sprite.width() - 4, thumbY, 3, thumbH, COLOR_GRAY);
    }

    sprite.fillRect(0, sprite.height() - 18, sprite.width(), 18, COLOR_BG);
    sprite.setTextColor(COLOR_GRAY);
    sprite.setTextDatum(TL_DATUM);
    sprite.drawString("W/S: Scroll | ESC: Zurueck", 10, sprite.height() - 15);
}

void drawResultsSymmetrie() {
    sprite.fillSprite(COLOR_BG);

    // Header
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

void draw() {
    sprite.fillSprite(COLOR_BG);

    if (currentState == STATE_MENU)                   drawMenu();
    else if (currentState == STATE_INPUT_F1)           drawInput("f(x) definieren", "f(x) = ", funcTemp);
    else if (currentState == STATE_INPUT_F2)           drawInput("g(x) definieren", "g(x) = ", funcTemp);
    else if (currentState == STATE_RESULTS_ACHSEN)     drawResultsAchsen();
    else if (currentState == STATE_INPUT_PUNKTPROBE)   drawPunktProbe();
    else if (currentState == STATE_RESULTS_GRADKOEFF)  drawResultsGradKoeff();
    else if (currentState == STATE_RESULTS_SYMMETRIE)   drawResultsSymmetrie();

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
            if (selectedMenuItem == 0) { funcTemp = expr1.rawStr; currentState = STATE_INPUT_F1; }
            else if (selectedMenuItem == 1) { funcTemp = expr2.rawStr; currentState = STATE_INPUT_F2; }
            else if (selectedMenuItem == 2) { 
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
    } else if (currentState == STATE_INPUT_PUNKTPROBE) {
        if (ks.enter) {
            probeInput = funcTemp;
            currentMathCmd = CMD_CALC_PUNKTPROBE;
            needRedraw = true;
        }
        else if (ch == 27 || ch == '`') { 
            currentState = STATE_MENU;
            needRedraw = true;
        }
        else if (ks.del) {
            if (funcTemp.length() > 0) {
                funcTemp.remove(funcTemp.length() - 1);
                resF1.defined = false; 
                resF2.defined = false;
                needRedraw = true;
            }
        }
        else if (!isSpecial && ch >= 32 && ch <= 126) {
            funcTemp += ch;
            needRedraw = true;
        }
    } else if (currentState == STATE_RESULTS_GRADKOEFF) {
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
    } else if (currentState == STATE_RESULTS_SYMMETRIE) {
        if (ch == 27 || ch == '`' || ks.del || ks.enter) {
            currentState = STATE_MENU;
            needRedraw = true;
        }
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