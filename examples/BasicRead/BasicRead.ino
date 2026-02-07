#include <Wire.h>
#include <P760.h>

P760 p760;

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin();
  Wire.setClock(100000); // datasheet: max 100 kbit/s

  if (!p760.begin(Wire)) {
    Serial.println("P760 not found at 0x12 or 0x24.");
    while (1) delay(1000);
  }

  Serial.print("P760 found at 0x");
  Serial.println(p760.address(), HEX);

  char model[5];
  if (p760.readModel(model)) {
    Serial.print("Model: ");
    Serial.println(model);
  }

  uint8_t fw;
  if (p760.readFwVersion(fw)) {
    Serial.print("FW: 0x");
    Serial.println(fw, HEX);
  }

  // Continuous mode (1s)
  p760.setModeContinuous();

  Serial.println("Warm up 30s...");
  uint32_t t0 = millis();
  while (millis() - t0 < 30000) {
    uint16_t pm1, pm25, pm10;
    (void)p760.readPM(pm1, pm25, pm10);
    delay(1000);
  }
  Serial.println("Start reading...");
}

void loop() {
  uint16_t pm1, pm25, pm10;
  uint16_t iaq;

  bool okPM = p760.readPM(pm1, pm25, pm10);
  bool okIAQ = p760.readIAQ(iaq);

  if (okPM) {
    Serial.print("PM1=");
    Serial.print(pm1);
    Serial.print(" PM2.5=");
    Serial.print(pm25);
    Serial.print(" PM10=");
    Serial.print(pm10);
  } else {
    Serial.print("PM=ERR");
  }

  Serial.print(" | ");

  if (okIAQ) {
    Serial.print("IAQ=");
    Serial.print(iaq);
  } else {
    Serial.print("IAQ=ERR");
  }

  // Optional pressure test
  uint32_t p;
  if (p760.readPressureFrom12B(p)) {
    Serial.print(" | Praw=");
    Serial.print(p);
  }

  Serial.println();
  delay(1000);
}