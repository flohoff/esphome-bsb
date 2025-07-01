
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]


bsb_ns = cg.esphome_ns.namespace("bsb")
BSBBusAdapter = bsb_ns.class_(
    "BSBBusAdapter", cg.Component, uart.UARTDevice
)

bsbreg_ns = cg.esphome_ns.namespace("bsb::BSBRegister")
BSBRegister = bsb_ns.class_(
    "BSBRegister", cg.Component
)

CONF_REG_ADDRESS = "address"
CONF_REG_NAME = "name"
CONF_REG_ACCESS = "access"
CONF_REG_VTYPE = "vtype"

BSBRegisterVType = bsbreg_ns.enum("RegisterType")
RegisterType = {
        "float3-div64-tempC": BSBRegisterVType.float3div64tempC
}

BSBRegisterAccess = bsbreg_ns.enum("RegisterAccess")
RegisterAccess = {
        "ro": BSBRegisterAccess.ro,
        "rw": BSBRegisterAccess.rw
}

BSB_REGISTERS_SCHEMA = cv.Schema({
        cv.GenerateID(): cv.declare_id(BSBRegister),
        cv.Required(CONF_REG_NAME): cv.string,
        cv.Required(CONF_REG_ADDRESS): cv.positive_int,
        cv.Required(CONF_REG_VTYPE): cv.enum(RegisterType),
        cv.Required(CONF_REG_ACCESS): cv.enum(RegisterAccess)
    })

CONF_REGISTERS = "registers"
CONFIG_SCHEMA = (
    cv.Schema({cv.GenerateID(): cv.declare_id(BSBBusAdapter)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend({
        cv.Optional(CONF_REGISTERS): cv.ensure_list(BSB_REGISTERS_SCHEMA)
    })
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    for regconfig in config[CONF_REGISTERS]:
        reg = cg.new_Pvariable(regconfig[CONF_ID])

        cg.add(reg.set_address(regconfig[CONF_REG_ADDRESS]))
        cg.add(reg.set_name(regconfig[CONF_REG_NAME]))
        cg.add(reg.set_access(regconfig[CONF_REG_ACCESS]))
        cg.add(reg.set_type(regconfig[CONF_REG_VTYPE]))

        cg.add(var.add_register(reg))

