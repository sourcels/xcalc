#pragma once
#include <Arduino.h>
#include "M5Cardputer.h"

enum UIState {
    STATE_MENU,
    STATE_INPUT_F1,
    STATE_INPUT_F2,
    STATE_RESULTS_ACHSEN,
    STATE_INPUT_PUNKTPROBE,
    STATE_RESULTS_GRADKOEFF,
    STATE_RESULTS_SYMMETRIE
};

extern UIState currentState;
void Task_TFT(void *pvParameters);