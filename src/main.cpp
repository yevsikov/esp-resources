#include <Arduino.h>

// Compile-time configuration used across the app.
class AppConfig {
public:
  // Serial monitor speed.
  static constexpr uint32_t kBaudrate = 115200;
  // GPIO for the onboard/external LED.
  static constexpr uint8_t kLedPin = 5;
  // GPIO where the button module OUT pin is connected.
  static constexpr uint8_t kButtonPin = 8;
  // Blink period in Blink mode.
  static constexpr uint32_t kBlinkIntervalMs = 1000;
  // Software debounce window for button events from ISR.
  static constexpr uint32_t kButtonDebounceMs = 150;
  // Reserved for future short-press behavior.
  static const uint8_t kShortPressBlinkCount = 3;
  // Print loop timing stats every N iterations.
  static constexpr uint16_t kLoopReportEveryIterations = 1000;
};

// Physical LED level abstraction.
enum class LedState : uint8_t {
  Off,
  On
};

// Logical behavior modes switched by button presses.
enum class LedMode : uint8_t {
  Blink,
  AlwaysOn,
  AlwaysOff
};

// Thin hardware wrapper for LED pin operations.
class Led {
public:
  explicit Led(uint8_t pin) : pin_(pin) {}

  // Configure LED pin as output.
  void init() const {
    pinMode(pin_, OUTPUT);
  }

  // Apply logical LED state to hardware pin.
  void set(LedState state) const {
    digitalWrite(pin_, state == LedState::On ? HIGH : LOW);
  }

private:
  uint8_t pin_;
};

// Runtime state container to avoid scattered globals.
struct AppState {
  Led led{AppConfig::kLedPin};
  LedMode currentMode = LedMode::Blink;
  LedState currentState = LedState::Off;
  // Last timestamp when LED toggled in Blink mode.
  uint32_t lastToggleMs = {};
  // Last accepted button event timestamp (debounce).
  uint32_t lastButtonHandledMs = {};
  // Loop performance counters.
  uint32_t loopIterations = {};
  uint64_t loopDurationUsTotal = {};
  // ISR-to-loop event flag.
  volatile bool buttonPressed = false;
};

AppState app;

// ISR must be minimal: only signal an event.
void IRAM_ATTR onButtonPressedIsr() {
  app.buttonPressed = true;
}

// Cycle mode order: Blink -> AlwaysOn -> AlwaysOff -> Blink.
void goToNextMode() {
  switch (app.currentMode) {
    case LedMode::Blink:
      app.currentMode = LedMode::AlwaysOn;
      break;
    case LedMode::AlwaysOn:
      app.currentMode = LedMode::AlwaysOff;
      break;
    case LedMode::AlwaysOff:
    default:
      app.currentMode = LedMode::Blink;
      break;
  }
}

// One non-blocking superloop step for LED behavior.
void runSuperloopStep(uint32_t now) {
  if (app.currentMode == LedMode::Blink) {
    if (now - app.lastToggleMs < AppConfig::kBlinkIntervalMs) {
      return;
    }

    app.lastToggleMs = now;
    app.currentState =
        (app.currentState == LedState::On) ? LedState::Off : LedState::On;
    app.led.set(app.currentState);
    Serial.println(app.currentState == LedState::On ? "on" : "off");
    return;
  }

  const LedState targetState =
      (app.currentMode == LedMode::AlwaysOn) ? LedState::On : LedState::Off;
  if (app.currentState != targetState) {
    app.currentState = targetState;
    app.led.set(app.currentState);
    Serial.println(app.currentState == LedState::On ? "on" : "off");
  }
}

// Hardware setup: serial, LED pin, button interrupt.
void setup() {
  Serial.begin(AppConfig::kBaudrate);
  app.led.init();
  app.led.set(app.currentState);

  pinMode(AppConfig::kButtonPin, INPUT);
  attachInterrupt(
      digitalPinToInterrupt(AppConfig::kButtonPin), onButtonPressedIsr, RISING);
}

// Main superloop: consume button events, run LED logic, report timing.
void loop() {
  const uint32_t startedUs = micros();
  const uint32_t now = millis();

  // Copy-and-clear ISR flag inside a very short critical section.
  bool pressed = false;
  noInterrupts();
  if (app.buttonPressed) {
    app.buttonPressed = false;
    pressed = true;
  }
  interrupts();

  // Handle debounced button event in normal (non-ISR) context.
  if (pressed) {
    if (now - app.lastButtonHandledMs >= AppConfig::kButtonDebounceMs) {
      app.lastButtonHandledMs = now;
      goToNextMode();

      const char *modeText = "blink";
      if (app.currentMode == LedMode::AlwaysOn) {
        modeText = "always_on";
      } else if (app.currentMode == LedMode::AlwaysOff) {
        modeText = "always_off";
      }

      Serial.print("mode=");
      Serial.println(modeText);
    }
  }

  runSuperloopStep(now);
  const uint32_t loopDurationUs = micros() - startedUs;

  // Optional loop-time statistics for performance visibility.
  ++app.loopIterations;
  app.loopDurationUsTotal += loopDurationUs;
  if (app.loopIterations >= AppConfig::kLoopReportEveryIterations) {
    const uint32_t avgLoopDurationUs =
        static_cast<uint32_t>(app.loopDurationUsTotal / app.loopIterations);
    Serial.print("loop_us last=");
    Serial.print(loopDurationUs);
    Serial.print(" avg=");
    Serial.println(avgLoopDurationUs);

    app.loopIterations = {};
    app.loopDurationUsTotal = {};
  }
}

