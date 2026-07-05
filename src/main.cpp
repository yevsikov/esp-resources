#include <Arduino.h>

class AppConfig {
public:
  static constexpr uint32_t kBaudrate = 115200;
  static constexpr uint8_t kLedPin = 5;
  static constexpr uint32_t kBlinkIntervalMs = 1000;
  static const uint8_t kShortPressBlinkCount = 3;
  static constexpr uint16_t kLoopReportEveryIterations = 1000;
};

enum class LedState : uint8_t {
  Off,
  On
};

enum class LedMode : uint8_t {
  Blink,
  AlwaysOn,
  AlwaysOff
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
LedMode currentMode = LedMode::Blink;
LedState currentState = LedState::Off;
uint32_t lastToggleMs = {};
uint32_t loopIterations = {};
uint64_t loopDurationUsTotal = {};

void runSuperloopStep(uint32_t now) {
  if (currentMode == LedMode::Blink) {
    if (now - lastToggleMs < AppConfig::kBlinkIntervalMs) {
      return;
    }

    lastToggleMs = now;
    currentState = (currentState == LedState::On) ? LedState::Off : LedState::On;
    led.set(currentState);
    Serial.println(currentState == LedState::On ? "on" : "off");
    return;
  }

  const LedState targetState =
      (currentMode == LedMode::AlwaysOn) ? LedState::On : LedState::Off;
  if (currentState != targetState) {
    currentState = targetState;
    led.set(currentState);
    Serial.println(currentState == LedState::On ? "on" : "off");
  }
}

void setup() {
  Serial.begin(AppConfig::kBaudrate);
  led.init();
  led.set(currentState);
}

void loop() {
  const uint32_t startedUs = micros();
  const uint32_t now = millis();
  runSuperloopStep(now);
  const uint32_t loopDurationUs = micros() - startedUs;

  ++loopIterations;
  loopDurationUsTotal += loopDurationUs;
  if (loopIterations >= AppConfig::kLoopReportEveryIterations) {
    const uint32_t avgLoopDurationUs =
        static_cast<uint32_t>(loopDurationUsTotal / loopIterations);
    Serial.print("loop_us last=");
    Serial.print(loopDurationUs);
    Serial.print(" avg=");
    Serial.println(avgLoopDurationUs);

    loopIterations = {};
    loopDurationUsTotal = {};
  }
}

