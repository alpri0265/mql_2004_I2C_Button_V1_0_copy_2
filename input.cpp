#include "config.h"
#include "input.h"

static inline bool isPressedPullup(uint8_t pin) { return digitalRead(pin) == LOW; }

struct DebouncedButton {
  uint8_t pin = 0;
  bool stable = false;
  bool lastRead = false;
  uint32_t lastChange = 0;

  void begin(uint8_t p) {
    pin = p;
    stable = isPressedPullup(pin);
    lastRead = stable;
    lastChange = millis();
  }

  // Rising edge: released -> pressed
  bool pressedEdge(uint16_t db_ms = 25) {
    bool r = isPressedPullup(pin);
    if (r != lastRead) {
      lastRead = r;
      lastChange = millis();
    }
    if ((millis() - lastChange) >= db_ms && stable != lastRead) {
      bool old = stable;
      stable = lastRead;
      if (!old && stable) return true;
    }
    return false;
  }
};

static DebouncedButton bStart, bUp, bDown, bOk, bMenu;

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

  bStart.begin(PIN_START_BTN);
  bUp.begin(PIN_BTN_UP);
  bDown.begin(PIN_BTN_DOWN);
  bOk.begin(PIN_BTN_OK);
  bMenu.begin(PIN_BTN_MENU);

  potSetFilterN(8);
  potUpdate();
}

void inputPoll(InputEvents &ev) {
  ev = {};

  if (bUp.pressedEdge())   ev.encStep = +1;
  if (bDown.pressedEdge()) ev.encStep = -1;

  ev.encClick   = bOk.pressedEdge();     // OK
  ev.menuClick  = bMenu.pressedEdge();   // MENU/BACK
  ev.startClick = bStart.pressedEdge();  // START/STOP

  potUpdate();
}
