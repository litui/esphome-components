#include "nyowu_listener.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace nyowu_ble {

static const char *const TAG = "nyowu_ble";

bool NyowuListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.get_name() == "YOWU-SELKIRK-3G") {
    ESP_LOGD(TAG, "Found Yowu BLE Device %s (MAC: %s)", device.get_name().c_str(), device.address_str().c_str());
    return true;
  }

  return false;
}

}  // namespace nyowu_ble
}  // namespace esphome

#endif
