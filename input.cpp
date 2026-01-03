#include "config.h"
#include "input.h"

static inline bool pressed(uint8_t pin) { return digitalRead(pin) == LOW; }

// ===== OK long/short =====
static bool     okPrev = false;
static bool     okLongFired = false;
static uint32_t okPressMs = 0;
static const uint16_t OK_LONG_MS = 700;

// ===== MENU short =====
static bool menuPrev = false;
static uint32_t menuLastMs = 0;
static const uint16_t MENU_GUARD_MS = 60;

// ===== UP/DOWN repeat (общий, мягкий)
// (Wizard Ø ускоряется отдельно в .ino, здесь просто удобный repeat)
static bool     upPrev = false, dnPrev = false;
static uint32_t upPressMs = 0, dnPressMs = 0;
static uint32_t upLastRptMs = 0, dnLastRptMs = 0;

static int8_t repeatStep(uint32_t now, uint32_t pressMs, uint32_t &lastRptMs) {
  uint32_t held = now - pressMs;

  uint16_t interval;
  if      (held < 350)  interval = 180;
  else if (held < 900)  interval = 120;
  else if (held < 1600) interval = 80;
  else                  interval = 55;

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
  pinMode(PIN_START_BTN, INPUT_PULLUP); // START читается в .ino (hard)

  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_OK,   INPUT_PULLUP);
  pinMode(PIN_BTN_MENU, INPUT_PULLUP);

  pinMode(PIN_POT, INPUT);

  uint32_t now = millis();

  okPrev = pressed(PIN_BTN_OK);
  okPressMs = now;
  okLongFired = false;

  menuPrev = pressed(PIN_BTN_MENU);
  menuLastMs = now;

  upPrev = pressed(PIN_BTN_UP);
  dnPrev = pressed(PIN_BTN_DOWN);
  upPressMs = dnPressMs = now;
  upLastRptMs = dnLastRptMs = now;

  potSetFilterN(8);
  potUpdate();
}

void inputPoll(InputEvents &ev) {
  ev = {};
  uint32_t now = millis();

  bool u = pressed(PIN_BTN_UP);
  bool d = pressed(PIN_BTN_DOWN);
  bool o = pressed(PIN_BTN_OK);
  bool m = pressed(PIN_BTN_MENU);

  // MENU short (guarded)
  if (m && !menuPrev) {
    if ((uint16_t)(now - menuLastMs) >= MENU_GUARD_MS) {
      ev.menuClick = true;
      menuLastMs = now;
    }
  }

  // UP/DOWN edge + repeat
  if (u && !upPrev) { ev.encStep += +1; upPressMs = now; upLastRptMs = now; }
  if (d && !dnPrev) { ev.encStep += -1; dnPressMs = now; dnLastRptMs = now; }
  if (u) ev.encStep += repeatStep(now, upPressMs, upLastRptMs);
  if (d) ev.encStep -= repeatStep(now, dnPressMs, dnLastRptMs);

  // OK short/long
  if (o && !okPrev) {
    okPressMs = now;
    okLongFired = false;
  }
  if (o && !okLongFired && (uint16_t)(now - okPressMs) >= OK_LONG_MS) {
    ev.encLong = true;
    okLongFired = true;
  }
  if (!o && okPrev) {
    if (!okLongFired) ev.encClick = true;
  }

  // START тут не делаем (hard-read в .ino)
  ev.startClick = false;

  okPrev = o;
  menuPrev = m;
  upPrev = u;
  dnPrev = d;

  potUpdate();
}
