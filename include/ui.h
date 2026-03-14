#pragma once
#include <Arduino.h>
#include "M5Cardputer.h"

enum UIState {
    STATE_MENU,
    STATE_INPUT_F1,
    STATE_INPUT_F2,
    STATE_RESULTS_ACHSEN,
    STATE_INPUT_PUNKTPROBE
};

extern volatile bool needRedraw;
extern UIState currentState;
extern String func1;
extern String func2;
extern String funcTemp;

void Task_TFT(void *pvParameters);