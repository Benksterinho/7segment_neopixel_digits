import math
import re

import esphome.codegen as cg
from esphome.components import light, select, switch, time
import esphome.config_validation as cv
from esphome.const import CONF_OUTPUT_ID
import esphome.final_validate as fv

DEPENDENCIES = ["esp32", "mqtt", "time"]

display_ns = cg.esphome_ns.namespace("modular_7segment")
Modular7Segment = display_ns.class_(
    "Modular7Segment", light.LightOutput, cg.Component
)


def layout_size(layout: str, pixels_per_segment: int, pixels_per_dot: int) -> int:
    tokens = re.findall(r"([D:.])([0-9]*)", layout)
    if not tokens or "".join(a + b for a, b in tokens) != layout:
        raise cv.Invalid("Layout must contain only D, :, . and optional LED counts")
    if len(tokens) > 32:
        raise cv.Invalid("Layout supports at most 32 modules")
    if sum(kind == "D" for kind, _ in tokens) != 4:
        raise cv.Invalid("The clock application requires exactly four digit modules")
    count = 0
    for kind, inline in tokens:
        density = int(inline) if inline else (
            pixels_per_segment if kind == "D" else pixels_per_dot
        )
        if len(inline) > 3 or not 1 <= density <= 255:
            raise cv.Invalid("Module LED counts must be between 1 and 255")
        count += density * (7 if kind == "D" else 2 if kind == ":" else 1)
    if count > 65535:
        raise cv.Invalid("Layout exceeds the 65535 LED limit")
    return count


def validate_config(config):
    layout_size(
        config["layout"], config["pixels_per_segment"], config["pixels_per_dot"]
    )
    punctuation = {2: set(), 3: set()}
    digits = 0
    for kind, _ in re.findall(r"([D:.])([0-9]*)", config["layout"]):
        if kind == "D":
            digits += 1
        elif digits in punctuation:
            punctuation[digits].add(kind)
    if ":" not in punctuation[2] or "." not in punctuation[3]:
        raise cv.Invalid(
            "Clock modes require a colon after digit two and a decimal after digit three"
        )
    if config.get("effects"):
        raise cv.Invalid("Effects would compete with the clock renderer")
    if str(config["restore_mode"]) != "ALWAYS_OFF":
        raise cv.Invalid("Use ALWAYS_OFF: the display restores its own combined settings")
    if config["gamma_correct"] != 1.0 or config["default_transition_length"].total_milliseconds:
        raise cv.Invalid("The display requires gamma_correct: 1 and zero default transitions")
    if not math.isfinite(config["temperature_offset"]):
        raise cv.Invalid("Temperature offset must be finite")
    return config


CONFIG_SCHEMA = cv.All(
    light.RGB_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(Modular7Segment),
            cv.Required("strip_id"): cv.use_id(light.AddressableLightState),
            cv.Required("time_id"): cv.use_id(time.RealTimeClock),
            cv.Required("auto_brightness_id"): cv.use_id(switch.Switch),
            cv.Required("auto_mode_id"): cv.use_id(switch.Switch),
            cv.Required("mode_select_id"): cv.use_id(select.Select),
            cv.Optional("layout", default="DD:D.D."): cv.string_strict,
            cv.Optional("pixels_per_segment", default=4): cv.int_range(min=1, max=255),
            cv.Optional("pixels_per_dot", default=1): cv.int_range(min=1, max=255),
            cv.Optional("dot_brightness_percent", default=100): cv.int_range(min=1, max=255),
            cv.Optional("temperature_offset", default=-2.0): cv.float_,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    validate_config,
)


def final_validate(config):
    full = fv.full_config.get()
    path = full.get_path_for_id(config["strip_id"])[:-1]
    strip = full.get_config_for_path(path)
    needed = layout_size(
        config["layout"], config["pixels_per_segment"], config["pixels_per_dot"]
    )
    if strip.get("platform") != "esp32_rmt_led_strip":
        raise cv.Invalid("strip_id must reference an ESP32 RMT LED strip")
    if strip["num_leds"] != needed:
        raise cv.Invalid(f"Physical strip must have exactly {needed} LEDs for this layout")
    if not strip.get("internal", False) or strip.get("effects"):
        raise cv.Invalid("Physical strip must be internal and have no effects")
    if strip["gamma_correct"] != 1.0 or strip["default_transition_length"].total_milliseconds:
        raise cv.Invalid("Physical strip requires gamma_correct: 1 and zero transitions")
    if str(strip["restore_mode"]) != "ALWAYS_OFF":
        raise cv.Invalid("Physical strip must start with restore_mode: ALWAYS_OFF")
    key = ("modular_7segment_strip", str(config["strip_id"]))
    if key in full.data:
        raise cv.Invalid("A physical strip may have only one display renderer")
    full.data[key] = True


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    native_config = {key: value for key, value in config.items() if key != "mqtt_id"}
    await light.register_light(var, native_config)
    for field in (
        "strip_id", "time_id", "auto_brightness_id", "auto_mode_id", "mode_select_id"
    ):
        target = await cg.get_variable(config[field])
        cg.add(getattr(var, f"set_{field}")(target))
    cg.add(var.set_layout(
        config["layout"], config["pixels_per_segment"], config["pixels_per_dot"],
        config["dot_brightness_percent"],
    ))
    cg.add(var.set_temperature_offset(config["temperature_offset"]))
