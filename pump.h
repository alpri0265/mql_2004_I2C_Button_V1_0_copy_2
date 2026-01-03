#pragma once
#include <Arduino.h>
#include "types.h"

void pumpBegin();
void pumpSetEnable(bool en);

void pumpStartSteps(uint32_t stepsPerSec);
void pumpStop();

void pumpRunCont(int32_t flow_x100, uint32_t pumpGain);
void pumpRunPulse(bool &phaseOn, uint32_t &phaseStartMs, const Settings &S, int32_t flow_x100);
