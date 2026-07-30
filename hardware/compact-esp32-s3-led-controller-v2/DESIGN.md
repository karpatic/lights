# Compact ESP32-S3 LED Controller V2 — design record

Last reconciled: 2026-07-30

Status: **design in progress; not fabrication-ready**

Repository association: this controller is the hardware target for [`karpatic/lights`](https://github.com/karpatic/lights). The EasyEDA project and automation snapshot are backed up under [`snapshots/2026-07-30`](snapshots/2026-07-30/README.md).

## Product goal

Build a small, inexpensive, two-layer controller that:

- runs the `lights` effects and BLE control firmware on an ESP32-S3;
- accepts regulated 5 V from an enclosure-mounted connector and two-wire pigtail;
- passes protected 5 V, level-shifted data, and ground to an addressable LED rope;
- is programmed and recovered through a reusable six-probe fixture;
- lets the ESP32-S3-MINI-1 embedded-antenna module define nearly all of the PCB width;
- avoids carrying connector placement, programming electronics, and development-board conveniences on every production PCB.

## Project lineage

### Frozen V1

V1 is preserved and must not be modified:

- board: 42 × 28 mm;
- four copper layers;
- 34 footprints;
- 231 tracks;
- 41 vias;
- 21 copper areas;
- 20/20 connectivity;
- zero EasyEDA DRC errors.

V1 proved the electrical concept but is larger and more complicated than necessary.

### Active V2

- EasyEDA Standard 6.5.51 project: `Compact ESP32-S3 USB LED Controller V2`;
- working portrait target: approximately **18 × 32 mm**;
- two copper layers;
- ESP32-S3-MINI-1-N8 with embedded antenna;
- antenna at the head, circuitry corridor in the middle, cable/input landings at the foot;
- active placement and routing remain unverified.

The 18 × 32 mm dimensions are a placement/routing target, not a released mechanical specification.

## Target electrical architecture

```text
regulated 5V input pigtail
    |
    +-- TVS to GND
    |
    +-- resettable PPTC
            |
            +-- protected 5V --> LED output 5V
            |
            +-- AP2112K-3.3 LDO --> ESP32 3V3
            |
            +-- 5V-powered AHCT buffer

ESP32-S3 GPIO18 --> AHCT buffer --> 33 ohm series resistor --> LED DATA
common GND -----------------------------------------------------> LED GND
```

### Power input

Target interface:

```text
5V_IN
GND
```

Proposed landing:

- two plated through-holes;
- 3.00 mm pitch;
- 2.20 mm copper pads;
- 1.20 mm finished holes;
- regulated 5 V only.

The external USB-C, JST, barrel, or other connector should be mounted in the enclosure and wired to this landing. The enclosure—not the solder joints—must provide strain relief.

Do not label this input `BAT` or `LiPo`. A JST connector is acceptable only when it carries regulated 5 V.

A bare single-cell LiPo is unsupported. Supporting one would require a charger, cell protection, boost conversion, and a battery power-path/source-management design that V2 does not contain.

### LED output

Target interface:

```text
5V
DATA
GND
```

Proposed landing:

- three plated through-holes;
- 3.00 mm pitch;
- 2.20 mm copper pads;
- 1.20 mm finished holes;
- hand-soldered, replaceable pigtail;
- enclosure-provided strain relief.

External-facing connector type, contact gender, wire colors, wire gauge, pin order, and mating orientation must be verified on the actual purchased pigtail before fabrication.

## Connector decision and source lag

The preferred production direction is **no board-mounted USB-C connector**. It saves connector and anchor area, removes the two USB-C CC resistors, simplifies routing, permits arbitrary case-connector placement, and avoids exposing two simultaneous 5 V source paths.

The 2026-07-30 backed-up EasyEDA source still contains:

- a power-only USB-C receptacle;
- two 5.1 kΩ CC resistors;
- the old `VBUS_RAW` naming and input topology.

Therefore the snapshot documents the transition; it does not yet implement the final input decision. The builder, wiring, placement, routing, and regenerated schematic/PCB must all be updated together before release.

## ESP32 module and RF geometry

Selected module:

- Espressif ESP32-S3-MINI-1-N8;
- LCSC C2913206;
- approximately 15.4–15.5 × 20.5 mm body;
- approximately 5.05 mm antenna section.

The module defines nearly all of the board width. Its complete antenna region should project beyond the head edge.

The antenna region requires an all-layer and mechanical keepout:

- no carrier-board copper;
- no traces or vias;
- no components;
- no ground plane;
- no pogo-fixture metal;
- no enclosure metal or conductive filament;
- no cable routing through the antenna volume.

Range and enclosure detuning must be tested on assembled hardware.

## 3.3 V supply

Current implementation candidate:

- Diodes Incorporated AP2112K-3.3TRG1;
- LCSC C51118;
- nominal 3.3 V / 600 mA;
- SOT-23-5/SOT-25 footprint class.

The LDO was selected instead of the earlier switching-regulator architecture because 5 V-to-3.3 V conversion simplifies a small two-layer board substantially.

LDO dissipation follows:

```text
P = (5.0 V - 3.3 V) × ESP-side current
```

Examples:

- 100 mA: 0.17 W;
- 200 mA: 0.34 W;
- 300 mA: 0.51 W.

The LED rope remains on protected 5 V. The LDO powers only the ESP32-side 3.3 V circuitry. Wi-Fi/BLE load steps and enclosure temperature require bench validation.

## LED data level shifting

Retain the 5 V-powered `SN74AHCT1G125`-class buffer unless the final LED family and cable are proven reliable from direct 3.3 V signaling.

- ESP32 data source: GPIO18 in the current V2 wiring map;
- buffer input pull-down: 100 kΩ;
- buffer local decoupling: 100 nF;
- output damping resistor: 33 Ω;
- output: `LED_DATA` at the three-wire landing.

The AHCT input threshold gives substantially better margin than direct 3.3 V drive into many 5 V addressable LEDs.

## Input protection

### TVS

The compact input TVS remains the default. It clamps short transients; it is not an overcurrent fuse.

### Resettable PPTC

The original Littelfuse `1812L300/24SLER` (LCSC C1512049) was rejected on cost:

- approximately $2.206 at quantity 5;
- approximately $1.4333 at quantity 100.

Selected cost-down candidate:

- BHFUSE `BSMD1812-300-16V`;
- LCSC C883162;
- 1812 package;
- 3 A hold at 25°C;
- 6 A trip specification at 25°C;
- 16 V maximum;
- 40 A maximum fault current;
- 0.010 Ω initial minimum resistance;
- 0.050 Ω maximum post-reflow resistance;
- maximum 4 second trip at 8 A;
- observed LCSC pricing on 2026-07-30: $0.0863 at quantity 5 and $0.0667 at quantity 100.

It uses the same EasyEDA package UUID as the previous 1812 part, making it a mechanical drop-in.

Hold-current temperature derating from its datasheet:

| Ambient | Approximate hold current |
|---:|---:|
| 25°C | 3.00 A |
| 40°C | 2.55 A |
| 50°C | 2.28 A |
| 60°C | 2.01 A |
| 70°C | 1.61 A |
| 85°C | 1.33 A |

The PPTC is not guaranteed to trip at exactly 3 A. A current-limited USB supply may shut down before the PPTC. Its main value is protection against higher-current external 5 V sources and sustained wiring faults. LED startup current, normal voltage drop, trip/recovery behavior, warm-enclosure derating, and partial-fault heating must be tested.

The official ESP32-DevKitC V4 schematic does not contain an input fuse/PPTC. Its USB path uses a BAT760-7 Schottky diode and relies substantially on source current limiting. That board does not pass several amperes to an external LED cable, so its omission is not sufficient justification for omitting protection here.

Do not substitute a deliberately thin PCB trace as a fuse.

## Programming fixture

Controller-side functions:

```text
5V
GND
UART TX
UART RX
EN
BOOT / GPIO0
```

Current compact proposal:

- six bottom-side bare copper pads;
- two columns × three rows;
- exact 2.00 mm X/Y pitch;
- 1.20 × 1.20 mm pads;
- no BOM/PnP entries;
- top-left board chamfer as a physical anti-reversal key.

The fixture generator in the 2026-07-30 snapshot still expects the older 36 × 25 mm board, 2.54 mm pitch, and 1.50 mm pads. Its generated fixture artifacts are archived as stale reference only. Regenerate all CSV/SVG/DXF/OpenSCAD outputs from the final portrait PCB source and selected physical probe geometry.

The fixture must use current-limited 5 V and must not power the board while another 5 V input is attached unless deliberate source isolation is added.

## Manufacturing and cost record

Preliminary estimates discussed before final connector removal and final JLC quotation:

| Quantity | Bare PCB + parts, hand assembled | PnP assembled | External connector/pigtail allowance |
|---:|---:|---:|---:|
| 5 | $9–11/unit | $14–18/unit | $2–4/unit |
| 100 | $5.90–6.40/unit | $6.30–7.10/unit | $0.60–1.50/unit |

With the BHFUSE substitution, the provisional electronic BOM estimate became approximately:

- $5.90 per board at quantity 5;
- $4.37 per board at quantity 100.

These are planning figures, not quotes. They exclude shipping, tax, enclosure, test/programming labor, in-house pigtail labor, fixture amortization, and possible JLC extended-part/setup charges. Removing board-mounted USB-C and its CC resistors should lower the eventual BOM further.

At quantity 100, PnP is expected to be worth the small incremental recurring cost. At quantity 5, manual assembly can be cheaper only when labor is treated as free.

## Firmware compatibility contract

The current repository firmware predates this board and is not ready to ship on it.

Current mismatches:

1. `platformio.ini` targets `esp32dev`, not an ESP32-S3 target.
2. Firmware defaults LED data to GPIO15; V2 routes the buffer input from GPIO18.
3. Firmware defaults `maxCurrent` to 8000 mA, but the value is currently only parsed and printed—it does not enforce an electrical current ceiling.
4. Firmware permits BLE updates to `pixelPin` after the `Adafruit_NeoPixel` object has already been constructed, so changing the field does not necessarily rebind the physical output.
5. A 300-pixel default with unconstrained full-white output can greatly exceed the intended controller, supply, pigtail, and enclosure current budget.
6. UART programming/recovery must work with the six-pad EN/BOOT fixture flow.

Before hardware release, firmware needs:

- an ESP32-S3 PlatformIO environment compatible with ESP32-S3-MINI-1-N8;
- GPIO18 as the fixed production LED-data pin, or an explicit safe reinitialization path;
- a real current estimator/limiter tied to LED count, brightness, and color output;
- a conservative product current ceiling validated with the selected supply and enclosure;
- pogo-fixture flash and recovery validation;
- BLE and RF range tests in the final enclosure.

Firmware current limiting is mandatory but does not replace correctly rated copper, connectors, pigtails, source protection, PPTC, and strain relief.

## Open design work

1. Remove board-mounted USB-C and both CC resistors from the canonical V2 implementation.
2. Add the two-hole `5V_IN/GND` landing.
3. Regenerate the schematic so the active project actually contains C883162.
4. Reconcile every part and net against the target power architecture.
5. Complete the approximately 18 × 32 mm portrait placement.
6. Route the portrait board on two layers while preserving a useful ground reference.
7. Verify that all protected 5 V current passes through the PPTC with no bypass.
8. Size power copper for the validated product current and temperature rise.
9. Run complete connectivity and EasyEDA DRC.
10. Validate the ESP32 footprint and antenna keepout against Espressif documentation.
11. Regenerate and physically validate the pogo fixture.
12. Reconcile BOM and PnP outputs with actual JLC stock/classification.
13. Generate and independently inspect Gerbers and drills.
14. Bench-test LDO temperature and 3.3 V transients.
15. Bench-test LED inrush, PPTC drop/trip/recovery, short behavior, and partial-fault heating.
16. Confirm pigtail polarity, contact gender, wire gauge, and strain relief.
17. Implement and test the firmware compatibility contract above.

## Release gate

Do not label V2 fabrication-ready until all of the following are true:

- final schematic and PCB agree with this target architecture;
- complete connectivity passes;
- zero unexplained DRC violations;
- no PPTC bypass;
- adequate 5 V copper width and thermal behavior;
- valid all-layer antenna keepout;
- production BOM and PnP reconciliation;
- Gerber/drill parsing and visual inspection;
- source-derived fixture coordinates and printed coupon validation;
- LDO, LED load, fuse, cable, and enclosure thermal tests;
- firmware current limiting and ESP32-S3/GPIO18 compatibility verified.
