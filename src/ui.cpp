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
    "5. Extrempunkte",
    "6. Wendepunkte",
    "7. Kruemmungsverhalten",
    "8. Monotonie"
};

int numMenuItems = 2;
int selectedMenuItem = 0;
int resultScrollY = 0;
int maxScrollHeight = 0;

static M5Canvas sprite(&M5Cardputer.Display);
volatile bool needRedraw = true;

static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_BORDER = 0x4208;
static const uint16_t COLOR_TEXT = TFT_WHITE;
static const uint16_t COLOR_GREEN = TFT_GREEN;
static const uint16_t COLOR_YELLOW = TFT_YELLOW;
static const uint16_t COLOR_RED = TFT_RED;
static const uint16_t COLOR_GRAY = 0x8410;

void drawMenu() {
    sprite.setTextDatum(TL_DATUM);
    sprite.setTextSize(1);
    sprite.setTextColor(COLOR_GREEN, COLOR_BG);
    sprite.drawString("=== Kurvendiskussion ===", 10, 5);

    numMenuItems = (expr1.valid || expr2.valid) ? 8 : 2;

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
    drawInput("Punktprobe (x,y)", "P = ", funcTemp);

    sprite.setTextColor(COLOR_GRAY, COLOR_BG);
    sprite.setTextDatum(TL_DATUM);
    sprite.drawString("Tipp: Nutze Leerzeichen (z.B. '2 4')", 10, 60);

    if (punktProbeResult != "") {
        sprite.setTextColor(punktProbeColor, COLOR_BG);
        sprite.setTextDatum(MC_DATUM);
        sprite.drawString(punktProbeResult, sprite.width() / 2, 95);
    }
}

void draw() {
    sprite.fillSprite(COLOR_BG);

    if (currentState == STATE_MENU) drawMenu();
    else if (currentState == STATE_INPUT_F1) drawInput("f(x) definieren", "f(x) = ", funcTemp);
    else if (currentState == STATE_INPUT_F2) drawInput("g(x) definieren", "g(x) = ", funcTemp);
    else if (currentState == STATE_RESULTS_ACHSEN) drawResultsAchsen();
    else if (currentState == STATE_INPUT_PUNKTPROBE) drawPunktProbe();

    if (mathBusy) {
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
    if (mathBusy) return;

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
    }
    else if (currentState == STATE_INPUT_PUNKTPROBE) {
        if (ks.enter) {
            int sepPos = funcTemp.indexOf(',');
            if (sepPos == -1) sepPos = funcTemp.indexOf(' '); 

            if (sepPos != -1 && expr1.valid) {
                double px = funcTemp.substring(0, sepPos).toDouble();
                double py = funcTemp.substring(sepPos + 1).toDouble();
                
                double fVal = expr1.evaluate(px);

                if (abs(fVal - py) < 0.01) {
                    punktProbeResult = "Der Punkt liegt auf dem Graphen";
                    punktProbeColor = COLOR_RED;
                } else {
                    punktProbeResult = "Der Punkt liegt nicht auf dem Graphen";
                    punktProbeColor = COLOR_GREEN;
                }

                if (expr2.valid) {
                    double gVal = expr2.evaluate(px);
                    bool onG = abs(gVal - py) < 0.01;
                    if (abs(fVal - py) < 0.01 && onG) {
                        punktProbeResult = "Liegt auf f(x) UND g(x)";
                        punktProbeColor = COLOR_RED;
                    }
                }
            } else if (!expr1.valid) {
                punktProbeResult = "Bitte zuerst f(x) definieren!";
                punktProbeColor = COLOR_YELLOW;
            } else {
                punktProbeResult = "Format: 'X Y' oder 'X,Y' nutzen!";
                punktProbeColor = COLOR_YELLOW;
            }
            needRedraw = true;
        } else if (ch == 27 || (ks.del && funcTemp == "")) {
            currentState = STATE_MENU;
            needRedraw = true;
        } else if (ks.del) {
            funcTemp.remove(funcTemp.length() - 1);
            needRedraw = true;
        } else if (!isSpecial && ch >= 32 && ch <= 126) {
            if (punktProbeResult != "") punktProbeResult = "";
            funcTemp += ch;
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