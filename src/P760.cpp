#include "P760.h"

P760::P760() : _wire(nullptr), _addr(0) {}

bool P760::begin(TwoWire& wire, uint8_t addr) {
  _wire = &wire;

  if (addr != 0) {
    _addr = addr;
    return probeAddress(_addr);
  }

  // Auto-detect: try both addresses that appear in the datasheet
  if (probeAddress(DEFAULT_ADDR_A)) {
    _addr = DEFAULT_ADDR_A;
    return true;
  }
  if (probeAddress(DEFAULT_ADDR_B)) {
    _addr = DEFAULT_ADDR_B;
    return true;
  }

  _addr = 0;
  return false;
}

bool P760::probeAddress(uint8_t addr) {
  if (!_wire) return false;
  _wire->beginTransmission(addr);
  uint8_t err = _wire->endTransmission(true);
  return (err == 0);
}

bool P760::writeRegU8(uint8_t reg, uint8_t val) {
  if (!_wire || _addr == 0) return false;

  _wire->beginTransmission(_addr);
  _wire->write(reg);
  _wire->write(val);
  uint8_t err = _wire->endTransmission(true);
  return (err == 0);
}

bool P760::readReg(uint8_t reg, uint8_t* buf, size_t len) {
  if (!_wire || _addr == 0 || !buf || len == 0) return false;

  // Write register address, then repeated start
  _wire->beginTransmission(_addr);
  _wire->write(reg);
  uint8_t err = _wire->endTransmission(false); // repeated start
  if (err != 0) return false;

  size_t got = _wire->requestFrom(static_cast<int>(_addr), static_cast<int>(len), static_cast<int>(true));
  if (got != len) {
    // Drain what we got to keep bus sane
    while (_wire->available()) (void)_wire->read();
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    int c = _wire->read();
    if (c < 0) return false;
    buf[i] = static_cast<uint8_t>(c);
  }
  return true;
}

bool P760::readPM(uint16_t& pm1, uint16_t& pm25, uint16_t& pm10) {
  uint8_t b[6];
  if (!readReg(REG_PM1_H, b, sizeof(b))) return false;

  pm1  = be16(&b[0]);
  pm25 = be16(&b[2]);
  pm10 = be16(&b[4]);
  return true;
}

bool P760::setMode(Mode mode) {
  return writeRegU8(REG_MODE, static_cast<uint8_t>(mode));
}

bool P760::readMode(Mode& mode) {
  uint8_t v = 0;
  if (!readReg(REG_MODE, &v, 1)) return false;
  mode = static_cast<Mode>(v);
  return true;
}

bool P760::readIAQ(uint16_t& iaq) {
  uint8_t b[2];
  if (!readReg(REG_IAQ_H, b, sizeof(b))) return false;
  iaq = be16(&b[0]);
  return true;
}

bool P760::boschEnable(bool on) {
  return writeRegU8(REG_BOSCH_ON, on ? 0x01 : 0x00);
}

bool P760::setPM25Stop(bool stop) {
  return writeRegU8(REG_PM25_STOP, stop ? 0x01 : 0x00);
}

bool P760::readFwVersion(uint8_t& ver) {
  return readReg(REG_FW_VER, &ver, 1);
}

bool P760::readModel(char out5[5]) {
  if (!out5) return false;

  uint8_t b[4];
  if (!readReg(REG_MODEL_1, b, sizeof(b))) return false;

  out5[0] = static_cast<char>(b[0]);
  out5[1] = static_cast<char>(b[1]);
  out5[2] = static_cast<char>(b[2]);
  out5[3] = static_cast<char>(b[3]);
  out5[4] = '\0';
  return true;
}

bool P760::readPressureFrom12B(uint32_t& pressure) {
  // Datasheet mentions: read 12 bytes(value[0]..value[11]),
  // Air Pressure = value[9]<<16 + value[10]<<8 + value[11]
  // However, it doesn't clearly state the starting register for that 12-byte array.
  // We conservatively start at 0x00 (PM1_H) to test the "single burst frame" hypothesis.
  uint8_t b[12];
  if (!readReg(0x00, b, sizeof(b))) return false;

  pressure =
      (static_cast<uint32_t>(b[9])  << 16) |
      (static_cast<uint32_t>(b[10]) << 8)  |
      (static_cast<uint32_t>(b[11]) << 0);
  return true;
}