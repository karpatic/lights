# Compact ESP32-S3 LED Controller V2

This is the hardware track for the firmware and browser-control work in [`karpatic/lights`](https://github.com/karpatic/lights). It is a compact, production-oriented ESP32-S3 controller for a 5 V addressable LED rope.

- [Current design record](DESIGN.md)
- [Backed-up EasyEDA and automation snapshot](snapshots/2026-07-30/README.md)
- EasyEDA project name: `Compact ESP32-S3 USB LED Controller V2`
- Working EasyEDA source: `/home/carlos/.easyeda/projects/Compact ESP32-S3 USB LED Controller V2/`
- Working automation: `/home/carlos/.local/share/easyeda-agent-harness/`

The checked-in snapshot is a recovery/reference artifact, not a fabrication release. The active design still requires schematic regeneration, portrait routing, connectivity verification, DRC, fixture regeneration, manufacturing exports, and bench validation.

## Relationship to this repository

The `lights` repository owns the firmware, effects, BLE control surface, and the hardware/firmware compatibility contract. EasyEDA remains the PCB editor and mutable electrical-design workspace. This subfolder provides the durable, version-controlled association between them.

The current firmware is not yet directly compatible with V2: it targets `esp32dev`, defaults LED data to GPIO15, and records—but does not enforce—an 8 A `maxCurrent`. V2 uses ESP32-S3-MINI-1-N8 and routes LED data through the AHCT buffer from GPIO18. These are release blockers documented in [DESIGN.md](DESIGN.md).
