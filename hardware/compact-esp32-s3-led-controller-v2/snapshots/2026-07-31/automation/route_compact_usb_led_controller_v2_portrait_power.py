#!/usr/bin/env python3
"""First-milestone power routing for the V2 18x32-mm portrait PCB.

Normal mode follows the EasyEDA harness pattern: read the live source, replace
only agent_v2_portrait_* objects, applySource(createNew=False), create missing
vias natively, refill, verify, and save.  --validate-static performs the same
source-derived generation against the repaired snapshot without invoking CDP.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import math
import re
import time
from pathlib import Path

ROOT = Path.home() / ".local/share/easyeda-agent-harness"
SNAPSHOT = Path("/tmp/easyeda-v2-repaired-live-pcb.json")
STATE = ROOT / "compact_usb_led_controller_v2_state.json"
PREFIX = "agent_v2_portrait_"
RETIRED_VIA_SITES = [
    {"net": "5V", "x": 4052.0, "y": 3519.8},
    {"net": "GND", "x": 4039.0, "y": 3498.0},
    {"net": "GND", "x": 4058.0, "y": 3507.0},
    {"net": "GND", "x": 4066.0, "y": 3507.0},
    {"net": "GND", "x": 4059.441, "y": 3505.0},
    {"net": "GND", "x": 4059.8, "y": 3505.5},
]
RETIRED_TRACK_IDS = {
    PREFIX + "5v_branch_reg",
    PREFIX + "gnd_island_bridge_top",
}

# EasyEDA units: 1 unit = 0.254 mm.
BOARD = [
    (4025.906, 3425.000),
    (4090.866, 3425.000),
    (4090.866, 3550.984),
    (4020.000, 3550.984),
    (4020.000, 3430.906),
]
GND_POUR = [
    (4038.000, 3426.200),
    (4089.600, 3426.200),
    (4089.600, 3549.800),
    (4021.200, 3549.800),
    (4021.200, 3464.000),
    (4038.000, 3464.000),
]
TOP_GND_POUR = [
    (4021.200, 3485.500),
    (4089.600, 3485.500),
    (4089.600, 3549.800),
    (4021.200, 3549.800),
]

EXPECTED = {
    ("J_5V_IN", "1"): ("5V_IN", 4082.500, 3541.811, 0, "11"),
    ("J_5V_IN", "2"): ("GND", 4070.689, 3541.811, 0, "11"),
    ("D_VBUS", "1"): ("5V_IN", 4078.000, 3518.9525, 90, "1"),
    ("D_VBUS", "2"): ("GND", 4078.000, 3513.0475, 270, "1"),
    ("F_VBUS", "1"): ("5V_IN", 4082.500, 3528.543, 180, "1"),
    ("F_VBUS", "2"): ("5V", 4067.906, 3528.543, 180, "1"),
    ("J_LED", "1"): ("5V", 4024.500, 3504.189, 0, "11"),
    ("J_LED", "3"): ("GND", 4024.500, 3527.811, 0, "11"),
    ("C_VBUS_IN", "1"): ("5V", 4036.437, 3492.000, 180, "1"),
    ("C_VBUS_IN", "2"): ("GND", 4028.563, 3492.000, 180, "1"),
    ("U_3V3", "1"): ("5V", 4042.882, 3488.2609, 270, "1"),
    ("U_3V3", "2"): ("GND", 4042.882, 3491.9979, 270, "1"),
    ("U_3V3", "3"): ("5V", 4042.882, 3495.7389, 270, "1"),
    ("U_3V3", "5"): ("3V3", 4053.118, 3488.2609, 270, "1"),
    ("C_3V3_OUT1", "1"): ("3V3", 4078.063, 3490.000, 0, "1"),
    ("C_3V3_OUT1", "2"): ("GND", 4085.937, 3490.000, 0, "1"),
    ("U_LEVEL", "1"): ("GND", 4059.441, 3502.3305, 0, "1"),
    ("U_LEVEL", "3"): ("GND", 4064.559, 3502.3305, 0, "1"),
    ("U_LEVEL", "5"): ("5V", 4059.441, 3493.6695, 0, "1"),
    ("C_LEVEL", "1"): ("5V", 4079.244, 3496.000, 0, "1"),
    ("C_LEVEL", "2"): ("GND", 4084.756, 3496.000, 0, "1"),
    ("TP_5V", "1"): ("5V", 4052.000, 3523.748, 0, "2"),
}


def load_json(path: Path) -> dict:
    with path.open() as f:
        return json.load(f)


def pad_index(src: dict, state: dict) -> dict[tuple[str, str], dict]:
    out = {}
    for alias, gid in state["ids"].items():
        fp = src.get("FOOTPRINT", {}).get(gid)
        assert fp, f"missing footprint {alias}:{gid}"
        for pad in fp.get("PAD", {}).values():
            out[(alias, str(pad.get("number")))] = pad
    return out


def assert_expected(pads: dict[tuple[str, str], dict]) -> None:
    for key, (net, x, y, rot, layer) in EXPECTED.items():
        p = pads[key]
        got = (
            p.get("net"),
            round(float(p["x"]), 4),
            round(float(p["y"]), 4),
            int(float(p.get("rotation", 0))),
            str(p.get("layerid")),
        )
        exp = (net, round(x, 4), round(y, 4), rot, layer)
        assert got == exp, f"{key} expected {exp}, got {got}"


def pt(pads: dict[tuple[str, str], dict], alias: str, num: str) -> tuple[float, float]:
    p = pads[(alias, num)]
    return (float(p["x"]), float(p["y"]))


def copper_layer_for_pad(pad: dict) -> str:
    layer = str(pad.get("layerid"))
    return "2" if layer == "11" else layer


def track(gid: str, net: str, layer: str, width: float, points: list[tuple[float, float]]) -> dict:
    return {
        "gId": gid,
        "layerid": layer,
        "net": net,
        "pointArr": [{"x": round(x, 4), "y": round(y, 4)} for x, y in points],
        "strokeWidth": width,
        "locked": 0,
    }


def pathstr(points: list[tuple[float, float]]) -> str:
    head, *tail = points
    return "M %.4f %.4f %s Z" % (
        head[0],
        head[1],
        " ".join("L %.4f %.4f" % p for p in tail),
    )


def via(gid: str, net: str, x: float, y: float, diameter: float = 2.4, hole_r: float = 0.6) -> dict:
    return {"gId": gid, "net": net, "x": x, "y": y, "diameter": diameter, "holeR": hole_r}


def build_objects(src: dict, state: dict) -> tuple[dict[str, dict], dict[str, dict], list[dict]]:
    pads = pad_index(src, state)
    assert_expected(pads)

    j5v = pt(pads, "J_5V_IN", "1")
    d1 = pt(pads, "D_VBUS", "1")
    f1 = pt(pads, "F_VBUS", "1")
    f2 = pt(pads, "F_VBUS", "2")
    led5 = pt(pads, "J_LED", "1")
    cvin = pt(pads, "C_VBUS_IN", "1")
    u3v3_1 = pt(pads, "U_3V3", "1")
    u3v3_3 = pt(pads, "U_3V3", "3")
    u3v3_5 = pt(pads, "U_3V3", "5")
    clevel = pt(pads, "C_LEVEL", "1")
    ulevel5 = pt(pads, "U_LEVEL", "5")
    c3v3 = pt(pads, "C_3V3_OUT1", "1")
    tp5v = pt(pads, "TP_5V", "1")

    tracks = {
        PREFIX + "5vin_j_d_f": track(
            PREFIX + "5vin_j_d_f",
            "5V_IN",
            "1",
            4.0,
            [j5v, (4082.5, 3530.5), f1, (4082.5, 3523.0), d1],
        ),
        PREFIX + "5v_trunk_f_led": track(
            PREFIX + "5v_trunk_f_led",
            "5V",
            "1",
            4.0,
            [f2, (4058.0, 3528.543), (4051.0, 3517.5), (4036.0, 3511.0), led5],
        ),
        PREFIX + "5v_branch_cvin": track(
            PREFIX + "5v_branch_cvin", "5V", "1", 1.5, [(4036.0, 3511.0), cvin]
        ),
        PREFIX + "5v_branch_reg_pin3": track(
            PREFIX + "5v_branch_reg_pin3",
            "5V",
            "1",
            1.5,
            [cvin, (4036.437, 3495.7389), u3v3_3],
        ),
        PREFIX + "5v_branch_reg_pin1": track(
            PREFIX + "5v_branch_reg_pin1",
            "5V",
            "1",
            1.5,
            [cvin, (4036.437, 3488.2609), u3v3_1],
        ),
        PREFIX + "5v_branch_level": track(
            PREFIX + "5v_branch_level",
            "5V",
            "1",
            1.2,
            [(4058.0, 3528.543), (4056.5, 3512.0), (4056.5, 3493.6695), ulevel5],
        ),
        PREFIX + "5v_branch_clevel": track(
            PREFIX + "5v_branch_clevel",
            "5V",
            "1",
            1.0,
            [ulevel5, (4059.441, 3498.5), (4074.0, 3498.5), clevel],
        ),
        PREFIX + "5v_to_tp_top": track(
            PREFIX + "5v_to_tp_top", "5V", "1", 2.0, [(4036.0, 3511.0), (4044.0, 3520.0), (4044.0, 3523.748)]
        ),
        PREFIX + "5v_to_tp_bottom": track(
            PREFIX + "5v_to_tp_bottom", "5V", "2", 2.0, [(4044.0, 3523.748), tp5v]
        ),
        PREFIX + "3v3_u_cout": track(
            PREFIX + "3v3_u_cout", "3V3", "1", 2.0, [u3v3_5, (4064.0, 3488.2609), (4068.0, 3490.0), c3v3]
        ),
        PREFIX + "3v3_esp_escape_top": track(
            PREFIX + "3v3_esp_escape_top", "3V3", "1", 1.0, [pt(pads, "U_ESP", "3"), (4031.5, 3438.7675)]
        ),
        PREFIX + "3v3_esp_dec_bottom": track(
            PREFIX + "3v3_esp_dec_bottom", "3V3", "2", 1.0, [(4031.5, 3438.7675), pt(pads, "C_ESP_DEC", "1")]
        ),
        PREFIX + "3v3_boot_pullup_bottom": track(
            PREFIX + "3v3_boot_pullup_bottom",
            "3V3",
            "2",
            1.0,
            [(4031.5, 3438.7675), (4031.5, 3434.5), (4044.0, 3434.5), (4044.0, 3447.0), pt(pads, "R_BOOT", "1")],
        ),
        PREFIX + "3v3_backbone_bottom": track(
            PREFIX + "3v3_backbone_bottom",
            "3V3",
            "2",
            1.0,
            [(4031.5, 3438.7675), (4031.5, 3434.5), (4074.0, 3434.5), (4074.0, 3487.5)],
        ),
        PREFIX + "3v3_main_tie_top": track(
            PREFIX + "3v3_main_tie_top", "3V3", "1", 1.0, [(4074.0, 3487.5), c3v3]
        ),
        PREFIX + "3v3_r_en_bottom": track(
            PREFIX + "3v3_r_en_bottom", "3V3", "2", 1.0, [(4074.0, 3487.5), (4074.0, 3506.0), (4070.0, 3509.0)]
        ),
        PREFIX + "3v3_r_en_top": track(
            PREFIX + "3v3_r_en_top", "3V3", "1", 1.0, [(4070.0, 3509.0), pt(pads, "R_EN", "1")]
        ),
    }

    gnd_pairs = [
        ("d_vbus_gnd", "D_VBUS", "2", (4084.5, 3513.0), 1.0),
        ("c_vbus_in_gnd", "C_VBUS_IN", "2", (4023.2, 3498.0), 1.0),
        ("u_3v3_gnd", "U_3V3", "2", (4048.0, 3492.0), 1.0),
        ("c_3v3_out_gnd", "C_3V3_OUT1", "2", (4085.9, 3497.0), 1.0),
        ("c_level_gnd", "C_LEVEL", "2", (4085.8, 3502.0), 1.0),
        ("u_level_oe_gnd", "U_LEVEL", "1", (4059.5, 3505.5), 1.0),
        ("u_level_gnd", "U_LEVEL", "3", (4064.559, 3505.0), 1.0),
        ("j_led_gnd", "J_LED", "3", (4031.0, 3527.8), 1.2),
        ("j_5v_in_gnd", "J_5V_IN", "2", (4063.5, 3541.8), 1.2),
    ]
    for name, alias, num, xy, width in gnd_pairs:
        tracks[PREFIX + "gnd_" + name] = track(
            PREFIX + "gnd_" + name, "GND", copper_layer_for_pad(pads[(alias, num)]), width, [pt(pads, alias, num), xy]
        )
    for name, alias, num, xy in (
        ("r_level_pd", "R_LEVEL_PD", "2", (4023.2, 3498.0)),
        ("c_en", "C_EN", "2", (4085.8, 3502.0)),
    ):
        tracks[PREFIX + "gnd_" + name] = track(
            PREFIX + "gnd_" + name,
            "GND",
            "1",
            1.0,
            [pt(pads, alias, num), xy],
        )
    tracks[PREFIX + "gnd_c_esp_dec_bottom"] = track(
        PREFIX + "gnd_c_esp_dec_bottom",
        "GND",
        "2",
        1.0,
        [pt(pads, "C_ESP_DEC", "2"), (4039.5, 3438.5)],
    )
    tracks[PREFIX + "gnd_c_esp_dec_to_exposed"] = track(
        PREFIX + "gnd_c_esp_dec_to_exposed",
        "GND",
        "1",
        1.0,
        [(4039.5, 3438.5), (4048.937, 3449.0035)],
    )
    tracks[PREFIX + "gnd_tp_gnd_bottom"] = track(
        PREFIX + "gnd_tp_gnd_bottom",
        "GND",
        "2",
        1.0,
        [pt(pads, "TP_GND", "1"), (4059.8, 3505.5)],
    )
    esp_fp = src["FOOTPRINT"][state["ids"]["U_ESP"]]
    esp_gnd_points = {
        (round(float(p["x"]), 4), round(float(p["y"]), 4))
        for p in esp_fp.get("PAD", {}).values()
        if p.get("net") == "GND"
    }
    required_esp_gnd = {
        (4027.874, 3427.9405),
        (4082.992, 3427.9405),
        (4027.874, 3483.0585),
        (4082.992, 3483.0585),
    }
    assert required_esp_gnd <= esp_gnd_points, (required_esp_gnd, esp_gnd_points)
    top_row = [pt(pads, "U_ESP", str(number)) for number in range(60, 45, -1)]
    tracks[PREFIX + "gnd_esp_top_row"] = track(
        PREFIX + "gnd_esp_top_row", "GND", "1", 1.0, top_row
    )
    tracks[PREFIX + "gnd_esp_top_left_corner"] = track(
        PREFIX + "gnd_esp_top_left_corner", "GND", "1", 1.0, [(4027.874, 3427.9405), top_row[0]]
    )
    tracks[PREFIX + "gnd_esp_left_side"] = track(
        PREFIX + "gnd_esp_left_side",
        "GND",
        "1",
        1.0,
        [(4027.874, 3427.9405), pt(pads, "U_ESP", "1"), pt(pads, "U_ESP", "2")],
    )
    tracks[PREFIX + "gnd_esp_top_right_corner"] = track(
        PREFIX + "gnd_esp_top_right_corner", "GND", "1", 1.0, [top_row[-1], (4082.992, 3427.9405), (4086.5, 3427.843)]
    )
    tracks[PREFIX + "gnd_esp_right_edge_spine"] = track(
        PREFIX + "gnd_esp_right_edge_spine",
        "GND",
        "1",
        1.0,
        [(4086.5, 3427.843), (4089.0, 3430.0), (4089.0, 3483.0585), (4082.992, 3483.0585)],
    )
    for number in (43, 42):
        tracks[PREFIX + f"gnd_esp_right_{number}"] = track(
            PREFIX + f"gnd_esp_right_{number}",
            "GND",
            "1",
            1.0,
            [pt(pads, "U_ESP", str(number)), (4086.5, float(pads[("U_ESP", str(number))]["y"])), (4086.5, 3427.843)],
        )
    areas = {
        PREFIX + "gnd_bottom_pour": {
            "gId": PREFIX + "gnd_bottom_pour",
            "layerid": "2",
            "net": "GND",
            "name": "V2 Portrait Bottom GND Pour",
            "order": "8",
            "pathStr": pathstr(GND_POUR),
            "clearanceWidth": 1,
            "fillStyle": "solid",
            "strokeWidth": 1,
            "thermal": "spoke",
            "keepIsland": "none",
            "locked": 0,
            "gridTrackWidth": 1,
            "gridClearance": 1,
            "toBoardOutline": 0,
            "fabricationImprove": "yes",
            "spoke_width": 1,
        },
        PREFIX + "gnd_top_lower_pour": {
            "gId": PREFIX + "gnd_top_lower_pour",
            "layerid": "1",
            "net": "GND",
            "name": "V2 Portrait Lower Top GND Pour",
            "order": "9",
            "pathStr": pathstr(TOP_GND_POUR),
            "clearanceWidth": 1,
            "fillStyle": "solid",
            "strokeWidth": 1,
            "thermal": "spoke",
            "keepIsland": "none",
            "locked": 0,
            "gridTrackWidth": 1,
            "gridClearance": 1,
            "toBoardOutline": 0,
            "fabricationImprove": "yes",
            "spoke_width": 1,
        },
    }

    vias = [
        via(PREFIX + "via_5v_tp_transition", "5V", 4044.0, 3523.748, 3.0, 0.8),
        via(PREFIX + "via_3v3_esp", "3V3", 4031.5, 3438.7675),
        via(PREFIX + "via_3v3_main", "3V3", 4074.0, 3487.5),
        via(PREFIX + "via_3v3_r_en", "3V3", 4070.0, 3509.0),
    ]
    for name, _, _, xy, _ in gnd_pairs:
        vias.append(via(PREFIX + "via_gnd_" + name, "GND", xy[0], xy[1]))
    for ix, x in enumerate((4048.937, 4055.433, 4061.929), 1):
        for iy, y in enumerate((3449.0035, 3455.4995, 3461.9955), 1):
            vias.append(via(PREFIX + f"via_gnd_esp_exposed_{ix}_{iy}", "GND", x, y))
    vias.extend(
        [
            via(PREFIX + "via_gnd_esp_top", "GND", 4086.5, 3427.843),
            via(PREFIX + "via_gnd_esp_bottom_left", "GND", 4027.874, 3483.0585),
            via(PREFIX + "via_gnd_esp_bottom_right", "GND", 4082.992, 3483.0585),
            via(PREFIX + "via_gnd_c_esp_dec", "GND", 4039.5, 3438.5),
            via(PREFIX + "via_gnd_stitch_central_lower", "GND", 4040.0, 3503.5),
        ]
    )
    return tracks, areas, vias


def point_in_poly(point: tuple[float, float], poly: list[tuple[float, float]]) -> bool:
    x, y = point
    inside = False
    j = len(poly) - 1
    for i, pi in enumerate(poly):
        xi, yi = pi
        xj, yj = poly[j]
        if ((yi > y) != (yj > y)) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
            inside = not inside
        j = i
    return inside


def validate_geometry(tracks: dict[str, dict], areas: dict[str, dict], vias: list[dict]) -> None:
    for tr in tracks.values():
        assert tr["layerid"] in {"1", "2"}, f"bad track layer {tr['gId']}:{tr['layerid']}"
        for p in tr["pointArr"]:
            assert point_in_poly((float(p["x"]), float(p["y"])), BOARD), f"track point off board {tr['gId']}:{p}"
    for area in areas.values():
        assert area["layerid"] in {"1", "2"}, f"bad area layer {area['gId']}:{area['layerid']}"
        pts = [(float(x), float(y)) for x, y in re.findall(r"[ML] ([0-9.]+) ([0-9.]+)", area["pathStr"])]
        for p in pts:
            assert point_in_poly(p, BOARD), f"copper area vertex off board {area['gId']}:{p}"
            assert not (p[0] < 4038.0 and p[1] < 3464.0), f"area enters antenna keepout {area['gId']}:{p}"
    for v in vias:
        assert point_in_poly((float(v["x"]), float(v["y"])), BOARD), f"via off board {v}"
    assert len([v for v in vias if v["net"] == "5V"]) == 1, "5V bottom transition must be exactly one via"


def remove_generated(
    src: dict,
    vias: list[dict],
    track_ids: set[str],
    area_ids: set[str],
) -> None:
    order = src.setdefault("itemOrder", [])
    for gid in list(src.setdefault("TRACK", {})):
        if gid in track_ids or gid in RETIRED_TRACK_IDS:
            del src["TRACK"][gid]
            while gid in order:
                order.remove(gid)
    for gid in list(src.setdefault("COPPERAREA", {})):
        if gid in area_ids:
            del src["COPPERAREA"][gid]
            while gid in order:
                order.remove(gid)
    sites = [*vias, *RETIRED_VIA_SITES]
    for gid, obj in list(src.setdefault("VIA", {}).items()):
        owned = gid in {plan["gId"] for plan in vias} or any(
            obj.get("net") == site["net"]
            and math.hypot(float(obj.get("x", 0)) - site["x"], float(obj.get("y", 0)) - site["y"]) <= 0.03
            for site in sites
        )
        if owned:
            del src["VIA"][gid]
            while gid in order:
                order.remove(gid)


def apply_generated(src: dict, tracks: dict[str, dict], areas: dict[str, dict], vias: list[dict]) -> dict:
    remove_generated(src, vias, set(tracks), set(areas))
    for layer in ("1", "2"):
        src["layers"][layer]["config"] = True
        src["layers"][layer]["visible"] = True
    for gid, tr in tracks.items():
        src.setdefault("TRACK", {})[gid] = tr
        src.setdefault("itemOrder", []).append(gid)
    for gid, area in areas.items():
        src.setdefault("COPPERAREA", {})[gid] = area
        src.setdefault("itemOrder", []).append(gid)
    return src


def has_via(src: dict, plan: dict, tol: float = 0.03) -> bool:
    for v in src.get("VIA", {}).values():
        if v.get("net") != plan["net"]:
            continue
        if math.hypot(float(v.get("x", 0)) - plan["x"], float(v.get("y", 0)) - plan["y"]) <= tol:
            return True
    return False


def static_validate() -> None:
    src = load_json(SNAPSHOT)
    state = load_json(STATE)
    tracks, areas, vias = build_objects(src, state)
    validate_geometry(tracks, areas, vias)
    staged = apply_generated(json.loads(json.dumps(src)), tracks, areas, vias)
    assert all(g in staged["TRACK"] for g in tracks), "missing generated tracks"
    assert all(g in staged["COPPERAREA"] for g in areas), "missing generated copper areas"
    print(json.dumps({"tracks": len(tracks), "areas": len(areas), "planned_native_vias": len(vias)}, indent=2))


def normal_apply() -> None:
    spec = importlib.util.spec_from_file_location("builder", ROOT / "build_esp32_lipo_led_board.py")
    assert spec and spec.loader
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    state = load_json(STATE)
    c = builder.CDP()
    try:
        src = builder.source(c)
        tracks, areas, vias = build_objects(src, state)
        validate_geometry(tracks, areas, vias)
        c.api("applySource", {"source": apply_generated(src, tracks, areas, vias), "createNew": False})
        time.sleep(10)
        live = builder.source(c)
        for v in vias:
            if not has_via(live, v):
                c.api(
                    "createShape",
                    {
                        "shapeType": "VIA",
                        "jsonCache": {
                            "x": v["x"],
                            "y": v["y"],
                            "net": v["net"],
                            "diameter": v["diameter"],
                            "holeR": v["holeR"],
                            "layerid": 11,
                            "locked": "0",
                        },
                    },
                )
                time.sleep(0.25)
        time.sleep(3)
        refill = builder.source(c)
        for gid in areas:
            refill.get("COPPERAREA", {}).get(gid, {}).pop("polygonArr", None)
        c.api("applySource", {"source": refill, "createNew": False})
        time.sleep(10)
        final = builder.source(c)
        missing = [g for g in tracks if g not in final.get("TRACK", {})]
        missing += [g for g in areas if g not in final.get("COPPERAREA", {})]
        missing_vias = [v["gId"] for v in vias if not has_via(final, v)]
        fills = {g: len(final.get("COPPERAREA", {}).get(g, {}).get("polygonArr", [])) for g in areas}
        if missing or missing_vias or not all(fills.values()):
            raise RuntimeError({"missing": missing, "missing_vias": missing_vias, "fills": fills})
        c.api("doCommand", {"cmd": "file_save"})
        time.sleep(2)
        print(json.dumps({"tracks": len(tracks), "areas": len(areas), "native_vias": len(vias), "fills": fills}, indent=2))
    finally:
        c.close()


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--validate-static", action="store_true")
    args = ap.parse_args()
    if args.validate_static:
        static_validate()
    else:
        normal_apply()


if __name__ == "__main__":
    main()
