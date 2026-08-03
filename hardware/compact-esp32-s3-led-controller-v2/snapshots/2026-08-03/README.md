# EasyEDA V2 connector-first routed recovery snapshot - 2026-08-03

This directory captures the accepted connector-first, two-layer routing milestone for the active `Compact ESP32-S3 USB LED Controller V2` EasyEDA Standard project.

It is a **verified routed recovery/reference snapshot, not a fabrication release**.

## Milestone captured

- 18 x 27 mm connector-first outline;
- two copper layers;
- 22 footprints;
- 53 total tracks, including the board outline;
- 30 vias;
- two persisted filled GND copper areas;
- all 11 nets connected;
- five aligned cable holes with 2.00 mm pads and 1.20 mm finished holes;
- six bottom-only, undrilled 1.20 mm square pogo pads arranged as a 3 x 2 field with 2.00 mm column pitch and 3.00 mm row pitch;
- placement, power, and signal generators compile;
- full deterministic placement/power/signal rerun was semantically identical;
- live EasyEDA DRC verified as `DRC Errors (0)`;
- project save, real tab close, canonical PCB-child reopen, structural count/fill persistence, and post-reopen DRC verification completed.

The previously rejected 18 x 29 mm compact experiment remains historical review evidence under [`../../reviews/2026-07-31`](../../reviews/2026-07-31/README.md), but this 18 x 27 mm connector-first result supersedes it as the latest accepted recovery point.

## Contents

- `easyeda/`: saved EasyEDA Standard schematic/project JSON, routed PCB JSON, and native `info` metadata.
- `automation/`: deterministic V2 build, wiring, layer configuration, placement, power/GND routing, signal routing, and generated component-ID state.
- `automation/dependencies/`: the shared EasyEDA/CDP helper imported by the V2 scripts.
- `archive-stale/`: quarantined USB-era router retained only as historical recovery context.
- `verification/verification.json`: machine-readable counts, geometry, source hashes, routing idempotence, live DRC, save/close/reopen, and persistence evidence from the live verification run.
- `MANIFEST.sha256`: integrity hashes for every backed-up artifact except the manifest itself.

No screenshot is included for this snapshot. Do not reuse the 2026-07-31 rejected-layout images as current proof.

## Verified state

| Check | Result |
|---|---:|
| Footprints | 22 |
| Total tracks | 53 |
| Vias | 30 |
| Copper areas | 2 |
| Nets | 11 / 11 |
| Outline | 18.0 x 27.0 mm |
| Cable pads / holes | 2.00 mm / 1.20 mm |
| Pogo field | 3 x 2, bottom-only, undrilled, 1.20 mm pads |
| Pogo pitch | 2.00 mm columns, 3.00 mm rows |
| Live DRC | 0 errors |
| Full rerun | Semantically identical |
| Save / close / reopen | Passed |
| Post-reopen DRC | 0 errors |

After reopen, EasyEDA serialized the persisted copper fills in `fillData` rather than `polygonArr`. Both GND copper areas retained non-empty `fillData`, and fresh post-reopen DRC remained zero.

## Authority and limitations

The live mutable working sources at snapshot time were:

```text
/home/carlos/.easyeda/projects/Compact ESP32-S3 USB LED Controller V2/
/home/carlos/.local/share/easyeda-agent-harness/
```

The repository copy is the durable recovery point. Restore deliberately and compare hashes before overwriting a newer EasyEDA workspace.

This milestone closes deterministic placement/routing, connectivity, DRC, idempotence, save/close/reopen, and source persistence for the connector-first PCB. It does **not** close the fabrication release gates. Before ordering boards, still complete:

1. independent schematic and PCB-layout design review;
2. fabricator capability and stack-up confirmation;
3. Gerber and drill export generation plus visual inspection;
4. BOM and pick-and-place reconciliation;
5. programming-fixture regeneration for the 2.00 mm by 3.00 mm pogo field;
6. manufacturer-rule review and order-package archiving;
7. assembled-board bring-up, load, thermal, PPTC, pigtail, enclosure, and RF validation;
8. firmware ESP32-S3 target, GPIO18, and current-limit alignment.

See [`../../DESIGN.md`](../../DESIGN.md) for architecture, firmware contract, safety assumptions, and release gates.
