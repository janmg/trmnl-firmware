#include <battery.h>
#include <config.h>

#if defined(BOARD_TRMNL_X)
static BQ27427Battery batteryInstance;
#elif defined(BOARD_WAVESHARE_PHOTOPAINTER)
static AXP2101Battery batteryInstance;
#else
static ADCBattery batteryInstance;
#endif

#ifdef INCLUDE_BQ27427
BQ27427Battery &battery() { return batteryInstance; }
#elif defined(INCLUDE_AXP2101)
AXP2101Battery &battery() { return batteryInstance; }
#else
BaseBattery &battery() { return batteryInstance; }
#endif
