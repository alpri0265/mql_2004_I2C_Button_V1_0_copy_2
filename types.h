#pragma once
#include <Arduino.h>

enum Material : uint8_t { MAT_STEEL = 0, MAT_ALUMINUM = 1 };
enum Mode     : uint8_t { MODE_CONT = 0, MODE_PULSE   = 1 };

enum AppState : uint8_t {
  ST_READY = 0,
  ST_WIZ_MAT,
  ST_WIZ_DIA,
  ST_WIZ_REC,
  ST_RUN,
  ST_MENU,
  ST_CAL_RUN,
  ST_CAL_INPUT
};

struct Settings {
  uint32_t magic;

  Material material;
  uint8_t cutter_mm;      // 3..50

  Mode mode;
  uint16_t pulse_on_ms;   // 100..5000
  uint16_t pulse_off_ms;  // 100..10000

  uint16_t kmin_x100;      // 0.20..1.00 -> 20..100
  uint16_t kmax_x100;      // 1.20..4.00 -> 120..400
  uint16_t al_factor_x100; // 1.00..2.00 -> 100..200

  uint8_t pot_avg_N;      // 4/8/16
  uint8_t pot_hyst_x100;  // hysteresis in x100 u/min

  uint32_t pump_gain_steps_per_u_min; // tune
  uint16_t steps_per_rev; // reference

  // Calibration
  bool     calibrated;
  uint32_t ml_per_u_x1000; // ml per 1.00 u (x1000)

  int32_t last_rec_x100;
};
