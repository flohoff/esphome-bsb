import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
        DEVICE_CLASS_TEMPERATURE,
        STATE_CLASS_MEASUREMENT,
        UNIT_CELSIUS,
        CONF_ID
)

from .. import bsb_ns

BsbSensor = bsb_ns.class_("Sensor", sensor.Sensor, cg.Component)

CONF_BSB_ID = "bsb_id"
CONF_ADDRESS = "address"
CONF_VTYPE = "vtype"

BSBRegisterVType = bsb_ns.enum("RegisterVType")
RegisterVType = {
        "float3-div64-tempC": BSBRegisterVType.VT_float3div64tempC,
        "percent": BSBRegisterVType.VT_percent
}

CONFIG_SCHEMA = sensor.sensor_schema().extend(
        {
            cv.GenerateID(): cv.declare_id(BsbSensor),
            cv.GenerateID(CONF_BSB_ID): cv.use_id(bsb_ns),
            cv.Required(CONF_ADDRESS): cv.hex_int,
            cv.Required(CONF_VTYPE): cv.enum(RegisterVType),
        }
    ).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    bsb = await cg.get_variable(config[CONF_BSB_ID])

    var = cg.new_Pvariable(
        config[CONF_ID], bsb, config[CONF_ADDRESS], config[CONF_VTYPE]
    )
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)


