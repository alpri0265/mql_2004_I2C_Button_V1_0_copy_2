#include <Arduino.h>
#include "config.h"
#include "pump.h"

static volatile bool stepEnable = false;
static volatile bool stepState = false;

// ===== Timer1 init =====
static void timer1Init() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;

  // CTC mode
  TCCR1B |= (1 << WGM12);

  // prescaler = 8
  TCCR1B |= (1 << CS11);

  OCR1A = 2000; // default, will be changed

  TIMSK1 |= (1 << OCIE1A);
  sei();
}

// ===== Set step frequency (Hz) =====
static void pumpSetRateHz(uint16_t hz) {
  if (hz < 1) hz = 1;
  if (hz > 2000) hz = 2000; // безопасный максимум

  uint32_t ocr = (F_CPU / 8UL / hz) - 1;
  OCR1A = (uint16_t)ocr;
}

// ===== STEP ISR =====
ISR(TIMER1_COMPA_vect) {
  //if (!stepEnable) return;

  // правильный STEP импульс для DM556
  digitalWrite(PIN_STEP, HIGH);
  delayMicroseconds(4);
  digitalWrite(PIN_STEP, LOW);
}

// ===== Public API =====
void pumpBegin() {
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENA, OUTPUT);

  digitalWrite(PIN_STEP, LOW);
  digitalWrite(PIN_DIR, HIGH);   // направление любое
  digitalWrite(PIN_ENA, LOW);    // disabled initially

  timer1Init();
}

void pumpSetEnable(bool en) {
  stepEnable = en;
  digitalWrite(PIN_ENA, en ? HIGH : LOW);
}

void pumpStop() {
  stepEnable = false;
  digitalWrite(PIN_ENA, LOW);
}

// ===== Continuous mode =====
// flow_x100: 1.00 u/min = 100
void pumpRunCont(int32_t flow_x100, uint32_t pumpGain) {
  if (flow_x100 <= 0 || pumpGain == 0) {
    pumpStop();
    return;
  }

  // steps/min = flow * gain
  uint64_t stepsPerMin =
    ((uint64_t)flow_x100 * (uint64_t)pumpGain) / 100ULL;

  if (stepsPerMin == 0) {
    pumpStop();
    return;
  }

  // steps/sec (округление вверх)
  uint32_t hz = (stepsPerMin + 59) / 60;
  if (hz < 1) hz = 1;

  pumpSetRateHz((uint16_t)hz);
  stepEnable = true;
}

// ===== Pulse mode (заглушка, если нужен) =====
void pumpRunPulse(bool &pulseOn,
                  uint32_t &pulseMs,
                  const Settings &S,
                  int32_t flow_x100) {
  // Пока не используем — можно дописать позже
}
