#pragma once
#include "types.h"

enum MenuAction : uint8_t {
  MENU_ACT_NONE = 0,
  MENU_ACT_SAVE,
  MENU_ACT_DEFAULTS,
  MENU_ACT_RECOMPUTE,
  MENU_ACT_CAL_START_60,
  MENU_ACT_CAL_START_120,
  MENU_ACT_CAL_CLEAR
};

struct MenuState {
  uint8_t index = 0;
  bool editing = false;
};

void menuReset(MenuState &m);

MenuAction menuOnDelta(MenuState &m, int8_t step, Settings &S);
MenuAction menuOnClick(MenuState &m, Settings &S);

void menuRender3(const MenuState &m, const Settings &S, char line1[21], char line2[21], char line3[21]);
