import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv

from .light import Modular7Segment, display_ns

ClockSwitch = display_ns.class_("ClockSwitch", switch.Switch)
CONFIG_SCHEMA = switch.switch_schema(ClockSwitch).extend(
    {
        cv.Required("display_id"): cv.use_id(Modular7Segment),
        cv.Required("kind"): cv.one_of("auto_brightness", "auto_mode"),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config["display_id"])
    var = cg.new_Pvariable(config["id"], parent, config["kind"] == "auto_brightness")
    await switch.register_switch(
        var, {key: value for key, value in config.items() if key != "mqtt_id"}
    )
