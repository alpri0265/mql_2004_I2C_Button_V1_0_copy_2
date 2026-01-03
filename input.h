#pragma once
#include <Arduino.h>

// Поля называются по старому (enc*), но теперь это 100% кнопки:
// encStep  = UP/DOWN
// encClick = OK (короткое)
// encLong  = OK (длинное)
// menuClick = MENU (короткое)
// startClick тут не используем (START читается "hard" в .ino)
struct InputEvents {
  int8_t  encStep = 0;
  bool    encClick = false;
  bool    encLong = false;
  bool    menuClick = false;
  bool    startClick = false;
};

void inputBegin();
void inputPoll(InputEvents &ev);

// POT filter
void potSetFilterN(uint8_t N);     // 4/8/16
uint16_t potGetAvgAdc();           // 0..1023 filtered
