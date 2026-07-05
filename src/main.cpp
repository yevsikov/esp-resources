#include <Arduino.h>

class AppConfig {
public:
  static constexpr uint32_t kBaudrate = 115200;
  static constexpr uint8_t kLedPin = 5;
  static constexpr uint32_t kBlinkIntervalMs = 1000;
  static const uint8_t kShortPressBlinkCount = 3;
};

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

Led led(AppConfig::kLedPin);
LedState currentState = LedState::Off;
uint32_t lastToggleMs = {};

void setup() {
  Serial.begin(AppConfig::kBaudrate);
  led.init();
  led.set(currentState);
}

void loop() {
  const uint32_t now = millis();
  if (now - lastToggleMs < AppConfig::kBlinkIntervalMs) {
    return;
  }

  lastToggleMs = now;
  currentState = (currentState == LedState::On) ? LedState::Off : LedState::On;
  led.set(currentState);

  Serial.println(currentState == LedState::On ? "on" : "off");
}

