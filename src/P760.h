/*
 * P760 Arduino Library
 * Copyright (c) 2026 V7 Idea Technology Ltd.
 * Licensed under the MIT License.
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>

class P760 {
public:
  // Datasheet suggests I2C max speed 100kHz; user should set Wire.setClock(100000)
  static constexpr uint8_t DEFAULT_ADDR_A = 0x12; // Datasheet: "P760 module I2C address 0x12"
  static constexpr uint8_t DEFAULT_ADDR_B = 0x24; // Datasheet: "I2C communication address ... 0x24"

  enum class Mode : uint8_t {
    Continuous = 0x00,   // output each second
    Interval60s = 0x01,  // output each 60 seconds (default)
  };

  P760();

  // addr = 0 => auto-detect (try 0x12 then 0x24)
  bool begin(TwoWire& wire = Wire, uint8_t addr = 0);

  uint8_t address() const { return _addr; }

  // Read PM values (ug/m3). This does a single 6-byte burst read starting at reg 0x00.
  bool readPM(uint16_t& pm1, uint16_t& pm25, uint16_t& pm10);

  // Mode register at 0x06 (R/W): 0=continuous, 1=60 seconds
  bool setMode(Mode mode);
  bool setModeContinuous() { return setMode(Mode::Continuous); }
  bool setModeInterval60s() { return setMode(Mode::Interval60s); }
  bool readMode(Mode& mode);

  // IAQ (VOC index) at 0x20 (2 bytes)
  bool readIAQ(uint16_t& iaq);

  // Bosch ON/OFF at 0x2E (R/W): 0x01=ON, 0x00=OFF
  bool boschEnable(bool on);

  // PM2.5 stop/start at 0xB6 (R/W): 0x01=stop, 0x00=start
  bool setPM25Stop(bool stop);

  // Firmware version at 0x70 (1 byte)
  bool readFwVersion(uint8_t& ver);

  // Model string at 0x81..0x84 (4 bytes, ASCII "P760")
  bool readModel(char out5[5]);

  // Optional: attempt to read 12 bytes starting at 0x00 and decode pressure from value[9..11]
  // Because datasheet gives formula but not explicit starting register for the 12-byte array.
  bool readPressureFrom12B(uint32_t& pressure);

private:
  // Register map (from datasheet)
  static constexpr uint8_t REG_PM1_H      = 0x00; // PM1_H, PM1_L
  static constexpr uint8_t REG_PM25_H     = 0x02; // PM2P5_H, PM2P5_L
  static constexpr uint8_t REG_PM10_H     = 0x04; // PM10_H, PM10_L
  static constexpr uint8_t REG_MODE       = 0x06; // mode
  static constexpr uint8_t REG_IAQ_H      = 0x20; // IAQ_H, IAQ_L
  static constexpr uint8_t REG_BOSCH_ON   = 0x2E; // Bosch on/off
  static constexpr uint8_t REG_FW_VER     = 0x70; // fw version
  static constexpr uint8_t REG_MODEL_1    = 0x81; // 'P'
  static constexpr uint8_t REG_PM25_STOP  = 0xB6; // stop/start

private:
  TwoWire* _wire;
  uint8_t  _addr;

  // ---- low-level helpers ----
  bool probeAddress(uint8_t addr);
  bool writeRegU8(uint8_t reg, uint8_t val);
  bool readReg(uint8_t reg, uint8_t* buf, size_t len);

  // Endian helper: P760 uses H then L
  static uint16_t be16(const uint8_t* b) {
    return (static_cast<uint16_t>(b[0]) << 8) | static_cast<uint16_t>(b[1]);
  }
};