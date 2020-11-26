#define PUSH_BUTTON 13

void setup() {
  Serial.begin(9600);
  pinMode(PUSH_BUTTON, INPUT);
}

void loop() {
  if (digitalRead(PUSH_BUTTON) == HIGH) {
    // turn on
    digitalWrite(LED, HIGH);
    Serial.println("ON");
  } else {
    // turn off
    digitalWrite(LED, LOW);
    Serial.println("OFF");
  }
}
