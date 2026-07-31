# Lights

Firmware, BLE browser controls, project notes, and a compact ESP32-S3 hardware design for addressable LED installations.

Start with [`PROJECT_STATE.md`](PROJECT_STATE.md). It is the single resume point for the current status, architecture, blockers, next steps, and canonical links.

## Current Status

- Site: GitHub Pages shell in [`index.html`](index.html), rendered from notebooks in [`ipynb/`](ipynb/) with `ipynb2web`.
- Hardware: [`Compact ESP32-S3 USB LED Controller V2`](hardware/compact-esp32-s3-led-controller-v2/DESIGN.md), design in progress and not fabrication-ready.
- Firmware: Arduino/PlatformIO code under [`src/`](src/) still targets generic `esp32dev`, defaults LED data to GPIO15, and does not enforce `maxCurrent`.
- Browser: [`bt.html`](bt.html) and [`lights.js`](lights.js) are BLE controls for already-flashed compatible firmware. Browser firmware upload is not currently supported.

## Active Hardware

- Module: ESP32-S3-MINI-1-N8 with 8 MB flash and embedded antenna
- Input: regulated 5 V pigtail landing; no bare LiPo support
- LED interface: protected 5 V, GPIO18 through a 5 V AHCT buffer, and common ground
- Programming: six-pad UART fixture providing 5V, GND, TX, RX, EN, and BOOT
- Authority: [`hardware/compact-esp32-s3-led-controller-v2/DESIGN.md`](hardware/compact-esp32-s3-led-controller-v2/DESIGN.md)

The V2 snapshot is preserved at [`hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-30/`](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-30/README.md) as recovery/reference only. No fabrication release or shared EasyEDA cloud URL exists.

## Local Verification

```bash
npm run build
npm start
```

`npm run build` validates the active route map, notebook JSON, internal notebook links, and required local files. `npm start` serves the GitHub Pages shell locally at `http://localhost:8000/`.

Archived stale notes and the unsupported browser upload experiment are in [`archive/`](archive/).
