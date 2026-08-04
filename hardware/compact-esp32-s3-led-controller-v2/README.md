# Compact ESP32-S3 LED Controller V2

This is the hardware track for the firmware and browser-control work in [`karpatic/lights`](https://github.com/karpatic/lights). It is a compact, production-oriented ESP32-S3 controller for a 5 V addressable LED rope.

- [Current design record](DESIGN.md)
- [Latest checked-in EasyEDA and automation snapshot](snapshots/2026-08-04/README.md)
- EasyEDA project name: `Compact ESP32-S3 USB LED Controller V2`
- Working EasyEDA source: `/home/carlos/.easyeda/projects/Compact ESP32-S3 USB LED Controller V2/`
- Working automation: `/home/carlos/.local/share/easyeda-agent-harness/`

The latest checked-in EasyEDA V2 recovery/reference snapshot is the verified 2026-08-04 19 × 23 mm two-layer checkpoint with 22 footprints, 45 copper tracks plus one outline track, 28 vias, two filled copper areas, 11/11 connected nets, EasyEDA DRC 0, selected deterministic automation, verification evidence, and Carlos's current board image.

Manufacturing intent for the current checkpoint is one-side JLC assembly of the ordinary bottom-side SMT only. Carlos installs/reflows the ESP32-S3 module on the otherwise SMT-empty top side and handles the top-side through-hole pigtails separately. This is still a two-copper-layer board.

EasyEDA remains the mutable working authority. The checked-in snapshot is an immutable recovery/reference artifact, not a fabrication release or final visual-layout acceptance. The active design still requires independent review, fabrication-rule confirmation, Gerber/drill inspection, BOM/CPL reconciliation, fixture regeneration, bench/thermal/RF validation, and firmware ESP32-S3/GPIO18/current-limit alignment.

The later V3 experiment was visually rejected and scrapped; its EasyEDA document, placement script, and seed were deleted. Preserve the V2 snapshots and reviews, but do not present V3 as active or retained design work.

## Relationship to this repository

The `lights` repository owns the firmware, effects, BLE control surface, and the hardware/firmware compatibility contract. EasyEDA remains the PCB editor and mutable electrical-design workspace. This subfolder provides the durable, version-controlled association between them.

The current firmware is not yet directly compatible with V2: it targets `esp32dev`, defaults LED data to GPIO15, and records—but does not enforce—an 8 A `maxCurrent`. V2 uses ESP32-S3-MINI-1-N8 and routes LED data through the AHCT buffer from GPIO18. These are release blockers documented in [DESIGN.md](DESIGN.md).
