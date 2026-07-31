# Lights Project State

Last reconciled: 2026-07-30

This is the resume point for the `lights` repository. If another agent picks up the project, start here, then read the linked hardware design record before changing firmware, PCB, fixture, or browser-control assumptions.

## Current Truth

- The public website is a GitHub Pages app rendered client-side by `ipynb2web` from [`index.html`](index.html) and notebooks in [`ipynb/`](ipynb/).
- The active hardware target is [`Compact ESP32-S3 USB LED Controller V2`](hardware/compact-esp32-s3-led-controller-v2/DESIGN.md).
- V2 is a design-in-progress EasyEDA Standard project. It is not fabrication-ready and has no shared EasyEDA cloud URL.
- The checked-in V2 snapshot under [`hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-30/`](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-30/README.md) is a recovery/reference artifact, not a release package.
- Current firmware still targets generic `esp32dev`, defaults LED data to GPIO15, and does not enforce `maxCurrent`.
- V2 hardware uses ESP32-S3-MINI-1-N8 and routes LED data from GPIO18 through a 5 V AHCT buffer.
- The browser BLE controller can talk to already-flashed compatible legacy firmware. It is not a V2 release tool and it cannot flash firmware in the browser.

## Canonical Links

- Public site shell and router: [`index.html`](index.html)
- Active site notebooks: [`ipynb/`](ipynb/)
- Standalone legacy BLE control surface: [`bt.html`](bt.html)
- BLE client logic: [`lights.js`](lights.js)
- Firmware entry point: [`src/main.cpp`](src/main.cpp)
- Firmware BLE parsing: [`src/communications.cpp`](src/communications.cpp)
- PlatformIO configuration: [`platformio.ini`](platformio.ini)
- Hardware authority: [`hardware/compact-esp32-s3-led-controller-v2/DESIGN.md`](hardware/compact-esp32-s3-led-controller-v2/DESIGN.md)
- Hardware recovery snapshot: [`hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-30/README.md`](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-30/README.md)
- Archived stale notes and upload experiment: [`archive/`](archive/)

## Current Architecture

```text
browser
  |
  | Web Bluetooth JSON writes
  v
legacy-compatible ESP32 firmware
  |
  | Adafruit_NeoPixel output, currently constructed from startup defaults
  v
addressable LED strip

future V2 hardware target
  ESP32-S3 GPIO18 -> AHCT buffer -> LED DATA
  protected 5 V -> LED 5V
  common GND -> LED GND
```

The future production path is ESP32-S3 V2 hardware plus firmware that is explicitly made compatible with that hardware. The current repo has not crossed that bridge yet.

## Firmware Compatibility Blockers

1. Replace or add the PlatformIO environment for ESP32-S3-MINI-1-N8 instead of generic `esp32dev`.
2. Make GPIO18 the fixed production LED data pin, or implement a verified safe reinitialization path.
3. Implement real current estimation and limiting. The existing `maxCurrent` field is only parsed and logged.
4. Choose a conservative product current ceiling after bench validation with the selected supply, enclosure, LED rope, copper, fuse, and pigtails.
5. Validate six-pad UART programming and recovery through EN/BOOT/GPIO0.
6. Test BLE and RF range in the final enclosure and antenna keepout geometry.

## Hardware Release Blockers

Use [`DESIGN.md`](hardware/compact-esp32-s3-led-controller-v2/DESIGN.md) as the source of truth. In short:

1. Remove board-mounted USB-C and CC resistors from the canonical V2 implementation.
2. Add the two-hole regulated `5V_IN/GND` input landing.
3. Regenerate schematic/PCB from the current architecture and reconcile all nets and parts.
4. Complete portrait placement/routing, DRC, connectivity, Gerber/drill inspection, BOM/PnP reconciliation, and fixture regeneration.
5. Bench-test LDO temperature, LED load/inrush, PPTC behavior, pigtails, enclosure thermal behavior, and RF range.

## Website Status

Active navigation is intentionally narrow:

- Project State
- Hardware
- Firmware
- Browser Control
- Flashing Status
- LED/Power Notes
- Next Steps
- Architecture
- Audio
- Compliance

Old WLED research, generic ESP32 research, dated shopping/pricing notes, old battery directions, old fixture geometry, and the unsupported browser upload experiment are archived under [`archive/`](archive/) and excluded from active navigation.

## Verification

Use the repo scripts:

```bash
npm run build
npm start
```

`npm run build` validates the route map, notebook JSON, active internal links, and required local files. `npm start` serves the GitHub Pages shell locally so the `ipynb2web` browser render can be checked at `http://localhost:8000/`.
