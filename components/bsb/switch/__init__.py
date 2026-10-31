import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_ID

from .. import bsb_ns

BSBSwitch = bsb_ns.class_("Switch", switch.Switch, cg.Component)

CONF_BSB_ID = "bsb_id"
CONF_ADDRESS = "address"
CONF_VTYPE = "vtype"

SwitchRegisterVType = bsb_ns.enum("SwitchRegisterVType")
SwitchRegisterVType = {
        "boolonoff": SwitchRegisterVType.VT_boolonoff
}

CONFIG_SCHEMA = (
  switch.switch_schema(BSBSwitch)
  .extend(
	  {
          cv.GenerateID(): cv.declare_id(BSBSwitch),
          cv.GenerateID(CONF_BSB_ID): cv.use_id(bsb_ns),
          cv.Required(CONF_ADDRESS): cv.hex_int,
          cv.Required(CONF_VTYPE): cv.enum(SwitchRegisterVType),
	  }
  )
  .extend(cv.polling_component_schema("60s"))
)

async def to_code(config):
    bsb = await cg.get_variable(config[CONF_BSB_ID])

    var = await switch.new_switch(config)
    cg.add(var.set_busadapter(bsb))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    await cg.register_component(var, config)

