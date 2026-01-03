#include <Arduino.h>
#include "config.h"
#include "pump.h"

static volatile bool stepEnable = false;

// ===== Timer1 init =====
static void timer1Init() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;

  // CTC mode
  TCCR1B |= (1 << WGM12);

  // prescaler = 8
  TCCR1B |= (1 << CS11);

  OCR1A = 2000;

  // start disabled
  TIMSK1 &= ~(1 << OCIE1A);

  sei();
}

// ===== Set step frequency (Hz) =====
static void pumpSetRateHz(uint16_t hz) {
  if (hz < 1) hz = 1;
  if (hz > 2000) hz = 2000;

  uint32_t ocr = (F_CPU / 8UL / hz) - 1;
  OCR1A = (uint16_t)ocr;
}

// ===== STEP ISR =====
ISR(TIMER1_COMPA_vect) {
  if (!stepEnable) return;

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
  digitalWrite(PIN_DIR, HIGH);
  digitalWrite(PIN_ENA, LOW);

  timer1Init();
}

void pumpSetEnable(bool en) {
  stepEnable = en;
  if (en) TIMSK1 |= (1 << OCIE1A);
  else    TIMSK1 &= ~(1 << OCIE1A);

  // ENA не используется по твоей схеме, но оставим как было:
  digitalWrite(PIN_ENA, en ? HIGH : LOW);
}

void pumpStartSteps(uint32_t stepsPerSec) {
  if (stepsPerSec == 0) {
    pumpStop();
    return;
  }
  if (stepsPerSec > 2000) stepsPerSec = 2000;
  pumpSetRateHz((uint16_t)stepsPerSec);
  pumpSetEnable(true);
}

void pumpStop() {
  stepEnable = false;
  TIMSK1 &= ~(1 << OCIE1A);
  digitalWrite(PIN_ENA, LOW);
}

// ===== Continuous mode =====
void pumpRunCont(int32_t flow_x100, uint32_t pumpGain) {
  if (flow_x100 <= 0 || pumpGain == 0) {
    pumpStop();
    return;
  }

  uint64_t stepsPerMin = ((uint64_t)flow_x100 * (uint64_t)pumpGain) / 100ULL;
  if (stepsPerMin == 0) {
    pumpStop();
    return;
  }

  uint32_t hz = (uint32_t)((stepsPerMin + 59) / 60);
  if (hz < 1) hz = 1;
  if (hz > 2000) hz = 2000;

  pumpSetRateHz((uint16_t)hz);
  pumpSetEnable(true);
}

// ===== Pulse mode (пока заглушка) =====
void pumpRunPulse(bool &phaseOn,
                  uint32_t &phaseStartMs,
                  const Settings &S,
                  int32_t flow_x100) {
  (void)phaseOn;
  (void)phaseStartMs;
  (void)S;
  (void)flow_x100;
}
