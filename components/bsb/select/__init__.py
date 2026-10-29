import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
        CONF_ID
)
from typing import Any, Callable

from .. import bsb_ns

BsbSelect = bsb_ns.class_("Select", select.Select, cg.Component)

CONF_BSB_ID = "bsb_id"
CONF_ADDRESS = "address"
CONF_VTYPE = "vtype"
CONF_OPTIONSMAP = "optionsmap"


def ensure_option_map() -> Callable[[Any], dict[str, int]]:
    """Validate the {label: BSB value} mapping; dict order defines the
    dropdown order and the index<->value mapping passed to the component."""
    def validator(value: Any) -> dict[str, int]:
        cv.check_not_templatable(value)
        option = cv.All(cv.string_strict)
        mapping = cv.All(cv.int_range(0, 255))
        options_map_schema = cv.Schema({option: mapping})
        return options_map_schema(value)
    return validator


SelectRegisterVType = bsb_ns.enum("SelectRegisterVType")
SelectRegisterVType = {
        "boolonoff": SelectRegisterVType.VT_boolonoff,
        "enum": SelectRegisterVType.VT_enum
}

CONFIG_SCHEMA = select.select_schema(BsbSelect).extend(
        {
            cv.GenerateID(CONF_BSB_ID): cv.use_id(bsb_ns),
            cv.Required(CONF_ADDRESS): cv.hex_int,
            cv.Required(CONF_VTYPE): cv.enum(SelectRegisterVType),
            cv.Required(CONF_OPTIONSMAP): ensure_option_map(),
        }
    ).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    options_map = config[CONF_OPTIONSMAP]

    bsb = await cg.get_variable(config[CONF_BSB_ID])

    var = cg.new_Pvariable(
        config[CONF_ID], bsb, config[CONF_ADDRESS], config[CONF_VTYPE],
        list(options_map.values())
    )
    await cg.register_component(var, config)
    await select.register_select(var, config, options=list(options_map.keys()))
