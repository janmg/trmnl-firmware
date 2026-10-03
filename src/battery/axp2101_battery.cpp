#include <axp2101.h>
#include <battery.h>

#ifdef INCLUDE_AXP2101
float AXP2101Battery::readVoltage() {
  return axp2101().batteryVoltage();
}
#endif
