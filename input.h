#pragma once
#include <Arduino.h>

struct InputEvents {
  int8_t  encStep;     // UP/DOWN: +1 / -1
  bool    encClick;    // OK
  bool    menuClick;   // MENU/BACK
  bool    startClick;  // START/STOP
};

void inputBegin();
void inputPoll(InputEvents &ev);

// POT filter
void potSetFilterN(uint8_t N);     // 4/8/16
uint16_t potGetAvgAdc();           // 0..1023 filtered
