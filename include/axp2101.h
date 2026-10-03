#pragma once

#include <stdint.h>

class Axp2101 {
public:
  bool isReady();
  bool initialize();
  bool readRegister(uint8_t reg, uint8_t &value);
  bool readRegisters(uint8_t reg, uint8_t *values, uint8_t length);
  bool writeRegister(uint8_t reg, uint8_t value);

  bool batteryConnected();
  bool vbusIn();
  bool charging();
  uint8_t chargerStatus();
  float batteryVoltage();

private:
  bool begin();
  bool _initialized = false;
  bool _available = false;
};

Axp2101 &axp2101();
