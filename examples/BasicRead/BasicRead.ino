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

  if (p760.startMeasurement(P760::Mode::Continuous)) {
    Serial.println("Measurement start command sent.");
  } else {
    Serial.println("Failed to start measurement.");
  }

  P760::Mode mode;
  if (p760.readMode(mode)) {
    Serial.print("Mode: ");
    Serial.println(mode == P760::Mode::Continuous ? "continuous" : "60s");
  } else {
    Serial.println("Mode: read failed");
  }

  bool pmStop;
  if (p760.readPM25Stop(pmStop)) {
    Serial.print("PM stop flag: ");
    Serial.println(pmStop ? "STOP" : "START");
  } else {
    Serial.println("PM stop flag: read failed");
  }

  Serial.println("Warm up 30s...");
  uint32_t t0 = millis();
  while (millis() - t0 < 30000) {
    uint16_t pm1, pm25, pm10;
    (void)p760.readPM(pm1, pm25, pm10);
    delay(1000);
  }
  Serial.println("Start reading...");
}

static void printHexByte(const __FlashStringHelper* label, uint8_t value) {
  Serial.print(label);
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

static void printHexArray(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    if (i + 1 != len) Serial.print(' ');
  }
}

void loop() {
  uint16_t pm1, pm25, pm10;
  uint16_t iaq;
  uint8_t rawPM[6];
  uint8_t envBlock[8];
  uint8_t modeRaw, boschRaw, fwRaw;
  P760::Mode mode;
  bool boschOn, pmStop;

  bool okPM = p760.readPM(pm1, pm25, pm10);
  bool okIAQ = p760.readIAQ(iaq);
  bool okRawPM = p760.readPMRaw(rawPM);
  bool okEnvBlock = p760.readEnvBlock(envBlock);
  bool okModeRaw = p760.readModeRaw(modeRaw);
  bool okBoschRaw = p760.readBoschEnableRaw(boschRaw);
  bool okFwRaw = p760.readFwVersion(fwRaw);
  bool okMode = p760.readMode(mode);
  bool okBosch = p760.readBoschEnable(boschOn);
  bool okPMStop = p760.readPM25Stop(pmStop);

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

  if (okRawPM) {
    Serial.print("RAW=");
    for (size_t i = 0; i < sizeof(rawPM); ++i) {
      if (rawPM[i] < 0x10) Serial.print('0');
      Serial.print(rawPM[i], HEX);
      if (i + 1 != sizeof(rawPM)) Serial.print(' ');
    }
  } else {
    Serial.print("RAW=ERR");
  }

  Serial.print(" | ");

  if (okIAQ) {
    Serial.print("IAQ=");
    Serial.print(iaq);
  } else {
    Serial.print("IAQ=ERR");
  }

  Serial.print(" | ");

  if (okEnvBlock) {
    const uint16_t iaqRaw = (static_cast<uint16_t>(envBlock[0]) << 8) | envBlock[1];
    const uint16_t co2Raw = (static_cast<uint16_t>(envBlock[2]) << 8) | envBlock[3];
    const uint16_t bvocRaw = (static_cast<uint16_t>(envBlock[4]) << 8) | envBlock[5];

    Serial.print("ENV[20..27]=");
    printHexArray(envBlock, sizeof(envBlock));
    Serial.print(" IAQraw=");
    Serial.print(iaqRaw);
    Serial.print(" CO2raw=");
    Serial.print(co2Raw);
    Serial.print(" BVOCraw=");
    Serial.print(bvocRaw);
    Serial.print(" TEMPraw=");
    Serial.print(envBlock[6]);
    Serial.print(" RHraw=");
    Serial.print(envBlock[7]);
  } else {
    Serial.print("ENV[20..27]=ERR");
  }

  Serial.print(" | ");

  if (okModeRaw) {
    printHexByte(F("0x06="), modeRaw);
  } else {
    Serial.print("0x06=ERR");
  }

  Serial.print("/");
  if (okMode) {
    Serial.print(mode == P760::Mode::Continuous ? "continuous" : "60s");
  } else {
    Serial.print("mode-ERR");
  }

  Serial.print(" | ");

  if (okBoschRaw) {
    printHexByte(F("0x2E="), boschRaw);
  } else {
    Serial.print("0x2E=ERR");
  }

  Serial.print("/");
  if (okBosch) {
    Serial.print(boschOn ? "bosch-on" : "bosch-off");
  } else {
    Serial.print("bosch-ERR");
  }

  Serial.print(" | ");

  if (okPMStop) {
    printHexByte(F("0xB6="), pmStop ? 0x01 : 0x00);
    Serial.print("/");
    Serial.print(pmStop ? "STOP" : "START");
  } else {
    Serial.print("0xB6=ERR/pmstop-ERR");
  }

  Serial.print(" | ");

  if (okFwRaw) {
    printHexByte(F("0x70="), fwRaw);
  } else {
    Serial.print("0x70=ERR");
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
