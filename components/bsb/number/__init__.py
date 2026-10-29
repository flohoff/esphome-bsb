import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
        CONF_ID,
        CONF_MAX_VALUE,
        CONF_MIN_VALUE,
        CONF_STEP
)

from .. import bsb_ns

BsbNumber = bsb_ns.class_("Number", number.Number, cg.Component)

CONF_BSB_ID = "bsb_id"
CONF_ADDRESS = "address"
CONF_VTYPE = "vtype"

NumberRegisterVType = bsb_ns.enum("NumberRegisterVType")
NumberRegisterVType = {
        "float3-div64-tempC": NumberRegisterVType.VT_float3div64tempC
}

CONFIG_SCHEMA = number.number_schema(BsbNumber, unit_of_measurement="°C").extend(
        {
            cv.GenerateID(CONF_BSB_ID): cv.use_id(bsb_ns),
            cv.Required(CONF_ADDRESS): cv.hex_int,
            cv.Required(CONF_VTYPE): cv.enum(NumberRegisterVType),
            cv.Required(CONF_MIN_VALUE): cv.float_,
            cv.Required(CONF_MAX_VALUE): cv.float_,
            cv.Optional(CONF_STEP, default=0.5): cv.float_,
        }
    ).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    bsb = await cg.get_variable(config[CONF_BSB_ID])

    var = cg.new_Pvariable(
        config[CONF_ID], bsb, config[CONF_ADDRESS], config[CONF_VTYPE]
    )
    await cg.register_component(var, config)
    await number.register_number(
        var, config,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=config[CONF_STEP]
    )
