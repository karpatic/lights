# Lights

Lights is one integrated browser, firmware, and hardware project for music/DJ-aware addressable LED installations.

The authoritative browser app is [`index.html`](index.html), rendered from the active notebooks in [`ipynb/`](ipynb/). The app includes the notebook dashboard, Web Bluetooth control through [`lights.js`](lights.js) and [`bt.html`](bt.html), active browser file/Web Serial tooling in [`upload.html`](upload.html) and [`web.js`](web.js), audio/Meyda/DJ planning notes, and ESP32 mesh planning notes.

Start with [`PROJECT_STATE.md`](PROJECT_STATE.md) for current status and resume order.

## Project Map

- Browser app shell and notebook router: [`index.html`](index.html), [`index.css`](index.css)
- Active notebooks: [`ipynb/`](ipynb/)
- BLE controller: [`bt.html`](bt.html), [`bt.css`](bt.css), [`lights.js`](lights.js)
- Browser file/Web Serial tooling: [`upload.html`](upload.html), [`web.js`](web.js)
- Firmware and lighting modes: [`src/`](src/), [`platformio.ini`](platformio.ini)
- Earlier firmware/reference sketches: [`lightstrip/`](lightstrip/)
- Compact ESP32-S3 controller hardware: [`hardware/compact-esp32-s3-led-controller-v2/`](hardware/compact-esp32-s3-led-controller-v2/)
- Media/reference assets used by notebooks: [`rsc/`](rsc/)

## Current Gaps

- Web Bluetooth control is implemented in [`lights.js`](lights.js) and writes JSON payloads to the configured BLE characteristic. Current firmware still targets generic `esp32dev`, defaults LED data to GPIO15, and does not enforce `maxCurrent`.
- V2 hardware targets ESP32-S3-MINI-1-N8 and GPIO18 through a 5 V AHCT buffer. Firmware needs an ESP32-S3 target, GPIO18 alignment, current limiting, and bench validation before V2 is release-ready.
- [`upload.html`](upload.html) and [`web.js`](web.js) restore the active browser FileReader/Web Serial tooling. The present firmware does not implement the `CMD:*` upload protocol yet, so PlatformIO remains the confirmed firmware build/upload path.
- Audio/Meyda and DJ control are active project concepts in the notebooks and external DJ-panel notes. Repository search does not show an implemented local runtime using `AudioContext`, `decodeAudioData`, `showDirectoryPicker`, or `showOpenFilePicker`.
- ESP32 mesh behavior is documented in the ESP32 and plan notebooks as ESP-NOW/ad-hoc mesh work. The checked-in firmware does not yet implement that mesh network.

## Active Hardware

- Revision: `Compact ESP32-S3 USB LED Controller V2`
- Status: design in progress; not fabrication-ready
- Module: ESP32-S3-MINI-1-N8 with 8 MB flash and embedded antenna
- Latest checked-in recovery snapshot: verified 19 × 23 mm 2026-08-04 recovery/reference snapshot with current EasyEDA source, selected automation, verification evidence, and current board image
- Snapshot metrics: two copper layers, 22 footprints, 45 copper tracks plus one outline track, 28 vias, two filled copper areas, 11/11 nets, and EasyEDA DRC 0
- Input: regulated 5 V pigtail landing; no bare LiPo support
- LED output: protected 5 V, level-shifted DATA, and common GND
- LED data path: GPIO18 through a 5 V AHCT buffer and 33 ohm series resistor
- Programming: six-pad UART fixture carrying 5V, GND, TX, RX, EN, and BOOT/GPIO0
- Assembly intent: JLC assembles ordinary bottom-side SMT; Carlos installs/reflows the top-side ESP32 module and handles pigtails separately
- Hardware authority: [`hardware/compact-esp32-s3-led-controller-v2/DESIGN.md`](hardware/compact-esp32-s3-led-controller-v2/DESIGN.md)

The latest checked-in hardware recovery point is the verified 19 × 23 mm snapshot under [`hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-08-04/`](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-08-04/README.md). It captures the current EasyEDA source after V2 was restored/reopened from disk following the rejected V3 experiment, plus deterministic automation, verification JSON, a verified manifest, and Carlos's current board image. It is still not fabrication-ready and not final visual-layout acceptance.

EasyEDA remains the mutable PCB design authority. The repository snapshot is a recovery/reference archive, not an order package or substitute for release review.

The earlier 18 × 27 mm connector-first recovery snapshot remains under [`hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-08-03/`](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-08-03/README.md), and the earlier no-USB 18 × 32 mm routed baseline remains under [`hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-31/`](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-31/README.md). Carlos's 18 × 29 mm top/bottom screenshots and the reason that compact placement was rejected are preserved as superseded historical review evidence under [`hardware/compact-esp32-s3-led-controller-v2/reviews/2026-07-31/`](hardware/compact-esp32-s3-led-controller-v2/reviews/2026-07-31/README.md). The later V3 document, placement script, and seed were deleted after rejection and should not be treated as retained design work.

The earlier [`2026-07-30` snapshot](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-30/README.md) is intentionally retained as historical transition evidence. Its USB-era source and old fixture geometry are not the current design and must not be manufactured.

## Local Verification

```bash
npm run build
npm start
```

`npm run build` validates the restored active route map, notebook JSON, internal links, and required local runtime files. `npm start` serves the app at `http://localhost:8000/`.
