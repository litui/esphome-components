import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, light
from esphome.components.rgbct import light as rgbct_light
from esphome.components.neewerlight import output as nw_output
from esphome.const import (
    CONF_COLOR_INTERLOCK,
    CONF_GAMMA_CORRECT,
    CONF_NAME,
    CONF_OUTPUT_ID,
)

DEPENDENCIES = ["ble_client"]
AUTO_LOAD = ["output", "rgbct"]
IS_PLATFORM_COMPONENT = True

neewerlight_ns = cg.esphome_ns.namespace("neewerlight")
rgbct_ns = cg.esphome_ns.namespace("rgbct")

NeewerRGBCTLightOutput = neewerlight_ns.class_(
    "NeewerRGBCTLightOutput",
    rgbct_light.RGBCTLightOutput,
    nw_output.NeewerBLEOutput,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(NeewerRGBCTLightOutput),
            cv.Required(CONF_NAME): cv.string,
            cv.Required(ble_client.CONF_BLE_CLIENT_ID): cv.use_id(ble_client.BLEClient),
            cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
            cv.Optional(CONF_COLOR_INTERLOCK, default=True): cv.boolean,
        }
    )
    .extend(cv.ENTITY_BASE_SCHEMA)
    .extend(light.RGB_LIGHT_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    await ble_client.register_ble_node(var, config)

    cg.add(var.set_color_interlock(config[CONF_COLOR_INTERLOCK]))
