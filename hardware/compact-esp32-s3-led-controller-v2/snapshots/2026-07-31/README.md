# EasyEDA V2 routed recovery snapshot — 2026-07-31

This directory captures the no-USB, two-layer portrait routing milestone for the active `Compact ESP32-S3 USB LED Controller V2` EasyEDA Standard project.

It is a **verified recovery/reference snapshot, not a fabrication release**.

## Milestone captured

- board-mounted USB-C and CC resistors removed;
- regulated two-hole `5V_IN/GND` landing present at 3.00 mm pitch with 2.20 mm pads and 1.20 mm finished holes;
- `5V_IN → TVS → BHFUSE C883162 PPTC → protected 5 V` topology retained;
- 18 × 32 mm portrait outline retained;
- ESP32-S3-MINI-1-N8 antenna-end copper keepout retained;
- three-hole LED output and six underside programming pogo pads retained;
- deterministic top/bottom power, GND, BOOT, UART, ESP_EN, and LED signal routing completed;
- live EasyEDA DRC verified as `DRC Errors (0)`;
- project save, hard close/reopen, structural count comparison, GND refill, and post-reopen DRC verification completed.

## Contents

- `easyeda/`: saved EasyEDA Standard schematic/project JSON, routed PCB JSON, and native `info` metadata.
- `automation/`: deterministic V2 build, wiring, layer configuration, placement, power/GND routing, signal routing, and generated component-ID state.
- `automation/dependencies/`: the shared EasyEDA/CDP helper imported by the V2 scripts.
- `archive-stale/`: quarantined USB-era router retained only as historical recovery context.
- `verification/verification.json`: machine-readable counts, geometry, hashes, routing idempotence, DRC, and persistence evidence.
- `verification/full-board.png`: full-board EasyEDA screenshot after final routing.
- `MANIFEST.sha256`: integrity hashes for every backed-up artifact except the manifest itself.

## Verified state

| Check | Result |
|---|---:|
| Footprints | 22 |
| Total tracks | 61 |
| Power/GND tracks | 38 |
| Signal tracks | 22 |
| Board-outline tracks | 1 |
| Vias | 38 |
| Copper areas | 2 |
| Outline | 18.0 × 32.0 mm |
| Live DRC | 0 errors |
| Reopened live counts vs persisted counts | Match |
| Editor dirty after final verification | No |

EasyEDA stores filled polygon arrays as derived state. The deterministic signal router refills both the bottom and lower top GND areas after reopening; the verified fill produced six bottom islands and five top islands while retaining zero incomplete or clearance errors.

## Authority and limitations

The live mutable working sources remain:

```text
/home/carlos/.easyeda/projects/Compact ESP32-S3 USB LED Controller V2/
/home/carlos/.local/share/easyeda-agent-harness/
```

The repository copy is the durable recovery point. Restore deliberately and compare hashes before overwriting a newer EasyEDA workspace.

This milestone closes schematic regeneration, placement, routing, connectivity, DRC, and persistence verification. It does **not** close the fabrication release gates. Before ordering boards, still complete:

1. independent schematic/layout design review;
2. Gerber and drill generation plus visual inspection;
3. BOM and pick-and-place reconciliation;
4. programming-fixture regeneration for the current pogo geometry;
5. manufacturer-rule review and order-package archiving;
6. assembled-board bench validation of LDO temperature, LED load/inrush, PPTC behavior, pigtails, enclosure thermals, and RF range.

See [`../../DESIGN.md`](../../DESIGN.md) for architecture, firmware contract, safety assumptions, and release gates.
