#include "config.h"
#include "input.h"

static inline bool isPressedPullup(uint8_t pin) { return digitalRead(pin) == LOW; }

struct DebouncedButton {
  uint8_t pin = 0;
  bool stable = false;      // debounced level (pressed=true)
  bool lastRead = false;    // raw read
  uint32_t lastChange = 0;

  void begin(uint8_t p) {
    pin = p;
    stable = isPressedPullup(pin);
    lastRead = stable;
    lastChange = millis();
  }

  bool update(uint16_t db_ms = 25) {
    bool r = isPressedPullup(pin);
    if (r != lastRead) {
      lastRead = r;
      lastChange = millis();
    }
    if ((millis() - lastChange) >= db_ms && stable != lastRead) {
      stable = lastRead;
      return true;
    }
    return false;
  }

  bool isPressed() const { return stable; }

  bool pressedEdge(uint16_t db_ms = 25) {
    bool changed = update(db_ms);
    return (changed && stable);
  }

  bool releasedEdge(uint16_t db_ms = 25) {
    bool changed = update(db_ms);
    return (changed && !stable);
  }
};

static DebouncedButton bStart, bUp, bDown, bOk, bMenu;

// ===== Hold repeat (ускорение UP/DOWN) =====
struct HoldRepeat {
  bool active = false;
  uint32_t pressMs = 0;
  uint32_t lastRptMs = 0;

  void onPress(uint32_t now) {
    active = true;
    pressMs = now;
    lastRptMs = now;
  }
  void onRelease() { active = false; }

  int8_t pollRepeat(uint32_t now) {
    if (!active) return 0;

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
};

static HoldRepeat upHold, downHold;

// ===== OK long press =====
static bool okPressed = false;
static bool okLongFired = false;
static uint32_t okPressMs = 0;
static constexpr uint16_t OK_LONG_MS = 700;

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
  uint32_t now = millis();

  // --- UP/DOWN: short + ускорение удержанием
  if (bUp.pressedEdge()) {
    ev.encStep += +1;
    upHold.onPress(now);
  }
  if (bUp.releasedEdge()) upHold.onRelease();

  if (bDown.pressedEdge()) {
    ev.encStep += -1;
    downHold.onPress(now);
  }
  if (bDown.releasedEdge()) downHold.onRelease();

  if (upHold.active)   ev.encStep += upHold.pollRepeat(now);
  if (downHold.active) ev.encStep -= downHold.pollRepeat(now);

  // --- OK: short vs long
  if (bOk.pressedEdge()) {
    okPressed = true;
    okLongFired = false;
    okPressMs = now;
  }
  if (okPressed && !okLongFired && bOk.isPressed() && (now - okPressMs) >= OK_LONG_MS) {
    ev.encLong = true;
    okLongFired = true;
  }
  if (bOk.releasedEdge()) {
    if (okPressed && !okLongFired) ev.encClick = true;
    okPressed = false;
  }

  // --- MENU / START short
  ev.menuClick  = bMenu.pressedEdge();
  ev.startClick = bStart.pressedEdge();

  // POT
  potUpdate();
}
