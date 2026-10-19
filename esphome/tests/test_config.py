"""Schema and code-generation tests; uses only disposable, dummy credentials."""

import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / ".venv/bin/esphome"
SPEC = importlib.util.spec_from_file_location(
    "display_schema", ROOT / "components/modular_7segment/light.py"
)
SCHEMA = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(SCHEMA)


class ConfigTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="seven-segment-config-")
        cls.stage = Path(cls.temporary.name)
        shutil.copytree(ROOT / "packages", cls.stage / "packages")
        (cls.stage / "components").symlink_to(ROOT / "components", target_is_directory=True)
        shutil.copyfile(ROOT / "secrets.yaml.example", cls.stage / "secrets.yaml")
        for board in ("esp32", "c3"):
            name = f"neopixel-clock-{board}.yaml"
            shutil.copyfile(ROOT / name, cls.stage / name)

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def run_config(self, board="esp32", extra="", command="config"):
        source = (ROOT / f"neopixel-clock-{board}.yaml").read_text()
        target = self.stage / "test-device.yaml"
        target.write_text(source.replace("substitutions:\n", "substitutions:\n" + extra, 1))
        return subprocess.run(
            [str(CLI), command, str(target)] + (
                ["--only-generate"] if command == "compile" else []
            ),
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
        )

    def test_board_configs_generate(self):
        for board in ("esp32", "c3"):
            with self.subTest(board=board):
                result = self.run_config(board, command="compile")
                self.assertEqual(result.returncode, 0, result.stdout)
                generated = (self.stage / ".esphome/build" /
                             f"neopixel-clock-{board}/src/main.cpp").read_text()
                self.assertIn("Modular7Segment", generated)
                self.assertIn("set_temperature_offset(-2.0f)", generated)
                self.assertNotIn("MQTTSwitchComponent", generated)
                self.assertNotIn("MQTTSelectComponent", generated)
                self.assertNotIn("MQTTJSONLightComponent(display_light)", generated)
                self.assertIn("set_reboot_timeout(0)", generated)

    def test_led_count_mismatch(self):
        result = self.run_config(extra="  num_leds: '115'\n")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("exactly 116 LEDs", result.stdout)

    def test_wrong_digit_count(self):
        result = self.run_config(extra="  layout: 'DDD'\n")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("exactly four digit", result.stdout)

    def test_missing_temperature_separator(self):
        result = self.run_config(extra="  layout: 'DD:DD'\n")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("decimal after digit three", result.stdout)

    def test_invalid_layout(self):
        for layout in ("", "DD?DD", "D0DDD", "D256DDD", "D0001DDD", "D" * 33):
            with self.subTest(layout=layout):
                with self.assertRaises(SCHEMA.cv.Invalid):
                    SCHEMA.layout_size(layout, 4, 1)

    def test_layout_sizes(self):
        self.assertEqual(SCHEMA.layout_size("DD:D.D.", 4, 1), 116)
        self.assertEqual(SCHEMA.layout_size("D2D3:D4D5.", 1, 1), 101)
        self.assertEqual(SCHEMA.layout_size("DDDD" + "." * 28, 4, 1), 140)


if __name__ == "__main__":
    unittest.main()
