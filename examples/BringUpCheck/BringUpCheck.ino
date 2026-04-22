#include <Wire.h>
#include <P760.h>

P760 p760;

static bool hasAnyNonZero(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (data[i] != 0x00) return true;
  }
  return false;
}

static void printHexArray(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    if (i + 1 != len) Serial.print(' ');
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin();
  Wire.setClock(100000);

  Serial.println();
  Serial.println("=== P760 Bring-Up Check ===");

  if (!p760.begin(Wire)) {
    Serial.println("FAIL: P760 not found at 0x12 or 0x24.");
    while (1) delay(1000);
  }

  Serial.print("I2C address: 0x");
  Serial.println(p760.address(), HEX);

  char model[5] = {0};
  if (p760.readModel(model)) {
    Serial.print("Model: ");
    Serial.println(model);
  } else {
    Serial.println("Model: read failed");
  }

  uint8_t fw = 0;
  if (p760.readFwVersion(fw)) {
    Serial.print("FW: 0x");
    if (fw < 0x10) Serial.print('0');
    Serial.println(fw, HEX);
  } else {
    Serial.println("FW: read failed");
  }

  Serial.println("Sending startMeasurement(continuous) every 5s for 60s...");
}

void loop() {
  static uint32_t startMs = millis();
  static uint32_t lastKickMs = 0;
  static uint32_t lastPrintMs = 0;
  static uint16_t sampleCount = 0;
  static uint16_t nonZeroPmSamples = 0;
  static uint16_t nonZeroEnvSamples = 0;
  static bool summaryPrinted = false;

  const uint32_t now = millis();

  if (now - lastKickMs >= 5000 || lastKickMs == 0) {
    lastKickMs = now;
    const bool started = p760.startMeasurement(P760::Mode::Continuous);
    Serial.print("[kick] startMeasurement: ");
    Serial.println(started ? "OK" : "FAIL");
  }

  if (now - lastPrintMs >= 1000 || lastPrintMs == 0) {
    lastPrintMs = now;

    uint8_t rawPm[6] = {0};
    uint8_t env[8] = {0};
    P760::Mode mode;
    bool pmStop = true;

    const bool okPm = p760.readPMRaw(rawPm);
    const bool okEnv = p760.readEnvBlock(env);
    const bool okMode = p760.readMode(mode);
    const bool okStop = p760.readPM25Stop(pmStop);

    ++sampleCount;
    if (okPm && hasAnyNonZero(rawPm, sizeof(rawPm))) ++nonZeroPmSamples;
    if (okEnv && hasAnyNonZero(env, sizeof(env))) ++nonZeroEnvSamples;

    Serial.print("[");
    Serial.print((now - startMs) / 1000);
    Serial.print("s] ");

    Serial.print("PMraw=");
    if (okPm) {
      printHexArray(rawPm, sizeof(rawPm));
    } else {
      Serial.print("ERR");
    }

    Serial.print(" ENV=");
    if (okEnv) {
      printHexArray(env, sizeof(env));
    } else {
      Serial.print("ERR");
    }

    Serial.print(" MODE=");
    if (okMode) {
      Serial.print(mode == P760::Mode::Continuous ? "continuous" : "60s");
    } else {
      Serial.print("ERR");
    }

    Serial.print(" PMSTOP=");
    if (okStop) {
      Serial.print(pmStop ? "STOP" : "START");
    } else {
      Serial.print("ERR");
    }

    Serial.println();
  }

  if (!summaryPrinted && now - startMs >= 60000) {
    summaryPrinted = true;

    Serial.println();
    Serial.println("=== 60s Summary ===");
    Serial.print("Samples: ");
    Serial.println(sampleCount);
    Serial.print("Non-zero PM samples: ");
    Serial.println(nonZeroPmSamples);
    Serial.print("Non-zero ENV samples: ");
    Serial.println(nonZeroEnvSamples);

    if (nonZeroPmSamples == 0 && nonZeroEnvSamples == 0) {
      Serial.println("RESULT: I2C/control path is alive, but measurement data stayed zero.");
      Serial.println("CHECK: 5V supply, current capacity, pump airflow, cable orientation, module health.");
    } else {
      Serial.println("RESULT: At least one measurement block became non-zero.");
    }

    Serial.println("Bring-up check complete.");
    while (1) delay(1000);
  }
}
