"""Exercise the server templates through real Git clones, without private secrets."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import textwrap
import unittest

ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / ".venv/bin/esphome"
SOURCE_URL = "https://github.com/Benksterinho/7segment_neopixel_digits.git"
REVISION_PLACEHOLDER = "REPLACE_WITH_PUBLISHED_COMMIT_SHA"


class ServerConfigTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="seven-segment-server-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.stage = Path(cls.temporary.name)
        cls.repository = cls.stage / "source"
        for directory in ("packages", "components"):
            shutil.copytree(ROOT / directory, cls.repository / "esphome" / directory)
        for args in (
            ["init", "--quiet"],
            ["add", "esphome"],
            ["-c", "user.name=ESPHome Test", "-c", "user.email=test@example.invalid",
             "-c", "commit.gpgsign=false", "commit", "--quiet", "-m", "Test fixture"],
        ):
            subprocess.run(["git", "-C", str(cls.repository), *args], check=True,
                           capture_output=True, text=True)
        cls.revision = subprocess.check_output(
            ["git", "-C", str(cls.repository), "rev-parse", "HEAD"], text=True
        ).strip()

    def stage_config(self, board, overrides=""):
        config_dir = self.stage / f"config-{board}-{self._testMethodName}"
        config_dir.mkdir()
        source = (ROOT / "server" / f"neopixel-clock-{board}.yaml.example").read_text()
        self.assertEqual(source.count("ref: ${project_revision}"), 2)
        source = source.replace(SOURCE_URL, self.repository.as_uri())
        source = source.replace(REVISION_PLACEHOLDER, self.revision)
        source = source.replace("substitutions:\n", "substitutions:\n" + overrides, 1)
        if os.environ.get("ESPHOME_SERVER_FULL_COMPILE") == "1":
            build = ROOT / ".esphome/build" / f"server-clock-{board}"
            source += f'\nesphome:\n  build_path: "{build}"\n'
        target = config_dir / "clock.yaml"
        target.write_text(source)
        shutil.copyfile(ROOT / "secrets.yaml.example", config_dir / "secrets.yaml")
        self.assertEqual({p.name for p in config_dir.iterdir()}, {"clock.yaml", "secrets.yaml"})
        return target

    def run_esphome(self, target, *args):
        return subprocess.run(
            [str(CLI), *args, str(target)], text=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
        )

    def test_remote_boards_build(self):
        full_compile = os.environ.get("ESPHOME_SERVER_FULL_COMPILE") == "1"
        for board in ("esp32", "c3"):
            with self.subTest(board=board):
                target = self.stage_config(board)
                args = ["compile"] if full_compile else ["compile", "--only-generate"]
                result = self.run_esphome(target, *args)
                self.assertEqual(result.returncode, 0, result.stdout)
                build = (ROOT / ".esphome/build" / f"server-clock-{board}" if full_compile
                         else target.parent / ".esphome/build" / f"neopixel-clock-{board}")
                generated = (build / "src/main.cpp").read_text()
                for expected in ("Modular7Segment", "set_temperature_offset(-2.0f)",
                                 "receive_co2", "receive_temperature", "receive_lux",
                                 'set_layout("DD:D.D.", 4, 1, 100)',
                                 "set_num_leds(116)", "set_default_transition_length(0)",
                                 "LIGHT_ALWAYS_OFF", "SWITCH_RESTORE_DISABLED"):
                    self.assertIn(expected, generated)
                self.assertEqual(generated.count("set_reboot_timeout(0)"), 3)
                self.assertIn(f"set_pin({4 if board == 'c3' else 27})", generated)
                defines = (build / "src/esphome/core/defines.h").read_text()
                variant = "ESP32-C3" if board == "c3" else "ESP32"
                self.assertIn(f'#define ESPHOME_VARIANT "{variant}"', defines)
                self.assertEqual("USB_SERIAL_JTAG" in generated, board == "c3")
                for unwanted in ("MQTTSwitchComponent", "MQTTSelectComponent",
                                 "MQTTJSONLightComponent(display_light)"):
                    self.assertNotIn(unwanted, generated)
                self.assertTrue((target.parent / ".esphome/packages").is_dir())
                self.assertTrue((target.parent / ".esphome/external_components").is_dir())
                if full_compile:
                    for name in ("firmware.factory.bin", "firmware.ota.bin"):
                        binary = build / "build" / name
                        self.assertTrue(binary.is_file(), str(binary))
                        self.assertGreater(binary.stat().st_size, 0)

    def test_remote_overrides(self):
        target = self.stage_config("esp32", "  temperature_offset: '1.5'\n")
        result = self.run_esphome(target, "compile", "--only-generate")
        self.assertEqual(result.returncode, 0, result.stdout)
        # Full-compile mode shares the persistent build path with the firmware test.
        build = (ROOT / ".esphome/build/server-clock-esp32"
                 if os.environ.get("ESPHOME_SERVER_FULL_COMPILE") == "1"
                 else target.parent / ".esphome/build/neopixel-clock-esp32")
        self.assertIn("set_temperature_offset(1.5f)", (build / "src/main.cpp").read_text())

    def test_remote_invalid_led_count(self):
        target = self.stage_config("c3", "  num_leds: '115'\n")
        result = self.run_esphome(target, "config")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("exactly 116 LEDs", result.stdout)

    def test_remote_missing_secret(self):
        target = self.stage_config("esp32")
        (target.parent / "secrets.yaml").unlink()
        result = self.run_esphome(target, "config")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("secrets.yaml", result.stdout)

    def test_guide_substitutions(self):
        guide = (ROOT / "SERVER_SETUP.md").read_text()
        substitutions = textwrap.dedent(
            "   substitutions:\n" + guide.split(
                "```yaml\n   substitutions:\n", 1
            )[1].split("   ```", 1)[0]
        )
        target = self.stage_config("esp32")
        source = target.read_text()
        source = substitutions + "\npackages:" + source.split(
            "\npackages:", 1
        )[1]
        target.write_text(source.replace(REVISION_PLACEHOLDER, self.revision))
        result = self.run_esphome(target, "config")
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("lab-clock", result.stdout)

    def test_shared_packages_have_no_local_dependencies(self):
        for name in ("clock-common.yaml", "board-esp32.yaml", "board-c3.yaml"):
            source = (ROOT / "packages" / name).read_text()
            self.assertNotIn("!secret", source)
            self.assertNotIn("type: local", source)
            self.assertNotIn("!include", source)


if __name__ == "__main__":
    unittest.main()
