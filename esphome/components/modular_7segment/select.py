import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv

from .light import Modular7Segment, display_ns

ClockSelect = display_ns.class_("ClockSelect", select.Select)
CONFIG_SCHEMA = select.select_schema(ClockSelect).extend(
    {cv.Required("display_id"): cv.use_id(Modular7Segment)}
)


async def to_code(config):
    parent = await cg.get_variable(config["display_id"])
    var = cg.new_Pvariable(config["id"], parent)
    await select.register_select(
        var, {key: value for key, value in config.items() if key != "mqtt_id"},
        options=["Time", "CO2", "Temperature"],
    )
