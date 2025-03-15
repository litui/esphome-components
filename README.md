# Litui's ESPHome Components

> [ESPHome](https://esphome.io/) is a system to control your ESP8266/ESP32 by simple yet powerful configuration files and control them remotely through Home Automation systems.

ESPHome, happily, also supports [3rd party components](https://esphome.io/components/external_components.html).

To add all my components at the current release version to any of your ESPHome-configured devices, use the following syntax in your ESP32 device's .yaml file:

```yaml
external_components:
- source: github://litui/esphome-components@v1.0.1
```

To add the main development tree instead, replace `v1.0.1` with `main`.

## NeewerLight Support

### Kudos

* Big props to Xu Lian ([keefo](https://github.com/keefo)) for their [NeewerLite](https://github.com/keefo/NeewerLite) OS X swift app which helped me through understanding the particulars of Neewer's strange BLE approach and quirks. My ESPHome component is a reimplementation of parts of their approach in C++.

### Rationale

Why make a Neewer Light BLE light component in ESPHome?
1. The official Neewer app is only available on iOS and Android, and is, in a word, terrible at its job. I have never been able to maintain connection to the lights using my Android phone. Additionally their app requires a Neewer login connected to my email address.
2. I don't have a personal OS X system or I would probably have just used keefo's NeewerLite.
3. Trying to control BLE devices from Windows was like pulling teeth and I couldn't for the life of me get it to stay connected.
4. I had similar luck trying to figure out a full-featured BLE implementation in Python on Raspbian that would serve my purposes.
5. I've recently been playing with Home Assistant, ESPHome, and ESP32 boards, and discovered (much to my amazement) that the ESP32's BLE stack/hardware can actually stay connected to the Neewer-RGB660.

### Supported Devices
The only device I've spent any time trying to support is my own Neewer RGB660. So the list, if you insist on one, looks like this:

* Neewer RGB660

Other Neewer RGB/Colour Temperature devices will likely work with all or a subset of these BLE commands but I haven't tailored the support. Feel free to fork and try things out.

### Usage

To add only the NeewerLight support to your ESPHome instance (in case I add more components later), use the following syntax in your ESP32 device's .yaml file.

```yaml
external_components:
- source: github://litui/esphome-components@v1.0.0
  components: [ neewerlight_ble, neewerlight ]
```

Similar to the [Airthings BLE implementation](https://github.com/esphome/esphome/tree/dev/esphome/components/airthings_ble), the `neewerlight_ble` component will simply draw your attention to the Neewer-RGB660 devices detected by `esp32_ble_tracker`. From there, you'll need to copy/paste or otherwise record the Bluetooth MAC addresses of your devices. After that point, `neewerlight_ble` becomes unnecessary.

To control your light in Home Assistant, you'll need to set up the `ble_client` with your MAC address and an ID, then set up a `light` block with the platform `neewerlight`.

It should look as follows:

```yaml
esp32_ble_tracker:

### Commented out after initial detection:
# neewerlight_ble:

### Added the following blocks with the detected
### mac addresses:
ble_client:
- mac_address: AA:AA:AA:AA:AA:AA
  id: nw660_ble_1
- mac_address: BB:BB:BB:BB:BB:BB
  id: nw660_ble_2

light:
- platform: neewerlight
  name: "NW660 RGB Light 1"
  ble_client_id: nw660_ble_1
  gamma_correct: 1.0
  default_transition_length: 0s

- platform: neewerlight
  name: "NW660 RGB Light 2"
  ble_client_id: nw660_ble_2
  gamma_correct: 1.0
  default_transition_length: 0s
```

Set `gamma_correct` as you desire; 1.0 makes the most sense for me. I **highly** recommend setting `default_transition_length` to `0s` to prevent spamming the wonky Neewer BLE implementation with far too many instructions at once. I've had my lamps suddenly stop responding to requests when overwhelmed (they overwhelm easily) and I needed to physically turn them off and on again.

### Todo:

I'm still working on learning the ropes of the ESPHome Python validations. The current set is not very strict.
