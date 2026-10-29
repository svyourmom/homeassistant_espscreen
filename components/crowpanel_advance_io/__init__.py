"""Elecrow CrowPanel Advance 7" (ESP32-S3) I2C board controller and early GT911 touch reset.

The CrowPanel Advance 7" has no ESP32 GPIO for its backlight or GT911 reset line; both sit behind an auxiliary I2C
board controller on touch_bus:
- V1.2-V1.5: STC8H1K28 auxiliary MCU at 0x30 (0 = full light .. 245 = dark; 250 pulses GT911 reset).
- V1.0-V1.1: PCA9557 / TCA9534 expander at 0x18 (IO0/IO1 = backlight enable, IO2 = GT911 reset).

GT911Touchscreen::setup() runs at setup_priority::DATA (600.0f) and marks the touchscreen failed if the GT911 has not
been reset yet. Because packages/ may not define an esphome: on_boot: of their own, this component runs at
setup_priority::IO (900.0f - after I2C at BUS = 1000.0f, before GT911 at DATA = 600.0f) to pulse the GT911 reset while
holding GPIO1 (INT) low so the GT911 answers at 0x5D, and turns the backlight on.
"""
import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = []
DEPENDENCIES = ["i2c"]

crowpanel_advance_io_ns = cg.esphome_ns.namespace("crowpanel_advance_io")
CrowPanelAdvanceIO = crowpanel_advance_io_ns.class_("CrowPanelAdvanceIO", cg.Component, i2c.I2CDevice)

CONFIG_SCHEMA = (
    cv.Schema({cv.GenerateID(): cv.declare_id(CrowPanelAdvanceIO)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x30))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
