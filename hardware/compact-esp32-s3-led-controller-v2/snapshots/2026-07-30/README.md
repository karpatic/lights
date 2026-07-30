# EasyEDA V2 recovery snapshot — 2026-07-30

This directory captures the mutable V2 design workspace at the point where it was associated with `karpatic/lights`.

It is a **recovery/reference snapshot, not a fabrication release**.

## Contents

- `easyeda/`: saved EasyEDA Standard project and PCB JSON plus the native `info` metadata needed for project restoration. The `info` BOM fields are stale and are not sourcing authority.
- `automation/`: current deterministic V2 schematic, wiring, two-layer configuration, portrait placement, and generated component-ID state.
- `automation/dependencies/`: the shared EasyEDA/CDP helper imported by the V2 scripts.
- `archive-stale/`: the old 36 × 25 mm routing and fixture-generation scripts. No compatible portrait routing script existed at snapshot time.
- `fixture-stale/`: fixture outputs from the older 36 × 25 mm / 2.54 mm-pitch geometry. They are preserved for history only and must not be manufactured.
- `MANIFEST.sha256`: integrity hashes for every backed-up artifact except the manifest itself.

## Authority and limitations

The live working sources remain:

```text
/home/carlos/.easyeda/projects/Compact ESP32-S3 USB LED Controller V2/
/home/carlos/.local/share/easyeda-agent-harness/
```

The repository copy is the durable association and recovery point. If work resumes from it, restore deliberately and compare hashes/diffs rather than overwriting a newer EasyEDA workspace.

Known source/design drift at snapshot time:

- active source still includes board-mounted power-only USB-C and CC resistors;
- target architecture uses a two-wire regulated-5-V pigtail input instead;
- the canonical builder names the new BHFUSE C883162, but the active EasyEDA project had not yet been rebuilt from that builder;
- portrait placement targets approximately 18 × 32 mm;
- portrait routing is incomplete/unverified;
- fixture generator and archived fixture files still describe the older 36 × 25 mm board, 2.54 mm pitch, and 1.50 mm pads;
- no final connectivity, DRC, Gerber, drill, BOM, or PnP release exists.

See [`../../DESIGN.md`](../../DESIGN.md) for the current architecture, firmware contract, safety assumptions, and release gates.
