#pragma once

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/rgbct/rgbct_light_output.h"
#include "esphome/components/output/float_output.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace neewerlight {

namespace espbt = esp32_ble_tracker;

static const char *const SERVICE_UUID = "69400001-B5A3-F393-E0A9-E50E24DCCA99";
static const char *const CHARACTERISTIC_UUID = "69400002-B5A3-F393-E0A9-E50E24DCCA99";
static const int MSG_MAX_SIZE = 10;  // size of msg_ string to reserve in bytes (uint8_t*).
static const float COLD_WHITE = 178.6;  // 5600 K
static const float WARM_WHITE = 312.5;  // 3200 K

class NeewerBLEOutput : public Component, public output::FloatOutput, public ble_client::BLEClientNode {
 public:
    void dump_config() override;
    void loop() override {}
    float get_setup_priority() const override {
      return setup_priority::DATA;
    }
    void set_service_uuid16(uint16_t uuid) { this->service_uuid_ = espbt::ESPBTUUID::from_uint16(uuid); }
    void set_service_uuid32(uint32_t uuid) { this->service_uuid_ = espbt::ESPBTUUID::from_uint32(uuid); }
    void set_service_uuid128(uint8_t *uuid) { this->service_uuid_ = espbt::ESPBTUUID::from_raw(uuid); }
    void set_service_uuid_str(const char *uuid) { this->service_uuid_ = espbt::ESPBTUUID::from_raw(uuid); }
    void set_char_uuid16(uint16_t uuid) { this->char_uuid_ = espbt::ESPBTUUID::from_uint16(uuid); }
    void set_char_uuid32(uint32_t uuid) { this->char_uuid_ = espbt::ESPBTUUID::from_uint32(uuid); }
    void set_char_uuid128(uint8_t *uuid) { this->char_uuid_ = espbt::ESPBTUUID::from_raw(uuid); }
    void set_char_uuid_str(const char *uuid) { this->char_uuid_ = espbt::ESPBTUUID::from_raw(uuid); }
    void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                            esp_ble_gattc_cb_param_t *param) override;
    void set_require_response(bool response) { this->require_response_ = response; }

    NeewerBLEOutput();

  protected:
    void write_state(float state) override;
    void build_msg_with_checksum();
    void msg_clear();
    void orig_msg_clear();
    bool require_response_;
    espbt::ESPBTUUID service_uuid_;
    espbt::ESPBTUUID char_uuid_;
    espbt::ClientState client_state_;

    const char* const TAG = "neewer_ble_output";

    uint8_t *msg_;
    uint8_t msg_len_;
    uint8_t *orig_msg_;
    uint8_t orig_msg_len_;
    bool command_block_ = false;

    const uint8_t command_prefix_ = 0x78;
    const uint8_t rgb_prefix_ = 0x86;
    const uint8_t ctwb_prefix_ = 0x87;
    const uint8_t effect_prefix_ = 0x88;

};

class NeewerStateOutput : public output::FloatOutput {
  protected:
    void write_state(float state) override;
};

class NeewerRGBCTLightOutput : public rgbct::RGBCTLightOutput, public NeewerBLEOutput {
  public:
    NeewerRGBCTLightOutput();

    void dump_config() override;
    void rgb_to_hsb(float red, float green, float blue, int *hue, uint8_t *saturation, uint8_t *brightness);
    
  protected:
    float old_red_ = 0.0;
    float old_green_ = 0.0;
    float old_blue_ = 0.0;
    float old_white_brightness_ = 0.0;
    float old_color_temperature_ = 0.0;

    const char* const TAG = "neewer_rgbct_light_output";

    bool did_rgb_change(float red, float green, float blue);
    bool did_ctwb_change(float color_temperature, float white_brightness);
    bool did_only_wb_change(float color_temperature, float white_brightness);
    void prepare_ctwb_msg(float color_temperature, float white_brightness);
    void prepare_rgb_msg(float red, float green, float blue);
    void prepare_wb_msg(float white_brightness);
    void set_old_rgbct(float red, float green, float blue, float color_temperature, float white_brightness);
    void write_state(light::LightState *state) override;
};

}  // namespace esphome
}  // namespace neewerlight

#endif  // USE_ESP32