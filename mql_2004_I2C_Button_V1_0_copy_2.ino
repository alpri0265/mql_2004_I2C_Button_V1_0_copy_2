#include "config.h"
#include "types.h"
#include "settings.h"
#include "input.h"
#include "reco.h"
#include "pump.h"
#include "ui.h"
#include "menu.h"

static AppState state = ST_READY;
static MenuState menu;

static bool wizardDone = false;  // Wizard один раз, дальше START/STOP

static int32_t rec_x100 = 55;
static int32_t potMin_x100 = 27;
static int32_t potMax_x100 = 110;
static int32_t set_x100 = 55;

static bool pulseOn = true;
static uint32_t pulseMs = 0;

// HARD START (direct pin)
static bool     startRawPrev = false;
static uint32_t startRawLastMs = 0;

// HARD DIA ACCEL (direct UP/DOWN only in ST_WIZ_DIA)
static bool     upPrev = false, dnPrev = false;
static uint32_t upPressMs = 0, dnPressMs = 0;
static uint32_t upLastRptMs = 0, dnLastRptMs = 0;

// Calibration
static const int32_t CAL_FLOW_U_X100 = 100; // 1.00 u/min
static uint16_t calTotalSec = 60;
static uint32_t calStartMs = 0;
static uint32_t calDurationMs = 60000UL;

static int32_t calMeasuredMl_x100 = 0; // 0..9999 (0.00..99.99 ml)
static uint8_t calDigitIdx = 0;        // 0..3 (tens, ones, tenths, hundredths)

static int32_t clampI32(int32_t v, int32_t lo, int32_t hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

static int32_t potMap(uint16_t adc, int32_t mn, int32_t mx) {
  if (mx < mn) mx = mn;
  int32_t span = mx - mn;
  return mn + (int32_t)(((int64_t)span * adc) / 1023);
}

static void recomputeRecAndRange() {
  rec_x100 = recoGetRecFlow_x100(S.material, S.cutter_mm, S.al_factor_x100);
  S.last_rec_x100 = rec_x100;

  potMin_x100 = (int32_t)((int64_t)rec_x100 * S.kmin_x100 / 100);
  potMax_x100 = (int32_t)((int64_t)rec_x100 * S.kmax_x100 / 100);
  if (potMax_x100 < potMin_x100 + 10) potMax_x100 = potMin_x100 + 10;

  set_x100 = potMap(potGetAvgAdc(), potMin_x100, potMax_x100);
}

static void startRun() {
  pulseOn = true;
  pulseMs = millis();
  digitalWrite(PIN_START_LED, HIGH);
  pumpSetEnable(true);
  state = ST_RUN;
  uiClear();
  uiDrawRun(S, rec_x100, set_x100, true);
}

static void stopRunToReady() {
  digitalWrite(PIN_START_LED, LOW);
  pumpStop();
  state = ST_READY;
  uiClear();
  uiDrawReady(S);
}

static void enterMenu() {
  // Safety: never keep pump running inside MENU
  if (state == ST_RUN || state == ST_CAL_RUN) {
    digitalWrite(PIN_START_LED, LOW);
    pumpStop();
  }
  state = ST_MENU;
  menuReset(menu);
  uiClear();
}

static void enterWizardSafe() {
  if (state == ST_RUN || state == ST_CAL_RUN) {
    digitalWrite(PIN_START_LED, LOW);
    pumpStop();
  }
  state = ST_WIZ_MAT;
  uiClear();
}

static void stopCalibrationPump() {
  digitalWrite(PIN_START_LED, LOW);
  pumpStop();
}

static void startCalibration(uint16_t sec) {
  calTotalSec = sec;
  calDurationMs = (uint32_t)sec * 1000UL;
  calStartMs = millis();

  calMeasuredMl_x100 = 0;
  calDigitIdx = 0;

  digitalWrite(PIN_START_LED, HIGH);
  pumpSetEnable(true);
  pumpRunCont(CAL_FLOW_U_X100, S.pump_gain_steps_per_u_min);

  state = ST_CAL_RUN;
  uiClear();
}

// Digit helpers for ml_x100 = TT*1000 + O*100 + t*10 + h
static uint8_t getDigit(int32_t ml_x100, uint8_t idx) {
  ml_x100 = clampI32(ml_x100, 0, 9999);
  switch (idx) {
    case 0: return (ml_x100 / 1000) % 10;
    case 1: return (ml_x100 / 100)  % 10;
    case 2: return (ml_x100 / 10)   % 10;
    default:return (ml_x100 / 1)    % 10;
  }
}

static int32_t setDigit(int32_t ml_x100, uint8_t idx, uint8_t digit) {
  ml_x100 = clampI32(ml_x100, 0, 9999);
  digit %= 10;

  int32_t tens = (ml_x100 / 1000) % 10;
  int32_t ones = (ml_x100 / 100)  % 10;
  int32_t tent = (ml_x100 / 10)   % 10;
  int32_t hund = (ml_x100 / 1)    % 10;

  if      (idx == 0) tens = digit;
  else if (idx == 1) ones = digit;
  else if (idx == 2) tent = digit;
  else               hund = digit;

  return clampI32(tens * 1000 + ones * 100 + tent * 10 + hund, 0, 9999);
}

static void saveCalibrationFromInput() {
  if (calMeasuredMl_x100 <= 0) return;

  uint32_t total_u_x100 = (uint32_t)calTotalSec * 100UL / 60UL;
  if (total_u_x100 == 0) return;

  uint32_t ml_per_u_x1000 = (uint32_t)calMeasuredMl_x100 * 1000UL / total_u_x100;
  if (ml_per_u_x1000 == 0) return;

  S.calibrated = true;
  S.ml_per_u_x1000 = ml_per_u_x1000;
  settingsSave();
}

// ---- helper: accelerated diameter steps only in ST_WIZ_DIA
static int8_t diaAccelStep(bool pressedNow, bool &prev, uint32_t &pressMs, uint32_t &lastRptMs, int8_t dir) {
  uint32_t now = millis();
  int8_t out = 0;

  if (pressedNow && !prev) { // press edge
    pressMs = now;
    lastRptMs = now;
    out = dir * 1; // first step always 1mm
  }

  if (pressedNow) {
    uint32_t held = now - pressMs;

    int8_t stepSize = 1;
    if (held >= 1500) stepSize = 10;
    else if (held >= 600) stepSize = 5;

    uint16_t interval;
    if      (held < 300)  interval = 180;
    else if (held < 900)  interval = 120;
    else if (held < 1600) interval = 80;
    else                  interval = 55;

    if ((now - lastRptMs) >= interval) {
      lastRptMs = now;
      out += (int8_t)(dir * stepSize);
    }
  }

  prev = pressedNow;
  return out;
}

void setup() {
  settingsLoad();
  uiBegin();
  inputBegin();
  potSetFilterN(S.pot_avg_N);

  pumpBegin();

  pinMode(PIN_START_LED, OUTPUT);
  digitalWrite(PIN_START_LED, LOW);

  (void)potGetAvgAdc();
  recomputeRecAndRange();
  uiDrawReady(S);

  // HARD START init
  startRawPrev = (digitalRead(PIN_START_BTN) == LOW);
  startRawLastMs = millis();

  // HARD DIA init
  upPrev = (digitalRead(PIN_BTN_UP) == LOW);
  dnPrev = (digitalRead(PIN_BTN_DOWN) == LOW);
  upPressMs = dnPressMs = millis();
  upLastRptMs = dnLastRptMs = millis();
}

void loop() {
  static uint32_t tPoll = 0;
  static uint32_t tUi = 0;
  static uint8_t lastPotN = 0;

  if (S.pot_avg_N != lastPotN) {
    lastPotN = S.pot_avg_N;
    potSetFilterN(S.pot_avg_N);
  }

  if (millis() - tPoll >= INPUT_POLL_MS) {
    tPoll = millis();

    InputEvents ev;
    inputPoll(ev);

    // HARD START override (direct pin)
    {
      bool sNow = (digitalRead(PIN_START_BTN) == LOW);
      if (sNow && !startRawPrev) {
        if ((uint16_t)(millis() - startRawLastMs) >= 60) {
          ev.startClick = true;
          startRawLastMs = millis();
        }
      }
      startRawPrev = sNow;
    }

    // LONG OK -> Wizard safely
    if (ev.encLong) {
      enterWizardSafe();
    }

    // HARD DIA accel override (only in ST_WIZ_DIA)
    if (state == ST_WIZ_DIA) {
      // ignore ev.encStep from input.cpp here
      ev.encStep = 0;

      bool upNow = (digitalRead(PIN_BTN_UP) == LOW);
      bool dnNow = (digitalRead(PIN_BTN_DOWN) == LOW);

      ev.encStep += diaAccelStep(upNow, upPrev, upPressMs, upLastRptMs, +1);
      ev.encStep += diaAccelStep(dnNow, dnPrev, dnPressMs, dnLastRptMs, -1);
    } else {
      // keep prev states updated so first accel press is correct when enter ST_WIZ_DIA
      upPrev = (digitalRead(PIN_BTN_UP) == LOW);
      dnPrev = (digitalRead(PIN_BTN_DOWN) == LOW);
    }

    // POT only in WIZ_REC / RUN
    if (state == ST_WIZ_REC || state == ST_RUN) {
      int32_t newSet = potMap(potGetAvgAdc(), potMin_x100, potMax_x100);
      int32_t diff = newSet - set_x100; if (diff < 0) diff = -diff;
      if (diff >= (int32_t)S.pot_hyst_x100) set_x100 = newSet;
    }

    // UP/DOWN
    if (ev.encStep != 0) {
      if (state == ST_WIZ_MAT) {
        S.material = (S.material == MAT_STEEL) ? MAT_ALUMINUM : MAT_STEEL;
        recomputeRecAndRange();
      } else if (state == ST_WIZ_DIA) {
        S.cutter_mm = (uint8_t)clampI32((int32_t)S.cutter_mm + ev.encStep, 3, 50);
        recomputeRecAndRange();
      } else if (state == ST_MENU) {
        MenuAction act = menuOnDelta(menu, ev.encStep, S);
        if (act == MENU_ACT_RECOMPUTE) recomputeRecAndRange();
      } else if (state == ST_CAL_INPUT) {
        uint8_t d = getDigit(calMeasuredMl_x100, calDigitIdx);
        if (ev.encStep > 0) d = (uint8_t)((d + 1) % 10);
        else                d = (uint8_t)((d + 9) % 10);
        calMeasuredMl_x100 = setDigit(calMeasuredMl_x100, calDigitIdx, d);
      }
    }

    // OK short
    if (ev.encClick) {
      if (state == ST_WIZ_MAT) {
        state = ST_WIZ_DIA;
        uiClear();
      } else if (state == ST_WIZ_DIA) {
        recomputeRecAndRange();
        state = ST_WIZ_REC;
        uiClear();
      } else if (state == ST_MENU) {
        MenuAction act = menuOnClick(menu, S);

        if (act == MENU_ACT_SAVE) {
          settingsSave();
        } else if (act == MENU_ACT_DEFAULTS) {
          settingsLoadDefaults();
          settingsSave();
          potSetFilterN(S.pot_avg_N);
          recomputeRecAndRange();
        } else if (act == MENU_ACT_RECOMPUTE) {
          recomputeRecAndRange();
        } else if (act == MENU_ACT_CAL_START_60) {
          startCalibration(60);
        } else if (act == MENU_ACT_CAL_START_120) {
          startCalibration(120);
        } else if (act == MENU_ACT_CAL_CLEAR) {
          S.calibrated = false;
          S.ml_per_u_x1000 = 0;
          settingsSave();
        }
      } else if (state == ST_CAL_INPUT) {
        if (calDigitIdx < 3) calDigitIdx++;
        else {
          saveCalibrationFromInput();
          state = ST_MENU;
          menuReset(menu);
          uiClear();
        }
      }
    }

    // MENU/BACK short
    if (ev.menuClick) {
      if (state == ST_READY) {
        enterMenu();
      } else if (state == ST_WIZ_REC || state == ST_RUN) {
        enterMenu();
      } else if (state == ST_MENU) {
        state = ST_READY;
        uiClear();
      } else if (state == ST_CAL_RUN) {
        stopCalibrationPump();
        state = ST_MENU;
        menuReset(menu);
        uiClear();
      } else if (state == ST_CAL_INPUT) {
        state = ST_MENU;
        menuReset(menu);
        uiClear();
      }
    }

    // START/STOP
    if (ev.startClick) {
      if (state == ST_READY) {
        if (!wizardDone) {
          state = ST_WIZ_MAT;
          uiClear();
        } else {
          recomputeRecAndRange();
          startRun();
        }
      } else if (state == ST_WIZ_MAT || state == ST_WIZ_DIA) {
        state = ST_READY;
        uiClear();
      } else if (state == ST_WIZ_REC) {
        wizardDone = true;
        startRun();
      } else if (state == ST_RUN) {
        stopRunToReady();
      } else if (state == ST_MENU) {
        state = ST_READY;
        uiClear();
      } else if (state == ST_CAL_RUN) {
        stopCalibrationPump();
        state = ST_MENU;
        menuReset(menu);
        uiClear();
      } else if (state == ST_CAL_INPUT) {
        state = ST_MENU;
        menuReset(menu);
        uiClear();
      }
    }
  }

  // Runtime (pump)
  if (state == ST_RUN) {
    if (S.mode == MODE_CONT) pumpRunCont(set_x100, S.pump_gain_steps_per_u_min);
    else pumpRunPulse(pulseOn, pulseMs, S, set_x100);
  } else if (state == ST_CAL_RUN) {
    pumpRunCont(CAL_FLOW_U_X100, S.pump_gain_steps_per_u_min);

    if ((millis() - calStartMs) >= calDurationMs) {
      stopCalibrationPump();
      state = ST_CAL_INPUT;
      uiClear();
    }
  }

  // UI refresh
  if (millis() - tUi >= UI_REFRESH_MS) {
    tUi = millis();

    switch (state) {
      case ST_READY:    uiDrawReady(S); break;
      case ST_WIZ_MAT:  uiDrawWizMaterial(S); break;
      case ST_WIZ_DIA:  uiDrawWizDiameter(S); break;
      case ST_WIZ_REC:  uiDrawWizRecommend(S, rec_x100, set_x100, potMin_x100, potMax_x100); break;
      case ST_RUN:      uiDrawRun(S, rec_x100, set_x100, true); break;

      case ST_MENU: {
        char l1[21], l2[21], l3[21];
        menuRender3(menu, S, l1, l2, l3);
        uiDrawMenu(menu.editing, l1, l2, l3);
      } break;

      case ST_CAL_RUN: {
        uint32_t elapsed = millis() - calStartMs;
        uint16_t left = (elapsed >= calDurationMs) ? 0 : (uint16_t)((calDurationMs - elapsed) / 1000UL);
        uiDrawCalRun(calTotalSec, left);
      } break;

      case ST_CAL_INPUT:
        uiDrawCalInputDigits(calMeasuredMl_x100, calDigitIdx);
        break;

      default: break;
    }
  }
}
