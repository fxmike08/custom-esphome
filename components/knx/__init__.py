import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_UART_ID, CONF_USE_ADDRESS, CONF_LAMBDA


CODEOWNERS = ["@fxmike@gmail.com"]
DEPENDENCIES = ["uart"]

knx_ns = cg.esphome_ns.namespace("knx")
knx_component = knx_ns.class_("KnxComponent", cg.Component, uart.UARTDevice)

CONF_LISTENING_ADDRESSES = "listen_group_address"
CONF_SERIAL_TIMEOUT = "serial_timeout"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(knx_component),
            cv.Required(CONF_USE_ADDRESS): cv.string,
            cv.Required(CONF_LAMBDA): cv.returning_lambda,
            cv.Optional(CONF_LISTENING_ADDRESSES, default=[]): cv.ensure_list(
                cv.string_strict
            ),
            cv.Optional(CONF_SERIAL_TIMEOUT, default=1000): cv.uint32_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    uart_component = await cg.get_variable(config[CONF_UART_ID])
    var = cg.new_Pvariable(config[CONF_ID], uart_component)

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(knx_component, "knx")], return_type=cg.void
        )
        cg.add(var.set_lambda_writer(lambda_))

    cg.add(var.set_use_address(config[CONF_USE_ADDRESS]))
    cg.add(var.set_serial_timeout(config[CONF_SERIAL_TIMEOUT]))

    for addreses in config[CONF_LISTENING_ADDRESSES]:
        cg.add(var.add_listen_group_address(addreses))

    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
