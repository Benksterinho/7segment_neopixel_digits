# Hardware Checklist

Use this checklist when LEDs show all-white, flicker, show random colors, or
fail the minimal LED tests. First identify the actual display hardware and its
data-chain order. The LED count, module order, and expected walk all depend on
that physical layout; a `D` (one digit) display is not the same hardware as a
`DD:DD` (four digits and a colon) display.

The checklist has two parts: confirm that the software configuration describes
the physical display, then check the electrical/data path. Stop debugging the
application until the basic LED test behaves as expected.

## 1. Match the software configuration to the display

1. **Write down the physical layout in DIN-to-DOUT order.** For example,
   `D` means one digit; `DD:DD` means two digits, a colon, then two digits.
   Include decimal points or other modules if present. The layout describes
   physical modules, not just the text you want to show.

2. **Match the LED counts.** The configured layout and pixels-per-segment /
   pixels-per-dot settings determine the required strip length. With four
   pixels per segment and one per dot, `D` needs 28 LEDs, while `DD:DD` needs
   114. Use the values for your own settings and hardware; do not assume these
   example counts apply to every display.

3. **Match module order and wiring.** Connect the first module's DIN to the
   data source, then chain each module's DOUT to the next module's DIN in the
   same order as the configured layout. A correct total LED count alone does
   not guarantee that digits, colons, and decimal points are in the right
   positions.

4. **Check the firmware's configured values.** Confirm the layout, pixel
   counts, data GPIO, and NeoPixel buffer length agree with the installed
   hardware. A mismatch can cause blank or incorrectly mapped display output
   even when the strip's electrical connections are sound.

## 2. Check the electrical and data path

1. **DIN/DOUT direction**
   - The ESP32 data wire must connect to **DIN** of the first module.
   - Follow the arrows printed on the PCB/strip; they point from DIN to DOUT.
   - Feeding DOUT instead of DIN prevents data from reaching the chain.

2. **Common ground**
   - ESP32 GND and LED power-supply GND **must** be connected.
   - Without a shared ground, the data signal has no reference.

3. **Correct GPIO**
   - Check the firmware's configured data pin, then verify the wire is on that
     GPIO. The main project currently defaults to **GPIO 27**; other board
     environments or sketches may use a different pin.

4. **3.3 V data level into 5 V LEDs**
   - The ESP32 outputs 3.3 V logic; WS2812B at 5 V expects about
     `0.7 x VDD` (3.5 V) for a logic HIGH, so direct drive can be marginal.
   - If the signal is unreliable, use a level shifter such as a **74AHCT125**
     or **74HCT245**. Do not use a bidirectional I2C shifter.
   - A roughly 330–470 ohm series resistor on the data line can help reduce
     ringing on longer wires.

5. **Correct LED protocol**
   - The firmware assumes **WS2812B** with `NEO_GRB + NEO_KHZ800`.
   - A different channel order usually causes wrong colors, not an all-white
     strip. Check protocol and color order against the actual LEDs.

6. **Power sized for the installed LED count**
   - Estimate worst-case current from the actual strip length. At up to about
     60 mA per LED at full white, a 28-LED `D` example could draw about 1.7 A;
     a 114-LED `DD:DD` example could draw about 6.8 A. Actual draw depends on
     brightness and LED type.
   - Use an adequate regulated 5 V supply. Do not power a long chain from the
     ESP32's 3.3 V pin or rely on its 5 V pin for the whole display.
   - Add a large capacitor (for example, 1000 uF, 6.3 V or higher) across
     5 V/GND near the first module.

## Minimal tests

These standalone tests bypass the display library and application. They
currently use GPIO 27 and a **49-LED buffer**, so adjust their pin and LED-count
constants to match your board and complete physical chain before running them.
The single-pixel test checks the first LED; the chain-walk test checks the
configured range in data order. Neither test verifies that the configured
`D`, `DD:DD`, or other module layout matches the display's segment mapping.

```sh
# Only physical LED #0 must light dim red:
.venv/bin/pio test -e wemos_d1_mini32 -f test_single_pixel

# One red LED must walk through the configured buffer and wrap:
.venv/bin/pio test -e wemos_d1_mini32 -f test_chain_walk
```

## Interpretation

| Observation in minimal tests | Meaning | Next step |
|------------------------------|---------|-----------|
| First LED red / ordered walk | Basic data path works for the tested range | Verify configured layout, LED count, and module mapping; then resume application debugging |
| All LEDs stay white | Hardware/data path may be broken | Check the layout and wiring first, then work through the electrical checklist |
| Random colors / flickering | Likely signal integrity, ground, or power issue | Check common ground, level shifting, wiring length, and supply capacity |
| Walk starts mid-chain / runs in an unexpected order | Data direction or physical chain order does not match expectations | Confirm DIN/DOUT and compare the physical chain order with the configured layout |
| Some modules do not light during the walk | Test buffer may be too short, or chain/power may be faulty | Set the test LED count to the full physical chain length; then check connections and power |

## Serial diagnostics

Use the firmware's reported `NUM_LEDS` and display-validity diagnostics, when
available, to check the compiled configuration. Their values depend on the
selected layout and pixel counts. Compare the reported LED count with the
actual chain length; do not expect one fixed count or module total for every
hardware layout.
