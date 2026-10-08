# Hardware Checklist

Use this when the strip shows all-white, flickering, or random colors and the
minimal tests (`test_single_pixel`, `test_chain_walk`) also fail. Check in
this order — stop debugging software until every item is verified.

## Checklist (in order)

1. **DIN/DOUT direction**
   - The ESP32 data wire must connect to **DIN** of the first module.
   - Follow the arrows printed on the PCB/strip; they point from DIN to DOUT.
   - Feeding DOUT produces a dead or all-white strip.

2. **Common ground**
   - ESP32 GND and the LED power-supply GND **must** be connected.
   - Without a shared ground the data signal has no reference and floats.

3. **Correct GPIO**
   - Current software uses **GPIO 27**. Verify the wire is actually on GPIO 27.

4. **3.3 V data level into 5 V LEDs**
   - The ESP32 outputs 3.3 V logic; WS2812B at 5 V expects ~0.7 x VDD (3.5 V)
     for a logic HIGH — marginally out of spec.
   - If the signal is marginal, add a level shifter such as a **74AHCT125** or
     **74HCT245** (not a bidirectional I2C shifter — too slow).
   - A ~330–470 ohm series resistor on the data line also helps against
     ringing on longer wires.

5. **Correct LED protocol**
   - The code assumes **WS2812B**.
   - `GRB` only controls channel order; a wrong color order gives wrong
     colors, never all-white. Do not chase this for all-white output.

6. **Power**
   - 49 LEDs showing full white can draw ~2.9 A (60 mA per LED). Use an
     adequate 5 V PSU.
   - Add a large capacitor (e.g. 1000 uF, 6.3 V+) across 5 V/GND near the
     first module.
   - Power-up white before FastLED initialization can cause brownouts and
     data instability — the firmware now clears the strip before Wi-Fi, but
     the PSU must still handle the inrush.
   - Do not power 49 LEDs from the ESP32's 5 V pin.

## Minimal tests

```sh
# Only physical LED #0 must light dim red:
.venv/bin/pio test -e wemos_d1_mini32 -f test_single_pixel

# One red LED must walk 0 -> 48 in physical order and wrap:
.venv/bin/pio test -e wemos_d1_mini32 -f test_chain_walk
```

## Interpretation matrix

| Observation in minimal tests | Meaning | Next step |
|------------------------------|---------|-----------|
| Single pixel red / ordered walk | Hardware works | Resume software debugging of the app |
| All LEDs stay white | Hardware/data path broken | Work the checklist above, in order |
| Random colors / flickering | Signal integrity | Ground, level shifter, power |
| Walk starts mid-strip / reversed | Wrong end or chain order | Fix DIN/DOUT wiring |

## Expected app diagnostics

On boot, the serial monitor (115200 baud) should show:

```
NUM_LEDS: 49
Display required LEDs: 49
Display modules: 2
Display valid: yes
```

The strip must be **black** during the Wi-Fi connect phase, then show the red
counter starting at `00`.
