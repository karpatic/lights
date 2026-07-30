# V2 pogo programming fixture

Authority: generated from the active EasyEDA PCB source. Units are millimetres.

## Datum and orientation

- Carrier PCB extents: **36.000 × 25.000 mm**.
- Board-top datum: top-left carrier extent; +X right, +Y toward the USB/pigtail edge.
- Top-left PCB corner has a **1.500 mm chamfer**, providing physical anti-reversal keying.
- Embedded antenna: top edge, projecting **5.05 mm** beyond the carrier.
- USB-C: bottom-right, projecting approximately **2.47 mm** beyond the carrier.
- Three-wire pigtail exits bottom-left.

## Pogo field

- Six bottom-side pads, each **1.50 × 1.50 mm**.
- Two columns × three rows, **2.54 mm** X/Y pitch.
- Exact source-derived centers are in `pogo-fixture-coordinates.csv`.
- `board_bottom_view_x_mm = 36.000 - board_top_x_mm`; Y is unchanged.
- Important: mirrored bottom-view coordinates are for artwork/viewing. A probe plate machined while the PCB remains top-facing-up uses `jig_probe_plate_x_mm/y_mm`, the physical board projection.

Board-top physical map:

```text
BOOT   GND
TX     EN
5V     RX
```

Fixture TX connects board RX; fixture RX connects board TX. Assert BOOT low while pulsing EN low, then release BOOT after reset to enter the ROM loader. Never fixture-power 5V while USB-C is also attached: this design has no source ORing. Use current-limited fixture power.

## Probe/mechanical starting parameters

- Suggested probe tip diameter: ≤1.0 mm.
- Nominal printed probe-barrel bore: 1.35 mm. This is deliberately a starting value—not a selected-probe specification. Print a bore coupon and update it for the real barrel/sleeve and printer shrinkage.
- Suggested working compression: 1.3–1.7 mm, subject to the selected probe datasheet.
- Four nonconductive 3.0-mm support-post locations are in `pogo-fixture-mechanical.csv`.
- Suggested clamps are away from the antenna overhang, USB shell, and pigtail exit.
- Use nonconductive fixture material near the antenna; avoid conductive/carbon-filled filament.

## Files

- `pogo-fixture-coordinates.csv`: electrical pad coordinates in top, mirrored-bottom, and actual probe-plate frames.
- `pogo-fixture-mechanical.csv`: support, clamp, and fixture mounting features.
- `pogo-fixture-parameters.json`: full machine-readable geometry and caveats.
- `pogo-fixture-dimensioned.svg`: dimensioned top and mirrored-bottom drawing.
- `pogo-fixture-board-top.dxf`: board-top CAD geometry.
- `pogo-fixture-v2.scad`: printable probe-plate source; OpenSCAD was not installed locally, so STL export remains to be run after selecting the real pogo barrel.
