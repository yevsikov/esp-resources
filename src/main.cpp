#include <Arduino.h>

namespace {
  constexpr uint32_t BAUDRATE = 115200;
  constexpr uint8_t LED_PIN = 5;
  constexpr uint32_t BLINK_INTERVAL_MS = 1000;
}

enum class LedState : uint8_t {
  Off,
  On
};

LedState currentState = LedState::Off;

void setLedState(LedState state) {
  currentState = state;
  digitalWrite(LED_PIN, state == LedState::On ? HIGH : LOW);
  Serial.println(state == LedState::On ? "on" : "off");
}

void setup() {
  Serial.begin(BAUDRATE);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  setLedState(LedState::On);
  delay(BLINK_INTERVAL_MS);

  setLedState(LedState::Off);
  delay(BLINK_INTERVAL_MS);
}

