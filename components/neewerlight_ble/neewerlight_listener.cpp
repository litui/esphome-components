#include "neewerlight_listener.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace neewerlight_ble {

static const char *const TAG = "neewerlight_ble";

bool NeewerLightListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.get_name() == "NEEWER-RGB660") {
    ESP_LOGD(TAG, "Found Neewer Light %s (MAC: %s)", device.get_name().c_str(), device.address_str().c_str());
    return true;
  }

  return false;
}

}  // namespace neewerlight_ble
}  // namespace esphome

#endif
