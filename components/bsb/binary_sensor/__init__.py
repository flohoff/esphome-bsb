import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
        CONF_ID
)

from .. import bsb_ns

BsbSensor = bsb_ns.class_("BinarySensor", binary_sensor.BinarySensor, cg.Component)

CONF_BSB_ID = "bsb_id"
CONF_ADDRESS = "address"
CONF_VTYPE = "vtype"

BinaryRegisterVType = bsb_ns.enum("BinaryRegisterVType")
BinaryRegisterVType = {
        "boolonoff": BinaryRegisterVType.VT_boolonoff
}

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema().extend(
        {
            cv.GenerateID(): cv.declare_id(BsbSensor),
            cv.GenerateID(CONF_BSB_ID): cv.use_id(bsb_ns),
            cv.Required(CONF_ADDRESS): cv.hex_int,
            cv.Required(CONF_VTYPE): cv.enum(BinaryRegisterVType),
        }
    ).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    bsb = await cg.get_variable(config[CONF_BSB_ID])

    var = cg.new_Pvariable(
        config[CONF_ID], bsb, config[CONF_ADDRESS], config[CONF_VTYPE]
    )
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)
