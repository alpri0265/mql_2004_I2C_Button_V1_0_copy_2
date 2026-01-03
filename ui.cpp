#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "ui.h"

static LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 20, 4);

static void printLine(uint8_t row, const char *text) {
  lcd.setCursor(0, row);
  for (uint8_t i = 0; i < 20; i++) {
    char c = text[i];
    if (c == '\0') {
      for (; i < 20; i++) lcd.print(' ');
      break;
    }
    lcd.print(c);
  }
}

// Convert u/min (x100) -> ml/min (x100) using ml_per_u_x1000 (ml per 1.00 u)
static int32_t uToMl_x100(int32_t u_x100, uint32_t ml_per_u_x1000) {
  if (u_x100 <= 0) return 0;
  return (int32_t)(((int64_t)u_x100 * (int64_t)ml_per_u_x1000) / 1000);
}

static void fmtFlow(char* out, size_t sz, int32_t u_x100, const Settings &S, bool showUnits = true, bool shortNoUnits = false) {
  bool useMl = S.calibrated && (S.ml_per_u_x1000 > 0);

  int32_t val_x100 = u_x100;
  const char* unit = "u/min";
  if (useMl) {
    val_x100 = uToMl_x100(u_x100, S.ml_per_u_x1000);
    unit = "ml/min";
  }

  if (val_x100 < 0) val_x100 = 0;
  long w = val_x100 / 100;
  long f = val_x100 % 100;

  if (shortNoUnits) snprintf(out, sz, "%ld.%02ld", w, f);
  else if (showUnits) snprintf(out, sz, "%ld.%02ld %s", w, f, unit);
  else snprintf(out, sz, "%ld.%02ld", w, f);
}

void uiBegin() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void uiClear() { lcd.clear(); }

void uiDrawReady(const Settings &S) {
  printLine(0, "MQL READY");
  printLine(1, "START: Wizard");
  printLine(2, "ENC: Menu");
  char line[21];
  snprintf(line, sizeof(line), "Last: %s O%umm",
           (S.material == MAT_STEEL) ? "Steel" : "Al",
           (unsigned)S.cutter_mm);
  printLine(3, line);
}

void uiDrawWizMaterial(const Settings &S) {
  printLine(0, "Select material");
  printLine(1, (S.material == MAT_STEEL) ? "> Steel" : "  Steel");
  printLine(2, (S.material == MAT_ALUMINUM) ? "> Aluminum" : "  Aluminum");
  printLine(3, "ENC=Sel OK=Click");
}

void uiDrawWizDiameter(const Settings &S) {
  printLine(0, "Cutter diameter");
  char line[21];
  snprintf(line, sizeof(line), "O: %umm", (unsigned)S.cutter_mm);
  printLine(1, line);
  printLine(2, "Range: 3..50");
  printLine(3, "Turn ENC, Click=OK");
}

void uiDrawWizRecommend(const Settings &S, int32_t rec_u_x100, int32_t set_u_x100, int32_t potMin_u_x100, int32_t potMax_u_x100) {
  char l0[21], l1[21], l2[21], l3[21];

  snprintf(l0, sizeof(l0), "%s O%umm  MQL",
           (S.material == MAT_STEEL) ? "Steel" : "Al",
           (unsigned)S.cutter_mm);

  char bRec[16], bSet[16], bMin[8], bMax[8];
  fmtFlow(bRec, sizeof(bRec), rec_u_x100, S, true);
  fmtFlow(bSet, sizeof(bSet), set_u_x100, S, true);
  fmtFlow(bMin, sizeof(bMin), potMin_u_x100, S, false, true);
  fmtFlow(bMax, sizeof(bMax), potMax_u_x100, S, false, true);

  snprintf(l1, sizeof(l1), "Rec: %s", bRec);
  snprintf(l2, sizeof(l2), "Set: %s", bSet);
  snprintf(l3, sizeof(l3), "POT %s..%s START", bMin, bMax);

  printLine(0, l0);
  printLine(1, l1);
  printLine(2, l2);
  printLine(3, l3);
}

void uiDrawRun(const Settings &S, int32_t rec_u_x100, int32_t set_u_x100, bool running) {
  char l0[21], l1[21], l2[21];

  snprintf(l0, sizeof(l0), "%s %s O%u %s",
           running ? "RUN" : "STOP",
           (S.material == MAT_STEEL) ? "Steel" : "Al",
           (unsigned)S.cutter_mm,
           (S.mode == MODE_CONT) ? "CONT" : "PULSE");

  char bRec[10], bSet[10], bFlow[16];
  fmtFlow(bRec, sizeof(bRec), rec_u_x100, S, false);
  fmtFlow(bSet, sizeof(bSet), set_u_x100, S, false);
  fmtFlow(bFlow, sizeof(bFlow), set_u_x100, S, true);

  snprintf(l1, sizeof(l1), "Rec %s Set %s", bRec, bSet);
  snprintf(l2, sizeof(l2), "Flow: %s", bFlow);

  printLine(0, l0);
  printLine(1, l1);
  printLine(2, l2);
  printLine(3, "STOP=BTN  ENC=Menu");
}

void uiDrawMenu(bool editing, const char line1[21], const char line2[21], const char line3[21]) {
  printLine(0, editing ? "MENU (edit)" : "MENU");
  printLine(1, line1);
  printLine(2, line2);
  printLine(3, line3);
}

void uiDrawCalRun(uint16_t totalSec, uint16_t secondsLeft) {
  char l1[21], l3[21];
  printLine(0, "CALIBRATION");
  snprintf(l1, sizeof(l1), "Duration: %us", (unsigned)totalSec);
  printLine(1, l1);
  printLine(2, "Measure output (ml)");
  snprintf(l3, sizeof(l3), "Running... %us", (unsigned)secondsLeft);
  printLine(3, l3);
}

void uiDrawCalInputDigits(int32_t ml_x100, uint8_t digitIdx) {
  if (ml_x100 < 0) ml_x100 = 0;
  if (ml_x100 > 9999) ml_x100 = 9999;
  if (digitIdx > 3) digitIdx = 0;

  uint8_t tens = (ml_x100 / 1000) % 10;
  uint8_t ones = (ml_x100 / 100)  % 10;
  uint8_t tent = (ml_x100 / 10)   % 10;
  uint8_t hund = (ml_x100 / 1)    % 10;

  char l2[21];
  snprintf(l2, sizeof(l2), "Measured: %u%u.%u%u ml", tens, ones, tent, hund);

  char cur[21];
  for (uint8_t i = 0; i < 20; i++) cur[i] = ' ';
  cur[20] = '\0';

  uint8_t col = 10;
  if      (digitIdx == 0) col = 10;
  else if (digitIdx == 1) col = 11;
  else if (digitIdx == 2) col = 13;
  else                    col = 14;
  if (col < 20) cur[col] = '^';

  printLine(0, "CAL: Enter ml");
  printLine(1, "ENC=chg OK=next/save");
  printLine(2, l2);
  printLine(3, cur);
}
