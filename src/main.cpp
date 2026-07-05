#include <Arduino.h>

class AppConfig {
public:
  static constexpr uint32_t kBaudrate = 115200;
  static constexpr uint8_t kLedPin = 5;
  static constexpr uint8_t kButtonPin = 8;
  static constexpr uint32_t kBlinkIntervalMs = 1000;
  static constexpr uint32_t kButtonDebounceMs = 150;
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
uint32_t lastButtonHandledMs = {};
uint32_t loopIterations = {};
uint64_t loopDurationUsTotal = {};
volatile bool buttonPressed = false;

void IRAM_ATTR onButtonPressedIsr() {
  buttonPressed = true;
}

void goToNextMode() {
  switch (currentMode) {
    case LedMode::Blink:
      currentMode = LedMode::AlwaysOn;
      break;
    case LedMode::AlwaysOn:
      currentMode = LedMode::AlwaysOff;
      break;
    case LedMode::AlwaysOff:
    default:
      currentMode = LedMode::Blink;
      break;
  }
}

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

  pinMode(AppConfig::kButtonPin, INPUT);
  attachInterrupt(
      digitalPinToInterrupt(AppConfig::kButtonPin), onButtonPressedIsr, RISING);
}

void loop() {
  const uint32_t startedUs = micros();
  const uint32_t now = millis();

  bool pressed = false;
  noInterrupts();
  if (buttonPressed) {
    buttonPressed = false;
    pressed = true;
  }
  interrupts();

  if (pressed) {
    if (now - lastButtonHandledMs >= AppConfig::kButtonDebounceMs) {
      lastButtonHandledMs = now;
      goToNextMode();

      const char *modeText = "blink";
      if (currentMode == LedMode::AlwaysOn) {
        modeText = "always_on";
      } else if (currentMode == LedMode::AlwaysOff) {
        modeText = "always_off";
      }

      Serial.print("mode=");
      Serial.println(modeText);
    }
  }

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

