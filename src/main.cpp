#include <Arduino.h>

#define BAUDRATE 115200
#define LED_PIN 5

void setup() {
  Serial.begin(BAUDRATE);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("hight");
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  Serial.println("low");
  delay(1000);

}

