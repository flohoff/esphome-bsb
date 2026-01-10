
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]

bsb_ns = cg.esphome_ns.namespace("bsb")
BSBBusAdapter = bsb_ns.class_(
    "BusAdapter", cg.Component, uart.UARTDevice
)

CONF_BUSADDRESS = "busaddress"

CONFIG_SCHEMA = (
    cv.Schema({cv.GenerateID(): cv.declare_id(BSBBusAdapter)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend({
        cv.Required(CONF_BUSADDRESS): cv.positive_int,
    })
)

async def register_bsb_sensor(var, config):
    parent = await cg.get_variable(config[CONF_BSB_ID])
    cg.add(var.set_bsb_parent(parent))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_busaddress(config[CONF_BUSADDRESS]))
# 
#     for regconfig in config[CONF_REGISTERS]:
#         reg = cg.new_Pvariable(regconfig[CONF_ID])
# 
#         cg.add(reg.set_address(regconfig[CONF_REG_ADDRESS]))
#         cg.add(reg.set_name(regconfig[CONF_REG_NAME]))
#         cg.add(reg.set_access(regconfig[CONF_REG_ACCESS]))
#         cg.add(reg.set_vtype(regconfig[CONF_REG_VTYPE]))
# 
#         if busaddress_id := regconfig.get(CONF_REG_BUSADDRESS):
#             busaddress = await cg.get_variable(busaddress_id)
#             cg.add(reg.set_busaddress(busaddress))
# 
#         cg.add(var.add_register(reg))
# 
#         if regconfig[CONF_REG_TYPE] == BSBRegisterType.sensor:
#             cg.add(var.add_sensor(reg))
#             sens = await sensor.new_sensor(regconfig)
