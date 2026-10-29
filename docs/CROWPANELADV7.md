# Elecrow CrowPanel Advance 7 inch, ESP32-S3, experimental

This is the **Elecrow CrowPanel Advance 7.0-inch ESP32-S3 HMI display** (SKU DIS08070H, V1.x): a 7-inch 800 x 480 IPS
RGB panel, GT911 capacitive touch, an ESP32-S3-WROOM-1-N16R8 with 16 MB flash and 8 MB octal PSRAM, and an I2C board
controller for the backlight and touch reset.

## Which board

- **CrowPanel Advance 7.0-inch (V1.0 to V1.5), not the Basic 7.0-inch (V3.0).** Elecrow sells two different 7-inch
  ESP32-S3 boards under similar names:
  - The **Advance 7.0-inch** has 16 MB flash, Touch I2C on GPIO15/GPIO16, RGB control pins DE=42, VSYNC=41, HSYNC=40,
    PCLK=39 (`model: CROWPANEL-ADVANCE-7`), a rear `WM-SWITCH` DIP block, and an I2C board controller (`STC8H1K28` at
    `0x30` on V1.2–V1.5, or `PCA9557`/`TCA9534` at `0x18` on V1.0–V1.1) that drives the backlight and GT911 reset.
    This board file covers both V1.0–V1.1 and V1.2–V1.5 revisions automatically.
  - The **Basic 7.0-inch** (silkscreened `ESP32 Terminal 7.0 inch RGB` / `V3.0`) has 4 MB flash, Touch I2C on
    GPIO19/GPIO20, DE=41, VSYNC=40, HSYNC=39, PCLK=0, and a GPIO PWM backlight on GPIO2/GPIO38; it is **not** this board
    file.
- **Rear DIP switch (`WM-SWITCH`).** Keep both switches at `1=OFF, 2=OFF` (normal operation). GPIO15 and GPIO16 are
  shared between the touch/controller I2C bus and the wireless module bay when the switches are flipped on.

## Install

Update Tessera Screen Manager and choose **CrowPanel · 7 inch, Advanced 7 ESP32-S3** (marked Experimental) in **New
screen**. Follow [Easy setup](EASY_SETUP.md) to create a profile with its own name, Wi-Fi references and unique API/OTA
keys. Connect the USB data cable to the **UART0 USB-C port** (via the onboard CH340C USB-to-UART bridge) to flash or
read logs. Tessera adds the screen to Home Assistant once it is on your Wi-Fi; then add its tiles.

The remote package is `packages/crowpaneladv7.yaml` and the checkout entry `checkout/crowpaneladv7.yaml`. Each combines
`packages/core.yaml` with `packages/boards/crowpaneladv7.yaml` and `packages/hardware/crowpanel-advance-s3.yaml`.

## Layout and capabilities

- Landscape: 800 x 480, four columns of four cells. Portrait: 480 x 800, two columns of seven cells.
- The standard look at 133 dpi, the density of the 7-inch Waveshare and Sunton, so a tile keeps its size in
  millimetres.
- The backlight is driven over I2C (`components/crowpanel_advance_io`): on V1.2–V1.5 boards the STC8H1K28 at `0x30`
  dims smoothly across its inverted scale (`0` = full light, `245` = dark), and on V1.0–V1.1 boards the PCA9557 at
  `0x18` switches the backlight on and off. Standby, night mode, Sleep, Wake and alert flashes are enabled.
- Camera tiles, full-screen snapshots, live tile pictures, camera alerts and media artwork use the shared ESP32-S3
  PSRAM implementation.
- The GT911 reports pixel coordinates in polling mode (20 ms), with no resistive calibration.
- LVGL uses a 12 % draw buffer, as on the other 800 x 480 ESP32-S3 boards.
- The onboard speaker, microphone, SD card slot and wireless module bay are not configured yet.

## Hardware references

- [Elecrow's GitHub repository for the CrowPanel Advance 7](https://github.com/Elecrow-RD/CrowPanel-Advance-7-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480)
- [ESPHome MIPI RGB](https://esphome.io/components/display/mipi_rgb/): the RGB565 data pins, control pins (DE GPIO42,
  VSYNC GPIO41, HSYNC GPIO40, PCLK GPIO39) and 20 MHz inverted-clock panel timing match Elecrow's schematic and ESPHome
  2026.10's `CROWPANEL-ADVANCE-7` preset, stated on `model: RPI` so the board builds on ESPHome 2026.9 and 2026.10+.
- I2C (`touch_bus`): SDA GPIO15, SCL GPIO16 at 400 kHz, shared by the GT911 (`0x5D`) and the board controller (`0x30`
  on V1.2+, `0x18` on V1.0–V1.1). At `setup_priority::IO`, `crowpanel_advance_io` holds GPIO1 (GT911 INT) low while
  pulsing the controller's touch-reset command so the GT911 latches address `0x5D` before `gt911` sets up. The GT911 is
  polled without `interrupt_pin` (in ESPHome 2026.10, configuring an internal GPIO `interrupt_pin` disables polling and
  stops touch reporting on this board).

## What to report while testing

1. The PCB revision printed on the back (`V1.0`–`V1.5`), which I2C controller was found in the boot log (`0x30` or
   `0x18`), and appearance in Tessera.
2. Correct colours and a stable picture across several page changes, a restart from the screen's settings, an update
   over the air and a cold start.
3. Physical taps near each corner, slider drags and edge swipes, lying down and standing up.
4. The brightness setting across its range, standby after the timeout and wake by touch, several times in a row, and
   night mode.
5. Camera tile pictures, full-screen camera images and camera alerts, including repeated opens and closes.
6. `sensor.<screen>_heap_free` with a full layout, any reset, Wi-Fi drop or I2C error, and how long the screen stayed
   running.

Do not publish Wi-Fi passwords or API/OTA keys with logs.
