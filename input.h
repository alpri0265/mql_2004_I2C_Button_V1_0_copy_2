#pragma once
#include <Arduino.h>

struct InputEvents {
  int8_t  encStep;      // UP/DOWN: +1 / -1
  bool    encClick;     // OK short
  bool    encLong;      // OK long
  bool    menuClick;    // MENU/BACK short
  bool    startClick;   // START/STOP short (может быть перезаписан HARD START в .ino)
};

void inputBegin();
void inputPoll(InputEvents &ev);

// POT filter
void potSetFilterN(uint8_t N);     // 4/8/16
uint16_t potGetAvgAdc();           // 0..1023 filtered
