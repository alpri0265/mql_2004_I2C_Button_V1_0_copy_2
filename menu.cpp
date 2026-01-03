#include <Arduino.h>
#include "menu.h"

// Items:
// 0 Material
// 1 Cutter Ø
// 2 Mode
// 3 Pulse ON
// 4 Pulse OFF
// 5 Kmin
// 6 Kmax
// 7 AlFactor
// 8 POT Avg N
// 9 POT Hyst
// 10 Pump Gain
// 11 Calibrate 60s (action)
// 12 Calibrate 120s (action)
// 13 Cal ml/u (read-only)
// 14 Clear Cal (action)
// 15 Save EEPROM (action)
// 16 Load Defaults (action)
static constexpr uint8_t ITEM_COUNT = 17;

static int32_t clampI32(int32_t v, int32_t lo, int32_t hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

void menuReset(MenuState &m) {
  m.index = 0;
  m.editing = false;
}

static void fmtPadded(char out[21], const char* s) {
  uint8_t i = 0;
  for (; i < 20 && s[i]; i++) out[i] = s[i];
  for (; i < 20; i++) out[i] = ' ';
  out[20] = '\0';
}

static void makeItemLine(char out[21], uint8_t idx, bool selected, const Settings &S) {
  char b[44];

  switch (idx) {
    case 0:
      snprintf(b, sizeof(b), "%cMaterial: %s", selected ? '>' : ' ',
               (S.material == MAT_STEEL) ? "Steel" : "Aluminum");
      break;
    case 1:
      snprintf(b, sizeof(b), "%cCutter O: %umm", selected ? '>' : ' ', (unsigned)S.cutter_mm);
      break;
    case 2:
      snprintf(b, sizeof(b), "%cMode: %s", selected ? '>' : ' ', (S.mode == MODE_CONT) ? "CONT" : "PULSE");
      break;
    case 3:
      snprintf(b, sizeof(b), "%cPulse ON: %ums", selected ? '>' : ' ', (unsigned)S.pulse_on_ms);
      break;
    case 4:
      snprintf(b, sizeof(b), "%cPulse OFF:%ums", selected ? '>' : ' ', (unsigned)S.pulse_off_ms);
      break;
    case 5: {
  uint16_t v = S.kmin_x100;
  snprintf(b, sizeof(b), "%cKmin: %u.%02u", selected ? '>' : ' ',
           (unsigned)(v / 100), (unsigned)(v % 100));
} break;
case 6: {
  uint16_t v = S.kmax_x100;
  snprintf(b, sizeof(b), "%cKmax: %u.%02u", selected ? '>' : ' ',
           (unsigned)(v / 100), (unsigned)(v % 100));
} break;
case 7: {
  uint16_t v = S.al_factor_x100;
  snprintf(b, sizeof(b), "%cAlFactor: %u.%02u", selected ? '>' : ' ',
           (unsigned)(v / 100), (unsigned)(v % 100));
} break;

    case 8:
      snprintf(b, sizeof(b), "%cPOT Avg N: %u", selected ? '>' : ' ', (unsigned)S.pot_avg_N);
      break;
    case 9: {
  uint16_t v = S.pot_hyst_x100;
  snprintf(b, sizeof(b), "%cPOT Hyst: %u.%02u", selected ? '>' : ' ',
           (unsigned)(v / 100), (unsigned)(v % 100));
} break;

    case 10:
      snprintf(b, sizeof(b), "%cPumpGain: %lu", selected ? '>' : ' ', (unsigned long)S.pump_gain_steps_per_u_min);
      break;

    case 11:
      snprintf(b, sizeof(b), "%cCalibrate 60s", selected ? '>' : ' ');
      break;

    case 12:
      snprintf(b, sizeof(b), "%cCalibrate 120s", selected ? '>' : ' ');
      break;

    case 13: {
      if (!S.calibrated) {
        snprintf(b, sizeof(b), "%cCal ml/u: (none)", selected ? '>' : ' ');
      } else {
        uint32_t x = S.ml_per_u_x1000;
        uint32_t w = x / 1000;
        uint32_t f = (x % 1000) / 10; // 2 decimals
        snprintf(b, sizeof(b), "%cCal ml/u: %lu.%02lu", selected ? '>' : ' ',
                 (unsigned long)w, (unsigned long)f);
      }
    } break;

    case 14:
      snprintf(b, sizeof(b), "%cClear calibration", selected ? '>' : ' ');
      break;

    case 15:
      snprintf(b, sizeof(b), "%cSave EEPROM", selected ? '>' : ' ');
      break;

    case 16:
      snprintf(b, sizeof(b), "%cLoad Defaults", selected ? '>' : ' ');
      break;

    default:
      snprintf(b, sizeof(b), "%c-", selected ? '>' : ' ');
      break;
  }

  fmtPadded(out, b);
}

MenuAction menuOnDelta(MenuState &m, int8_t step, Settings &S) {
  if (step == 0) return MENU_ACT_NONE;

  if (!m.editing) {
    int16_t ni = (int16_t)m.index + step;
    if (ni < 0) ni = 0;
    if (ni >= ITEM_COUNT) ni = ITEM_COUNT - 1;
    m.index = (uint8_t)ni;
    return MENU_ACT_NONE;
  }

  switch (m.index) {
    case 0:
      S.material = (step > 0) ? MAT_ALUMINUM : MAT_STEEL;
      return MENU_ACT_RECOMPUTE;

    case 1:
      S.cutter_mm = (uint8_t)clampI32((int32_t)S.cutter_mm + step, 3, 50);
      return MENU_ACT_RECOMPUTE;

    case 2:
      S.mode = (S.mode == MODE_CONT) ? MODE_PULSE : MODE_CONT;
      return MENU_ACT_NONE;

    case 3:
      S.pulse_on_ms = (uint16_t)clampI32((int32_t)S.pulse_on_ms + step * 50, 100, 5000);
      return MENU_ACT_NONE;

    case 4:
      S.pulse_off_ms = (uint16_t)clampI32((int32_t)S.pulse_off_ms + step * 100, 100, 10000);
      return MENU_ACT_NONE;

    case 5:
      S.kmin_x100 = (uint16_t)clampI32((int32_t)S.kmin_x100 + step * 2, 20, 100);
      return MENU_ACT_RECOMPUTE;

    case 6:
      S.kmax_x100 = (uint16_t)clampI32((int32_t)S.kmax_x100 + step * 5, 120, 400);
      return MENU_ACT_RECOMPUTE;

    case 7:
      S.al_factor_x100 = (uint16_t)clampI32((int32_t)S.al_factor_x100 + step * 2, 100, 200);
      return MENU_ACT_RECOMPUTE;

    case 8: {
      uint8_t n = S.pot_avg_N;
      if (step > 0) { if (n == 4) n = 8; else if (n == 8) n = 16; }
      else          { if (n == 16) n = 8; else if (n == 8) n = 4; }
      S.pot_avg_N = n;
      return MENU_ACT_NONE;
    }

    case 9:
      S.pot_hyst_x100 = (uint8_t)clampI32((int32_t)S.pot_hyst_x100 + step, 0, 50);
      return MENU_ACT_NONE;

    case 10: {
      int32_t g = (int32_t)S.pump_gain_steps_per_u_min;
      g += step * 50;
      g = clampI32(g, 50, 50000);
      S.pump_gain_steps_per_u_min = (uint32_t)g;
      return MENU_ACT_NONE;
    }

    default:
      return MENU_ACT_NONE;
  }
}

MenuAction menuOnClick(MenuState &m, Settings &S) {
  if (!m.editing) {
    if (m.index == 11) return MENU_ACT_CAL_START_60;
    if (m.index == 12) return MENU_ACT_CAL_START_120;
    if (m.index == 14) return MENU_ACT_CAL_CLEAR;
    if (m.index == 15) return MENU_ACT_SAVE;
    if (m.index == 16) return MENU_ACT_DEFAULTS;

    if (m.index == 13) return MENU_ACT_NONE; // read-only

    m.editing = true;
    return MENU_ACT_NONE;
  } else {
    m.editing = false;
    return MENU_ACT_NONE;
  }
}

void menuRender3(const MenuState &m, const Settings &S, char line1[21], char line2[21], char line3[21]) {
  uint8_t top = 0;
  if (m.index >= 2) top = m.index - 2;
  if (top > ITEM_COUNT - 3) top = ITEM_COUNT - 3;

  makeItemLine(line1, top + 0, (m.index == top + 0), S);
  makeItemLine(line2, top + 1, (m.index == top + 1), S);
  makeItemLine(line3, top + 2, (m.index == top + 2), S);
}
