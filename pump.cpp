#include <Arduino.h>
#include "config.h"
#include "pump.h"

static volatile bool stepEnable = false;

// --- Timer1: CTC, частота задаётся через OCR1A + прескалер
static void timer1Init() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;

  // CTC mode
  TCCR1B |= (1 << WGM12);

  // Start disabled
  TIMSK1 &= ~(1 << OCIE1A);

  // Default OCR
  OCR1A = 2000;

  sei();
}

static void setPrescalerBits(uint16_t presc) {
  // Clear CS12..CS10
  TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));

  switch (presc) {
    case 1:    TCCR1B |= (1 << CS10); break;
    case 8:    TCCR1B |= (1 << CS11); break;
    case 64:   TCCR1B |= (1 << CS11) | (1 << CS10); break;
    case 256:  TCCR1B |= (1 << CS12); break;
    case 1024: TCCR1B |= (1 << CS12) | (1 << CS10); break;
    default:   TCCR1B |= (1 << CS11); break; // 8
  }
}

// Подбор прескалера чтобы OCR1A влезал в 16 бит (0..65535)
static void pumpSetRateHz(uint16_t hz) {
  if (hz < 1) hz = 1;
  if (hz > 2000) hz = 2000; // безопасный максимум для ISR

  const uint16_t prescList[] = {1, 8, 64, 256, 1024};
  uint16_t bestPresc = 1024;
  uint32_t bestOcr = 65535;

  for (uint8_t i = 0; i < sizeof(prescList) / sizeof(prescList[0]); i++) {
    uint16_t p = prescList[i];
    uint32_t ocr = (F_CPU / (uint32_t)p / (uint32_t)hz) - 1UL;
    if (ocr <= 65535UL) {
      bestPresc = p;
      bestOcr = ocr;
      break; // берём самый “быстрый” прескалер, который влезает
    }
  }

  uint8_t sreg = SREG;
  cli();
  setPrescalerBits(bestPresc);
  OCR1A = (uint16_t)bestOcr;
  SREG = sreg;
}

ISR(TIMER1_COMPA_vect) {
  if (!stepEnable) return;

  digitalWrite(PIN_STEP, HIGH);
  delayMicroseconds(4);
  digitalWrite(PIN_STEP, LOW);
}

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

  if (en) TIMSK1 |= (1 << OCIE1A);
  else    TIMSK1 &= ~(1 << OCIE1A);

  // ENA у тебя аппаратно не используется, но оставим как было
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

void pumpRunCont(int32_t flow_x100, uint32_t pumpGain) {
  if (flow_x100 <= 0 || pumpGain == 0) {
    pumpStop();
    return;
  }

  // steps/min = flow * gain
  uint64_t stepsPerMin = ((uint64_t)flow_x100 * (uint64_t)pumpGain) / 100ULL;
  if (stepsPerMin == 0) {
    pumpStop();
    return;
  }

  // steps/sec (округление вверх)
  uint32_t hz = (uint32_t)((stepsPerMin + 59ULL) / 60ULL);
  if (hz < 1) hz = 1;
  if (hz > 2000) hz = 2000;

  pumpSetRateHz((uint16_t)hz);
  pumpSetEnable(true);
}

void pumpRunPulse(bool &phaseOn,
                  uint32_t &phaseStartMs,
                  const Settings &S,
                  int32_t flow_x100) {
  (void)phaseOn;
  (void)phaseStartMs;
  (void)S;
  (void)flow_x100;
}
