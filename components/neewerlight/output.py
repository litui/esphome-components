import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, output
from esphome.const import (
    CONF_OUTPUT_ID,
)

DEPENDENCIES = ["ble_client"]
IS_PLATFORM_COMPONENT = True

neewerlight_ns = cg.esphome_ns.namespace("neewerlight")
NeewerBLEOutput = neewerlight_ns.class_(
    "NeewerBLEOutput", cg.Component, output.FloatOutput, ble_client.BLEClientNode
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(NeewerBLEOutput),
            cv.Required(ble_client.CONF_BLE_CLIENT_ID): cv.use_id(ble_client.BLEClient),
        }
    )
    .extend(output.FLOAT_OUTPUT_SCHEMA)
    .extend(ble_client.BLE_CLIENT_SCHEMA)
)


# async def to_code(config):
#     var = cg.new_Pvariable(config[CONF_ID])
#     await cg.register_component(var, config)

#     await ble_client.register_ble_node(var, config)
