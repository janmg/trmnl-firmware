#include <power.h>

#ifdef INCLUDE_AXP2101_POWER
#include <axp2101.h>

UsbStatus AXP2101Power::usbStatus() {
  if (!axp2101().isReady()) {
    return UsbStatus::UNKNOWN;
  }
  return axp2101().vbusIn() ? UsbStatus::CONNECTED : UsbStatus::DISCONNECTED;
}

ChargingStatus AXP2101Power::chargingStatus() {
  if (!axp2101().isReady()) {
    return ChargingStatus::UNKNOWN;
  }
  return axp2101().charging() ? ChargingStatus::CHARGING : ChargingStatus::NOT_CHARGING;
}
#endif // INCLUDE_AXP2101_POWER
