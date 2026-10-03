#include <Arduino.h>
#include <Wire.h>
#include <axp2101.h>
#include <config.h>
#include "trmnl_log.h"

#ifdef BOARD_WAVESHARE_PHOTOPAINTER

namespace {
constexpr uint8_t STATUS1 = 0x00;
constexpr uint8_t STATUS2 = 0x01;
constexpr uint8_t ADC_CHANNEL_CTRL = 0x30;
constexpr uint8_t BATTERY_VOLTAGE_H = 0x34;
constexpr uint8_t BATTERY_VOLTAGE_L = 0x35;
constexpr uint8_t BATTERY_DETECTION_CTRL = 0x68;
constexpr uint8_t DC_ONOFF_DVM_CTRL = 0x80;
constexpr uint8_t DC1_VOLTAGE_CTRL = 0x82;
constexpr uint8_t LDO_ONOFF_CTRL0 = 0x90;
constexpr uint8_t ALDO3_VOLTAGE_CTRL = 0x94;
constexpr uint8_t ALDO4_VOLTAGE_CTRL = 0x95;
}

bool Axp2101::begin() {
  if (_initialized) {
    return _available;
  }

  _initialized = true;
  Wire.begin(PHOTO_PAINTER_PMIC_SDA, PHOTO_PAINTER_PMIC_SCL);
  Wire.beginTransmission(PHOTO_PAINTER_PMIC_ADDR);
  _available = Wire.endTransmission() == 0;
  if (_available) {
    Log_info("AXP2101 PMIC found on I2C address 0x%02X", PHOTO_PAINTER_PMIC_ADDR);
  } else {
    Log_error("AXP2101 PMIC NOT found on I2C address 0x%02X", PHOTO_PAINTER_PMIC_ADDR);
  }
  return _available;
}

bool Axp2101::isReady() {
  return begin();
}

bool Axp2101::initialize() {
  if (!begin()) {
    return false;
  }

  uint8_t value = 0;

  // 1. Configure DC1 voltage to 3.3V (1500mV + 18*100mV = 3300mV -> 0x12)
  if (!readRegister(DC1_VOLTAGE_CTRL, value) ||
      !writeRegister(DC1_VOLTAGE_CTRL, (value & 0xe0) | 0x12)) {
    Log_error("AXP2101 failed to set DC1 voltage");
    return false;
  }

  // 2. Enable DC1 power output (bit 0 in register 0x80)
  if (!readRegister(DC_ONOFF_DVM_CTRL, value) ||
      !writeRegister(DC_ONOFF_DVM_CTRL, value | 0x01)) {
    Log_error("AXP2101 failed to enable DC1 output");
    return false;
  }

  // 3. Configure ALDO3 voltage to 3.3V (500mV + 28*100mV = 3300mV -> 0x1c)
  if (!readRegister(ALDO3_VOLTAGE_CTRL, value) ||
      !writeRegister(ALDO3_VOLTAGE_CTRL, (value & 0xe0) | 0x1c)) {
    Log_error("AXP2101 failed to set ALDO3 voltage");
    return false;
  }

  // 4. Configure ALDO4 voltage to 3.3V (500mV + 28*100mV = 3300mV -> 0x1c)
  if (!readRegister(ALDO4_VOLTAGE_CTRL, value) ||
      !writeRegister(ALDO4_VOLTAGE_CTRL, (value & 0xe0) | 0x1c)) {
    Log_error("AXP2101 failed to set ALDO4 voltage");
    return false;
  }

  // 5. Enable ALDO3 (bit 2) and ALDO4 (bit 3) power outputs in register 0x90
  if (!readRegister(LDO_ONOFF_CTRL0, value) ||
      !writeRegister(LDO_ONOFF_CTRL0, value | 0x0c)) {
    Log_error("AXP2101 failed to enable ALDO3/ALDO4 output");
    return false;
  }

  // 6. Enable battery voltage ADC & detection
  if (!readRegister(ADC_CHANNEL_CTRL, value) ||
      !writeRegister(ADC_CHANNEL_CTRL, value | 0x01)) {
    Log_error("AXP2101 failed to enable ADC battery measure");
    return false;
  }
  if (!readRegister(BATTERY_DETECTION_CTRL, value) ||
      !writeRegister(BATTERY_DETECTION_CTRL, value | 0x01)) {
    Log_error("AXP2101 failed to enable battery detection");
    return false;
  }

  // Log verification readback
  uint8_t reg80 = 0, reg82 = 0, reg90 = 0, reg94 = 0, reg95 = 0;
  readRegister(DC_ONOFF_DVM_CTRL, reg80);
  readRegister(DC1_VOLTAGE_CTRL, reg82);
  readRegister(LDO_ONOFF_CTRL0, reg90);
  readRegister(ALDO3_VOLTAGE_CTRL, reg94);
  readRegister(ALDO4_VOLTAGE_CTRL, reg95);
  Log_info("AXP2101 initialized: DC_EN=0x%02X DC1_V=0x%02X LDO_EN=0x%02X ALDO3_V=0x%02X ALDO4_V=0x%02X",
           reg80, reg82, reg90, reg94, reg95);

  return true;
}

bool Axp2101::readRegister(uint8_t reg, uint8_t &value) {
  return readRegisters(reg, &value, 1);
}

bool Axp2101::readRegisters(uint8_t reg, uint8_t *values, uint8_t length) {
  if (!begin()) {
    return false;
  }

  Wire.beginTransmission(PHOTO_PAINTER_PMIC_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(PHOTO_PAINTER_PMIC_ADDR, length) != length) {
    return false;
  }

  for (uint8_t i = 0; i < length; ++i) {
    values[i] = Wire.read();
  }
  return true;
}

bool Axp2101::writeRegister(uint8_t reg, uint8_t value) {
  if (!begin()) {
    return false;
  }

  Wire.beginTransmission(PHOTO_PAINTER_PMIC_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool Axp2101::batteryConnected() {
  uint8_t status = 0;
  return readRegister(STATUS1, status) && (status & 0x08) != 0;
}

bool Axp2101::vbusIn() {
  uint8_t status1 = 0;
  uint8_t status2 = 0;
  return readRegister(STATUS1, status1) && readRegister(STATUS2, status2) &&
         (status2 & 0x08) == 0 && (status1 & 0x20) != 0;
}

bool Axp2101::charging() {
  uint8_t status = 0;
  return readRegister(STATUS2, status) && ((status >> 5) & 0x07) == 0x01;
}

uint8_t Axp2101::chargerStatus() {
  uint8_t status = 0;
  return readRegister(STATUS2, status) ? status & 0x07 : 0xff;
}

float Axp2101::batteryVoltage() {
  if (!batteryConnected()) {
    return -1;
  }

  uint8_t voltage[2] = {};
  if (!readRegisters(BATTERY_VOLTAGE_H, voltage, 2)) {
    return -1;
  }

  uint16_t millivolts = ((voltage[0] & 0x1f) << 8) | voltage[1];
  return millivolts * 0.0011f;
}

#else

bool Axp2101::isReady() { return false; }
bool Axp2101::initialize() { return false; }
bool Axp2101::readRegister(uint8_t, uint8_t &) { return false; }
bool Axp2101::readRegisters(uint8_t, uint8_t *, uint8_t) { return false; }
bool Axp2101::writeRegister(uint8_t, uint8_t) { return false; }
bool Axp2101::batteryConnected() { return false; }
bool Axp2101::vbusIn() { return false; }
bool Axp2101::charging() { return false; }
uint8_t Axp2101::chargerStatus() { return 0xff; }
float Axp2101::batteryVoltage() { return -1.0f; }
bool Axp2101::begin() { return false; }

#endif

Axp2101 &axp2101() {
  static Axp2101 instance;
  return instance;
}
