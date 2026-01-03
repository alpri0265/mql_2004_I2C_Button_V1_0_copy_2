#include "config.h"
#include "input.h"

static inline bool pressed(uint8_t pin) { return digitalRead(pin) == LOW; }

// OK long
static bool prevOk = false;
static bool okLongFired = false;
static uint32_t okPressMs = 0;
static const uint16_t OK_LONG_MS = 700;

// MENU short
static bool prevMenu = false;

// UP/DOWN accel
static bool prevUp = false;
static bool prevDown = false;
static uint32_t upPressMs = 0, upLastRptMs = 0;
static uint32_t dnPressMs = 0, dnLastRptMs = 0;

static int8_t holdRepeat(uint32_t now, uint32_t pressMs, uint32_t &lastRptMs) {
  uint32_t held = now - pressMs;

  uint16_t interval;
  if      (held < 400)  interval = 180;
  else if (held < 900)  interval = 120;
  else if (held < 1600) interval = 70;
  else                  interval = 45;

  if ((now - lastRptMs) < interval) return 0;
  lastRptMs = now;
  return 1;
}

// ===== POT filter =====
static uint16_t potBuf[16];
static uint8_t potN = 8;
static uint8_t potIdx = 0;
static bool potPrimed = false;
static uint16_t potAvg = 0;

void potSetFilterN(uint8_t N) {
  if (N != 4 && N != 8 && N != 16) N = 8;
  potN = N;
  potIdx = 0;
  potPrimed = false;
  potAvg = 0;
}

static void potUpdate() {
  uint16_t v = analogRead(PIN_POT);

  if (!potPrimed) {
    for (uint8_t i = 0; i < potN; i++) potBuf[i] = v;
    potPrimed = true;
    potIdx = 0;
    potAvg = v;
    return;
  }

  potBuf[potIdx] = v;
  potIdx = (potIdx + 1) % potN;

  uint32_t sum = 0;
  for (uint8_t i = 0; i < potN; i++) sum += potBuf[i];
  potAvg = (uint16_t)(sum / potN);
}

uint16_t potGetAvgAdc() { return potAvg; }

void inputBegin() {
  pinMode(PIN_START_BTN, INPUT_PULLUP);

  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_OK,   INPUT_PULLUP);
  pinMode(PIN_BTN_MENU, INPUT_PULLUP);

  pinMode(PIN_POT, INPUT);

  prevOk   = pressed(PIN_BTN_OK);
  prevMenu = pressed(PIN_BTN_MENU);
  prevUp   = pressed(PIN_BTN_UP);
  prevDown = pressed(PIN_BTN_DOWN);

  okLongFired = false;
  okPressMs = millis();

  potSetFilterN(8);
  potUpdate();
}

void inputPoll(InputEvents &ev) {
  ev = {};
  uint32_t now = millis();

  bool o = pressed(PIN_BTN_OK);
  bool m = pressed(PIN_BTN_MENU);
  bool u = pressed(PIN_BTN_UP);
  bool d = pressed(PIN_BTN_DOWN);

  // MENU short
  if (m && !prevMenu) ev.menuClick = true;

  // UP/DOWN + accel
  if (u && !prevUp) { ev.encStep += +1; upPressMs = now; upLastRptMs = now; }
  if (d && !prevDown){ ev.encStep += -1; dnPressMs = now; dnLastRptMs = now; }
  if (u) ev.encStep += holdRepeat(now, upPressMs, upLastRptMs);
  if (d) ev.encStep -= holdRepeat(now, dnPressMs, dnLastRptMs);

  // OK short/long
  if (o && !prevOk) { okPressMs = now; okLongFired = false; }
  if (o && !okLongFired && (uint16_t)(now - okPressMs) >= OK_LONG_MS) { ev.encLong = true; okLongFired = true; }
  if (!o && prevOk) { if (!okLongFired) ev.encClick = true; }

  prevOk = o;
  prevMenu = m;
  prevUp = u;
  prevDown = d;

  potUpdate();
}
