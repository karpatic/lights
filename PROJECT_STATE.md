# Lights Project State

Last reconciled: 2026-08-01

This is the resume point for the `lights` repository. Read this first, then inspect the linked hardware design record before changing firmware, PCB, fixture, browser-control, audio, or mesh assumptions.

## Current Truth

- Lights is one integrated active music/DJ lighting application, not a split collection of inactive side projects.
- The authoritative browser app is [`index.html`](index.html), rendered client-side from the active notebooks in [`ipynb/`](ipynb/).
- The notebook home page embeds the Web Bluetooth dashboard and calls the same [`lights.js`](lights.js) client used by [`bt.html`](bt.html).
- [`lights.js`](lights.js) writes JSON control payloads to the configured BLE characteristic for compatible flashed firmware.
- [`upload.html`](upload.html) and [`web.js`](web.js) are active browser FileReader/Web Serial tooling. They are not yet a proven firmware flashing path because the current firmware lacks the matching `CMD:*` handler.
- Audio/Meyda and DJ control remain active product direction in the notebooks and external DJ-panel notes. The repository does not currently contain a local `AudioContext`/`decodeAudioData` runtime.
- ESP32 mesh behavior remains active product direction in the ESP32 and plan notebooks. The current firmware does not yet implement the ESP-NOW/ad-hoc mesh network.
- The active hardware target is [`Compact ESP32-S3 USB LED Controller V2`](hardware/compact-esp32-s3-led-controller-v2/DESIGN.md), still design-in-progress and not fabrication-ready.
- V2 has a proven 18 × 32 mm routed recovery baseline and a newer experimental 18 × 29 mm compact iteration. Neither is fabrication-ready, and there is no shared EasyEDA cloud URL.
- The experimental compact iteration has 22 footprints, 60 tracks, 35 vias, two GND pours, 11/11 connected nets, 2.00 mm cable pads, 1.20 mm finished holes, and a fresh zero-error EasyEDA DRC result.
- The compact iteration is blocked on a placement redesign: the vertically rotated PPTC raw-input pad overlaps the same-net 5 V input landing and crowds cable soldering access. Zero DRC does not make that mechanical arrangement acceptable.
- The latest immutable-style checked-in recovery point remains the earlier 18 × 32 mm routed baseline under [`hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-31/`](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-31/README.md). Do not mistake it for the later compact iteration.
- Carlos's current top/bottom screenshots and the connector-first placement review are preserved under [`hardware/compact-esp32-s3-led-controller-v2/reviews/2026-07-31/`](hardware/compact-esp32-s3-led-controller-v2/reviews/2026-07-31/README.md).
- Current firmware still targets generic `esp32dev`, defaults LED data to GPIO15, and does not enforce `maxCurrent`.
- V2 hardware uses ESP32-S3-MINI-1-N8 and routes LED data from GPIO18 through a 5 V AHCT buffer.

## Canonical Links

- Browser app shell and router: [`index.html`](index.html)
- Active site notebooks: [`ipynb/`](ipynb/)
- Notebook home/dashboard: [`ipynb/index.ipynb`](ipynb/index.ipynb)
- Standalone BLE controller: [`bt.html`](bt.html)
- BLE client logic: [`lights.js`](lights.js)
- Browser file/Web Serial page: [`upload.html`](upload.html)
- Browser file/Web Serial logic: [`web.js`](web.js)
- Firmware entry point: [`src/main.cpp`](src/main.cpp)
- Firmware BLE parsing: [`src/communications.cpp`](src/communications.cpp)
- PlatformIO configuration: [`platformio.ini`](platformio.ini)
- Hardware authority: [`hardware/compact-esp32-s3-led-controller-v2/DESIGN.md`](hardware/compact-esp32-s3-led-controller-v2/DESIGN.md)
- Current compact-layout visual review: [`hardware/compact-esp32-s3-led-controller-v2/reviews/2026-07-31/README.md`](hardware/compact-esp32-s3-led-controller-v2/reviews/2026-07-31/README.md)
- Latest routed hardware recovery snapshot: [`hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-31/README.md`](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-31/README.md)
- Earlier pre-routing recovery snapshot: [`hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-30/README.md`](hardware/compact-esp32-s3-led-controller-v2/snapshots/2026-07-30/README.md)

## Active App Routes

- Lights: `#/`
- LEDs: `#/lights`
- Sound: `#/audio`
- Web: `#/web`
- Upload: `#/upload`
- WLED: `#/wled`
- ESP32: `#/esp32`
- Plan: `#/plan`
- Board Designs: `#/board_designs`
- Diagrams: `#/diagrams`
- Licensing: `#/licensing`

Friendly routes from the cleanup pass, such as `#/hardware`, `#/firmware`, `#/browser`, `#/flashing`, and `#/architecture`, are aliases into the restored route set.

## Implemented And Planned Flow

```text
browser app / notebook dashboard
  |
  | calls lights.js / bt.html controls
  v
Web Bluetooth JSON writes
  |
  v
current compatible firmware
  |
  | Adafruit_NeoPixel output from current startup/default assumptions
  v
addressable LED strip

browser file/Web Serial tooling
  |
  | FileReader + navigator.serial + CMD:* messages
  v
firmware handler still needed

audio/Meyda/DJ planning
  |
  | future analysis/control payloads should feed the BLE JSON path
  v
lights.js control payloads

ESP32 mesh planning
  |
  | future ESP-NOW/ad-hoc routing work
  v
multiple light nodes
```

## Firmware Compatibility Blockers

1. Replace or add the PlatformIO environment for ESP32-S3-MINI-1-N8 instead of generic `esp32dev`.
2. Make GPIO18 the production LED data pin for V2, or implement and verify a safe runtime reinitialization path.
3. Implement real current estimation and limiting. The existing `maxCurrent` field is parsed/logged metadata today.
4. Reconcile the browser defaults with validated product defaults. The 300-pixel / 8 A browser defaults are not a validated V2 product rating.
5. Add or deliberately remove the browser `CMD:*` upload protocol by matching firmware behavior to [`web.js`](web.js).
6. Validate six-pad UART programming and recovery through EN/BOOT/GPIO0.
7. Implement and test any ESP-NOW/ad-hoc mesh behavior before documenting it as firmware runtime.
8. Test BLE/RF range in the final enclosure and antenna keepout geometry.

## Hardware Release Blockers

Use [`DESIGN.md`](hardware/compact-esp32-s3-led-controller-v2/DESIGN.md) as the hardware source of truth. In short:

Completed in the 2026-07-31 routed baseline and subsequent compact experiment:

- removed board-mounted USB-C and CC resistors;
- added the two-hole regulated `5V_IN/GND` input landing;
- regenerated and reconciled the no-USB schematic and PCB;
- completed a deterministic 18 × 32 mm routed baseline with connectivity, live DRC, and save/reopen verification;
- demonstrated an 18 × 29 mm compact route with 35 vias, complete connectivity, and fresh zero-error DRC;
- reduced cable pads to 2.00 mm while retaining 1.20 mm finished holes and aligned the two cable-GND holes.

Remaining release gates:

1. Rebuild the lower-half placement so the PPTC, TVS, and component bodies do not obstruct cable holes or soldering access.
2. Study a single horizontal five-hole cable row, horizontal PPTC power flow, and a lower-height 3 × 2 pogo field before committing new copper.
3. Freeze the accepted compact source and perform canonical save/close/reopen verification on that exact iteration.
4. Complete an independent schematic/layout review.
5. Generate and visually inspect Gerbers and drill files.
6. Reconcile BOM and pick-and-place outputs against the intended parts and transformed connector footprints.
7. Regenerate and verify the programming fixture for the accepted six-pad geometry.
8. Review manufacturer rules and archive the exact order package.
9. Bench-test LDO temperature, LED load/inrush, PPTC behavior, pigtails, enclosure thermal behavior, and RF range.

## Verification

Use the repo scripts:

```bash
npm run build
npm start
```

`npm run build` validates the restored route map, active notebook JSON, active internal links, and required local runtime files. `npm start` serves the browser app locally at `http://localhost:8000/`.
