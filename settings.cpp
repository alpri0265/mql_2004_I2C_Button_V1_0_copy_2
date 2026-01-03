#include <EEPROM.h>
#include "settings.h"

Settings S;
static constexpr uint32_t SETTINGS_MAGIC = 0x4D514C31UL; // "MQL1"

void settingsLoadDefaults() {
  S.magic = SETTINGS_MAGIC;

  S.material = MAT_STEEL;
  S.cutter_mm = 10;

  S.mode = MODE_CONT;
  S.pulse_on_ms = 500;
  S.pulse_off_ms = 2000;

  S.kmin_x100 = 50;       // 0.50x
  S.kmax_x100 = 200;      // 2.00x
  S.al_factor_x100 = 130; // Al = 1.30x Steel

  S.pot_avg_N = 8;
  S.pot_hyst_x100 = 2;    // 0.02 u/min hysteresis (in u units)

  S.pump_gain_steps_per_u_min = 1000;
  S.steps_per_rev = 3200;

  S.calibrated = false;
  S.ml_per_u_x1000 = 0;

  S.last_rec_x100 = 55;
}

void settingsLoad() {
  EEPROM.get(0, S);
  if (S.magic != SETTINGS_MAGIC) {
    settingsLoadDefaults();
    settingsSave();
  }
}

void settingsSave() {
  S.magic = SETTINGS_MAGIC;
  EEPROM.put(0, S);
}
