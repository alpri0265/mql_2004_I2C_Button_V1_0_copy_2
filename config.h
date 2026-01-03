#pragma once
#include <Arduino.h>

// ===== LCD 2004 I2C =====
constexpr uint8_t LCD_I2C_ADDR = 0x27;   // поменяй на 0x3F если у тебя так

// ===== POT =====
constexpr uint8_t PIN_POT = A0;

// ===== START button + LED =====
constexpr uint8_t PIN_START_BTN = A1;    // кнопка START/STOP (NO) -> GND
constexpr uint8_t PIN_START_LED = A2;    // LED кнопки START

// ===== DM556 =====
constexpr uint8_t PIN_STEP = 3;          // PUL+
constexpr uint8_t PIN_DIR  = 10;         // DIR+
constexpr uint8_t PIN_ENA  = 11;         // ENA+

// ===== UI кнопки (вместо энкодера) =====
constexpr uint8_t PIN_BTN_UP   = 2;       // UP   (NO -> GND)
constexpr uint8_t PIN_BTN_DOWN = 12;      // DOWN (NO -> GND)
constexpr uint8_t PIN_BTN_OK   = A3;      // OK   (NO -> GND)
constexpr uint8_t PIN_BTN_MENU = 4;       // MENU/BACK (NO -> GND)

// ===== Тайминги =====
constexpr uint16_t INPUT_POLL_MS = 5;
constexpr uint16_t UI_REFRESH_MS = 200;
