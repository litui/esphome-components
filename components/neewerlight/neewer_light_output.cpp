#include "neewer_light_output.h"

#ifdef USE_ESP32

namespace esphome {
namespace neewerlight {

void NeewerBLEOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "Neewer BLE Output:");
  ESP_LOGCONFIG(TAG, "  MAC address        : %s", this->parent_->address_str().c_str());
  ESP_LOGCONFIG(TAG, "  Service UUID       : %s", this->service_uuid_.to_string().c_str());
  ESP_LOGCONFIG(TAG, "  Characteristic UUID: %s", this->char_uuid_.to_string().c_str());
  LOG_BINARY_OUTPUT(this);
};

void NeewerBLEOutput::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                             esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT:
      this->client_state_ = espbt::ClientState::ESTABLISHED;
      ESP_LOGW(TAG, "[%s] Connected successfully!", this->char_uuid_.to_string().c_str());
      break;
    case ESP_GATTC_DISCONNECT_EVT:
      ESP_LOGW(TAG, "[%s] Disconnected", this->char_uuid_.to_string().c_str());
      this->client_state_ = espbt::ClientState::IDLE;
      break;
    case ESP_GATTC_WRITE_CHAR_EVT: {
      if (param->write.status == 0) {
        break;
      }

      auto *chr = this->parent()->get_characteristic(this->service_uuid_, this->char_uuid_);
      if (chr == nullptr) {
        ESP_LOGW(TAG, "[%s] Characteristic not found.", this->char_uuid_.to_string().c_str());
        break;
      }
      if (param->write.handle == chr->handle) {
        ESP_LOGW(TAG, "[%s] Write error, status=%d", this->char_uuid_.to_string().c_str(), param->write.status);
      }
      break;
    }
    default:
      break;
  }
};

void NeewerBLEOutput::write_state(float state) {
  if (this->client_state_ != espbt::ClientState::ESTABLISHED) {
    ESP_LOGW(TAG, "[%s] Not connected to BLE client.  State update can not be written.",
             this->char_uuid_.to_string().c_str());
    return;
  }

  auto *chr = this->parent()->get_characteristic(this->service_uuid_, this->char_uuid_);
  if (chr == nullptr) {
    ESP_LOGW(TAG, "[%s] Characteristic not found.  State update can not be written.",
             this->char_uuid_.to_string().c_str());
    return;
  }

  // this->msg_ must be prepared prior to running this function

  ESP_LOGD(TAG, "Message length before write to light: %i", this->msg_len_);
  if(!this->msg_ && !this->msg_len_) {
    ESP_LOGI(TAG, "Could not send message to light - 0 length message.");
  } else if(chr != nullptr) {
    ESP_LOGI(TAG, "Attempting to write colour command %i bytes, state value: %f", this->msg_len_, state);
    for (int i = 0; i < this->msg_len_; i++) {
      ESP_LOGV(TAG, "Reading byte %i/%i, value %i", i, this->msg_len_-1, this->msg_[i]);
    }
    chr->write_value(this->msg_, this->msg_len_, ESP_GATT_WRITE_TYPE_RSP);
  } else {
    ESP_LOGD(TAG, "Writing colour command failed: could not get characteristic from parent.");
  }
};

// Prepare the msg_ byte array and append checksum
// Algorithm borrowed from https://github.com/keefo/NeewerLite (MIT Licensed)
void NeewerBLEOutput::build_msg_with_checksum(){
  this->msg_clear();

  int checksum = 0;
  for (int i = 0; i < this->orig_msg_len_; i++) {
    this->msg_[i] = this->orig_msg_[i] < 0 ? (uint8_t)(this->orig_msg_[i] + 0x100) : (uint8_t) this->orig_msg_[i];
    checksum = checksum + this->msg_[i];
  }
  this->msg_[this->orig_msg_len_] = (uint8_t) (checksum & 0xFF);
  this->msg_len_ = this->orig_msg_len_ + 1;
  ESP_LOGD(TAG, "Message length at end of build_msg_with_checksum: %i", this->msg_len_);
};

void NeewerBLEOutput::msg_clear() {
  for (int i = 0; i < MSG_MAX_SIZE; i++) {
    this->msg_[i] = 0;
  }
  this->msg_len_ = 0;
};

void NeewerBLEOutput::orig_msg_clear() {
  for (int i = 0; i < MSG_MAX_SIZE; i++) {
    this->orig_msg_[i] = 0;
  }
  this->orig_msg_len_ = 0;
};

NeewerBLEOutput::NeewerBLEOutput() {
  msg_ = (uint8_t*) malloc (MSG_MAX_SIZE);
  this->msg_clear();
  orig_msg_ = (uint8_t*) malloc (MSG_MAX_SIZE);
  this->orig_msg_clear();
};

void NeewerRGBCTLightOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "Neewer RGBCT Light Output:");
  ESP_LOGCONFIG(TAG, "  MAC address        : %s", this->parent_->address_str().c_str());
  ESP_LOGCONFIG(TAG, "  Service UUID       : %s", this->service_uuid_.to_string().c_str());
  ESP_LOGCONFIG(TAG, "  Characteristic UUID: %s", this->char_uuid_.to_string().c_str());
  ESP_LOGCONFIG(TAG, "  Require Response   : %s", this->require_response_ ? "True" : "False");
  ESP_LOGCONFIG(TAG, "  Colour Temperatures: %.2f - %.2f", 
                this->cold_white_temperature_, this->warm_white_temperature_);
  ESP_LOGCONFIG(TAG, "  Colour Interlock   : %s", this->color_interlock_ ? "On" : "Off");
  LOG_BINARY_OUTPUT(this);
};

bool NeewerRGBCTLightOutput::did_rgb_change(float red, float green, float blue) {
  if (red != this->old_red_) return true;
  if (green != this->old_green_) return true;
  if (blue != this->old_blue_) return true;
  return false;
};

bool NeewerRGBCTLightOutput::did_ctwb_change(float color_temperature, float white_brightness) {
  if (color_temperature != this->old_color_temperature_) return true;
  if (white_brightness != this->old_white_brightness_) return true;
  return false;
};

bool NeewerRGBCTLightOutput::did_only_wb_change(float color_temperature, float white_brightness) {
  if (color_temperature != this->old_color_temperature_) return false;
  if (white_brightness != this->old_white_brightness_) return true;
  return false;  // Neither changed.
};

void NeewerRGBCTLightOutput::prepare_ctwb_msg(float color_temperature, float white_brightness) {
  uint8_t ct = (uint8_t) abs((color_temperature * 24.0) - 56.0);
  uint8_t wb = (uint8_t) (white_brightness * 100.0);

  this->orig_msg_clear();

  // Prepare string for write_state.
  this->orig_msg_[0] = this->command_prefix_;        // 0x78
  this->orig_msg_[1] = this->ctwb_prefix_;           // 0x86
  this->orig_msg_[2] = 2;                            // Byte Count = 2 for ctwb
  this->orig_msg_[3] = wb;                           // brightness 0x00 - 0x64
  this->orig_msg_[4] = ct;                           // color_temp 0x20 - 0x38
  this->orig_msg_len_ = 5;

  NeewerBLEOutput::build_msg_with_checksum();
};

// For whatever reason, the RGB660 will only allow brightness alone if CT hasn't changed
void NeewerRGBCTLightOutput::prepare_wb_msg(float white_brightness) {
  uint8_t wb = (uint8_t) (white_brightness * 100.0);

  this->orig_msg_clear();

  // Prepare string for write_state.
  this->orig_msg_[0] = this->command_prefix_;        // 0x78
  this->orig_msg_[1] = this->ctwb_prefix_;           // 0x86
  this->orig_msg_[2] = 1;                            // Byte Count = 2 for ctwb
  this->orig_msg_[3] = wb;                           // brightness 0x00 - 0x64
  this->orig_msg_len_ = 4;
  
  NeewerBLEOutput::build_msg_with_checksum();
};

void NeewerRGBCTLightOutput::prepare_rgb_msg(float red, float green, float blue) {
  int hue;
  uint8_t saturation;
  uint8_t brightness;

  // Surprise, the "RGB" light isn't actually RGB!
  this->rgb_to_hsb(red, green, blue, &hue, &saturation, &brightness);

  this->orig_msg_clear();

  // Prepare string for write_state.
  this->orig_msg_[0] = this->command_prefix_;        // 0x78
  this->orig_msg_[1] = this->rgb_prefix_;            // 0x86
  this->orig_msg_[2] = 4;                            // Byte Count = 4 for RGB
  this->orig_msg_[3] = (int) (hue & 0xFF);           // hue int, split across two 8 bit ints
  this->orig_msg_[4] = (int) ((hue & 0xFF00) >> 8);  // hue Shift 8 bits over
  this->orig_msg_[5] = saturation;                   // saturation 0x00 - 0x64
  this->orig_msg_[6] = brightness;                   // brightness 0x00 - 0x64
  this->orig_msg_len_ = 7;
  
  NeewerBLEOutput::build_msg_with_checksum();
};

// Algorithm cobbled together from various corners of the internet. Works great!
void NeewerRGBCTLightOutput::rgb_to_hsb(float red, float green, float blue,
                                        int *hue, uint8_t *saturation, uint8_t *brightness) {
  float max_value = 0.0;
  float min_value = 0.0;
  float diff_value = 0.0;

  ESP_LOGD(TAG, "Attempting RGB colour conversion from %f, %f, %f...", red, green, blue);

  // Determine maximum
  max_value = red < green ? green : red;
  max_value = max_value < blue ? blue : max_value;

  // Determine minimum
  min_value = red < green ? red : green;
  min_value = min_value < blue ? min_value : blue;

  // Determine difference
  diff_value = max_value - min_value;

  // Calculate value
  *brightness = (uint8_t) (max_value * 100);

  // Calculate saturation
  if (max_value == 0) {
    *saturation = 0;
  } else {
    *saturation = (uint8_t) ((diff_value / max_value) * 100);
  }

  // Math the hue
  if (diff_value == 0) {
    *hue = 0;
  } else {
    if (max_value == red) {
      *hue = (int) (60 * ((float) remainder(((green - blue) / diff_value), 6.0)));
    } else if (max_value == green) {
      *hue = (int) (60 * (((blue - red) / diff_value) + 2.0));
    } else {
      *hue = (int) (60 * (((red - green) / diff_value) + 4.0));
    }
    if (*hue < 0) {
      *hue = *hue + 360;
    }
    if (*hue >= 360) {
      *hue = *hue - 360;
    }
  }

  ESP_LOGD(TAG, "Converted RGB to HSB values %i, %i, %i", *hue, *saturation, *brightness);
};

void NeewerRGBCTLightOutput::write_state(light::LightState *state) {
  // Call original write state to set new values for each state.
  float red, green, blue, color_temperature, white_brightness;

  state->current_values_as_rgbct(&red, &green, &blue, &color_temperature, &white_brightness);

  // Prep values for logic to determine which mode we need to change
  bool rgb_changed = this->did_rgb_change(red, green, blue);
  bool ctwb_changed = this->did_ctwb_change(color_temperature, white_brightness);
  bool only_wb_changed = this->did_only_wb_change(color_temperature, white_brightness);
  bool rgb_is_zero = (red == 0.0 && green == 0.0) && blue == 0.0;
  bool wb_is_zero = white_brightness == 0.0;
  bool nothing_changed = !rgb_changed && !ctwb_changed;

  // The following logic is to handle different message modes on the NW660RGB
  // in contention with the colour interlock mode which sets the inactive mode
  // to zeroes.
  if (rgb_changed && wb_is_zero) {
    ESP_LOGD(TAG, "RGB value changed while WB == 0");
    this->prepare_rgb_msg(red, green, blue);
  } else if (ctwb_changed && rgb_is_zero) {
    ESP_LOGD(TAG, "CTWB value changed while RGB == 0");
      if (only_wb_changed) {
        ESP_LOGD(TAG, "Only WB changed.");
        this->prepare_wb_msg(white_brightness);
      } else {
        ESP_LOGD(TAG, "CT and WB changed.");
        this->prepare_ctwb_msg(color_temperature, white_brightness);
      }
  } else {
    if (nothing_changed && rgb_is_zero) {
      // If nothing changed while in CTWB mode, the RGB values will
      // end up all zero effectively turning off the light if sent.
      // Instead bail and don't write anything to the light.
      ESP_LOGD(TAG, "Nothing changed and RGB == 0, bailing.");
      return;
    }
    ESP_LOGD(TAG, "Executing RGB fallback.");
    // Default to setting RGB if for whatever reason both are non-zero, or both
    // are zero without having triggered a change.
    this->prepare_rgb_msg(red, green, blue);
  }

  // Message having been prepared, we can send it off into the sunset.
  NeewerBLEOutput::write_state(1.0);

  // We're probably done with the old values now, so let's change them up.
  this->old_red_ = red;
  this->old_green_ = green;
  this->old_blue_ = blue;
  this->old_color_temperature_ = color_temperature;
  this->old_white_brightness_ = white_brightness;

  // Do whatever else setting the current levels individually accomplishes
  this->red_->set_level(red);
  this->green_->set_level(green);
  this->blue_->set_level(blue);
  this->color_temperature_->set_level(color_temperature);
  this->white_brightness_->set_level(white_brightness);
};

NeewerRGBCTLightOutput::NeewerRGBCTLightOutput() {
  this->set_service_uuid_str(SERVICE_UUID);
  this->set_char_uuid_str(CHARACTERISTIC_UUID);

  // RGBCT-specific light settings
  this->set_red(new NeewerStateOutput());
  this->set_green(new NeewerStateOutput());
  this->set_blue(new NeewerStateOutput());
  this->set_color_temperature(new NeewerStateOutput());
  this->set_white_brightness(new NeewerStateOutput());
  this->set_cold_white_temperature(COLD_WHITE);
  this->set_warm_white_temperature(WARM_WHITE);

  NeewerBLEOutput();

  // Assume colour interlock is on as the NW660 definitely treats RGB and CT as separate modes
  // this->set_color_interlock(true);

  // Generic light settings
  // this->set_default_transition_length(0);
  // this->set_gamma_correct(1.0);

  // BLE-specific settings
  this->set_require_response(true);
};

void NeewerStateOutput::write_state(float state) {
  // Do nothing with the written state
};

}  // namespace neewerlight
}  // namespace esphome

#endif  // USE_ESP32
