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

class Led {
public:
  explicit Led(uint8_t pin) : pin_(pin) {}

  void init() const {
    pinMode(pin_, OUTPUT);
  }

  void set(LedState state) const {
    digitalWrite(pin_, state == LedState::On ? HIGH : LOW);
  }

private:
  uint8_t pin_;
};

Led led(LED_PIN);
LedState currentState = LedState::Off;
uint32_t lastToggleMs = 0;

void setup() {
  Serial.begin(BAUDRATE);
  led.init();
  led.set(currentState);
}

void loop() {
  const uint32_t now = millis();
  if (now - lastToggleMs < BLINK_INTERVAL_MS) {
    return;
  }

  lastToggleMs = now;
  currentState = (currentState == LedState::On) ? LedState::Off : LedState::On;
  led.set(currentState);

  Serial.println(currentState == LedState::On ? "on" : "off");
}

