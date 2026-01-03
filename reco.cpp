#include "reco.h"

// Baseline recommendation in abstract "u/min" (x100).
// After calibration it will be shown as ml/min automatically.
static int32_t recFlowSteelBase_x100(uint8_t dia_mm) {
  if (dia_mm <= 6)  return 35;   // 0.35
  if (dia_mm <= 12) return 55;   // 0.55
  if (dia_mm <= 25) return 90;   // 0.90
  return 140;                    // 1.40
}

int32_t recoGetRecFlow_x100(Material mat, uint8_t dia_mm, uint16_t al_factor_x100) {
  int32_t steel = recFlowSteelBase_x100(dia_mm);
  if (mat == MAT_STEEL) return steel;

  int32_t al = (int32_t)((int64_t)steel * (int64_t)al_factor_x100 / 100);
  if (al < 1) al = 1;
  return al;
}
