#!/usr/bin/env python3
"""Deterministic 19 x 23 mm routing for one-side JLC assembly.

U_ESP is the only top-side SMT component and is intentionally user-installed.
All other SMT parts and fixture pads are bottom-side. Top copper is reserved for
short ESP escapes and ordered corridors below/outside the module body.
"""
from __future__ import annotations

import importlib.util
import json
import math
import time
from pathlib import Path

ROOT = Path.home() / ".local/share/easyeda-agent-harness"
STATE = ROOT / "compact_usb_led_controller_v2_state.json"
PREFIX = "agent_v2_single_side_"
OLD_PREFIXES = ("agent_v2_distributed_", PREFIX)
BOARD = [
    (4023.9375, 3425.0),
    (4092.8346, 3425.0),
    (4092.8346, 3515.5512),
    (4018.0315, 3515.5512),
    (4018.0315, 3430.906),
]
POUR = [
    (4024.45, 3426.2),
    (4091.634, 3426.2),
    (4091.634, 3514.3512),
    (4019.2315, 3514.3512),
    (4019.2315, 3431.42),
]


def load_json(path: Path) -> dict:
    return json.loads(path.read_text())


def pad_index(src: dict, state: dict) -> dict[tuple[str, str], dict]:
    out = {}
    for alias, gid in state["ids"].items():
        fp = src.get("FOOTPRINT", {}).get(gid)
        assert fp, f"missing footprint {alias}:{gid}"
        for pad in fp.get("PAD", {}).values():
            out[(alias, str(pad.get("number")))] = pad
    return out


def pt(pads: dict, alias: str, number: str) -> tuple[float, float]:
    pad = pads[(alias, number)]
    return float(pad["x"]), float(pad["y"])


def track(gid: str, net: str, layer: str, width: float, points: list[tuple[float, float]]) -> dict:
    assert len(points) >= 2
    return {
        "gId": gid,
        "layerid": layer,
        "net": net,
        "pointArr": [{"x": round(x, 4), "y": round(y, 4)} for x, y in points],
        "strokeWidth": width,
        "locked": 0,
    }


def via(gid: str, net: str, x: float, y: float, diameter: float = 2.4, hole_r: float = 0.6) -> dict:
    return {"gId": gid, "net": net, "x": x, "y": y, "diameter": diameter, "holeR": hole_r}


def pathstr(points: list[tuple[float, float]]) -> str:
    head, *tail = points
    return "M %.4f %.4f %s Z" % (head[0], head[1], " ".join("L %.4f %.4f" % p for p in tail))


def point_in_poly(point: tuple[float, float], poly: list[tuple[float, float]]) -> bool:
    x, y = point
    inside = False
    j = len(poly) - 1
    for i, (xi, yi) in enumerate(poly):
        xj, yj = poly[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
            inside = not inside
        j = i
    return inside


def add(tracks: dict, name: str, net: str, layer: str, width: float, points: list[tuple[float, float]]) -> None:
    gid = PREFIX + name
    tracks[gid] = track(gid, net, layer, width, points)


def build_objects(src: dict, state: dict) -> tuple[dict[str, dict], dict[str, dict], list[dict]]:
    pads = pad_index(src, state)
    tracks: dict[str, dict] = {}

    # Raw input protection is entirely bottom-side and close to the input row.
    j5 = pt(pads, "J_5V_IN", "1")
    f_in = pt(pads, "F_VBUS", "1")
    f_out = pt(pads, "F_VBUS", "2")
    d_in = pt(pads, "D_VBUS", "1")
    add(tracks, "5vin_input", "5V_IN", "2", 4.0, [j5, (f_in[0], 3499.4672), f_in])
    add(tracks, "5vin_tvs", "5V_IN", "2", 1.2, [(f_in[0], 3499.4672), (4089.0, 3495.7636), d_in])

    # Protected 5 V: short bottom feeds for the level shifter, and one top
    # corridor below U1 for the LED output and remote LDO branch.
    v5_main = (4070.7034, 3500.5)
    v5_reg = (4023.5, 3479.0)
    add(tracks, "5v_main_escape_bottom", "5V", "2", 4.0, [f_out, v5_main])
    add(
        tracks, "5v_main_top", "5V", "1", 4.0,
        [v5_main, (4059.0, 3500.5), (4053.4645, 3506.0355), pt(pads, "J_LED", "1")],
    )
    add(
        tracks, "5v_reg_top", "5V", "1", 1.5,
        [v5_main, (4023.5, 3500.5), v5_reg],
    )
    cvin = pt(pads, "C_VBUS_IN", "1")
    add(tracks, "5v_reg_to_cvin", "5V", "2", 1.5, [v5_reg, (4029.937, 3479.0), cvin])
    add(tracks, "5v_cvin_to_pin3", "5V", "2", 1.5, [cvin, (4029.937, 3463.794), pt(pads, "U_3V3", "3")])
    add(
        tracks, "5v_cvin_to_pin1", "5V", "2", 1.5,
        [cvin, (4023.0, 3465.063), (4023.0, 3449.379), pt(pads, "U_3V3", "1")],
    )
    add(tracks, "5v_level_feed", "5V", "2", 1.5, [f_out, (f_out[0], 3470.0), (4068.0, 3470.0), pt(pads, "U_LEVEL", "5")])
    add(tracks, "5v_level_decoupling", "5V", "2", 1.2, [f_out, (f_out[0], pt(pads, "C_LEVEL", "1")[1]), pt(pads, "C_LEVEL", "1")])
    v5_fixture = (4025.5, 3491.0)
    add(tracks, "5v_fixture_top", "5V", "1", 1.2, [(4025.5, 3500.5), v5_fixture])
    add(tracks, "5v_fixture_bottom", "5V", "2", 1.2, [v5_fixture, pt(pads, "TP_5V", "1")])

    # Local 3V3 generation and upper distribution rail beneath U1.
    uout = pt(pads, "U_3V3", "5")
    cout = pt(pads, "C_3V3_OUT1", "1")
    add(tracks, "3v3_local_output", "3V3", "2", 2.0, [uout, (4043.0, 3459.143), (4043.0, 3465.063), cout])
    add(tracks, "3v3_source_rise", "3V3", "2", 1.2, [uout, (4045.0, 3449.379), (4045.0, 3436.0)])
    v3_esp = (4023.5, 3438.767)
    add(
        tracks, "3v3_left_rail", "3V3", "2", 1.2,
        [(4045.0, 3436.0), (4042.5, 3433.5), (4028.767, 3433.5), v3_esp, pt(pads, "C_ESP_DEC", "1")],
    )
    add(tracks, "3v3_esp_top", "3V3", "1", 1.2, [v3_esp, pt(pads, "U_ESP", "3")])
    add(
        tracks, "3v3_boot_pullup", "3V3", "2", 1.0,
        [pt(pads, "R_BOOT", "1"), (4041.0, 3438.966), (4041.0, 3435.0), (4042.5, 3433.5)],
    )
    add(tracks, "3v3_en_pullup", "3V3", "2", 1.0, [(4045.0, 3436.0), pt(pads, "R_EN", "1")])

    # LED data path: one U1 escape, then a compact all-bottom local chain.
    v_led = (4050.0, 3478.0)
    add(tracks, "led_esp_top", "LED_DATA_3V3", "1", 1.0, [pt(pads, "U_ESP", "22"), v_led])
    add(tracks, "led_level_bottom", "LED_DATA_3V3", "2", 1.0, [v_led, pt(pads, "U_LEVEL", "2")])
    add(tracks, "led_pulldown", "LED_DATA_3V3", "2", 1.0, [pt(pads, "U_LEVEL", "2"), pt(pads, "R_LEVEL_PD", "1")])
    add(
        tracks, "led_buffered", "LED_BUF_OUT", "2", 1.0,
        [pt(pads, "U_LEVEL", "4"), (4059.0, 3484.390), pt(pads, "R_DATA", "1")],
    )
    add(tracks, "led_output", "LED_DATA", "2", 1.2, [pt(pads, "R_DATA", "2"), pt(pads, "J_LED", "2")])

    # BOOT: local pullup bottom-side; fixture route uses the empty top-left edge.
    v_boot = (4023.5, 3442.114)
    v_boot_fixture = (4019.5, 3478.0)
    add(tracks, "boot_esp_top", "BOOT", "1", 1.0, [pt(pads, "U_ESP", "4"), v_boot])
    add(tracks, "boot_pullup_bottom", "BOOT", "2", 1.0, [v_boot, pt(pads, "R_BOOT", "2")])
    add(
        tracks, "boot_fixture_top", "BOOT", "1", 1.0,
        [v_boot, (4019.5, 3446.114), v_boot_fixture],
    )
    add(tracks, "boot_fixture_bottom", "BOOT", "2", 1.0, [v_boot_fixture, (4019.5, 3484.0), pt(pads, "TP_BOOT", "1")])

    # UART and EN use ordered top corridors below U1. Their right-edge order
    # avoids crossings: TX turns first, RX second, EN last.
    v_tx = (4086.0, 3452.153)
    v_rx = (4088.5, 3448.807)
    v_en = (4091.0, 3432.074)
    v_tx_fixture = (4037.374, 3490.5)
    v_rx_fixture = (4046.5, 3493.5)
    v_en_fixture = (4045.8, 3496.5)
    add(tracks, "uart_tx_escape", "UART_TX", "1", 1.0, [pt(pads, "U_ESP", "39"), v_tx])
    add(
        tracks, "uart_tx_top", "UART_TX", "1", 1.0,
        [v_tx, (4086.0, 3490.5), v_tx_fixture],
    )
    add(tracks, "uart_tx_fixture", "UART_TX", "2", 1.0, [v_tx_fixture, pt(pads, "TP_TX", "1")])
    add(tracks, "uart_rx_escape", "UART_RX", "1", 1.0, [pt(pads, "U_ESP", "40"), v_rx])
    add(
        tracks, "uart_rx_top", "UART_RX", "1", 1.0,
        [v_rx, (4088.5, 3493.5), v_rx_fixture],
    )
    add(tracks, "uart_rx_fixture", "UART_RX", "2", 1.0, [v_rx_fixture, pt(pads, "TP_RX", "1")])
    add(tracks, "en_escape", "ESP_EN", "1", 1.0, [pt(pads, "U_ESP", "45"), v_en])
    add(tracks, "en_top", "ESP_EN", "1", 1.0, [v_en, (4091.0, 3494.5), (4089.0, 3496.5), v_en_fixture])
    add(tracks, "en_fixture", "ESP_EN", "2", 1.0, [v_en_fixture, pt(pads, "TP_EN", "1")])
    add(tracks, "en_cap_bottom", "ESP_EN", "2", 1.0, [v_en, (4087.0, 3436.074), pt(pads, "C_EN", "1")])
    add(tracks, "en_rc_bottom", "ESP_EN", "2", 1.0, [pt(pads, "C_EN", "1"), pt(pads, "R_EN", "2")])

    # Explicit local GND returns complement the filled top/bottom planes.
    v_gnd_dec = (4037.5, 3438.767)
    v_gnd_en = (4087.0, 3445.0)
    v_gnd_tvs = (4090.0, 3502.5)
    v_gnd_level_1 = (4051.0, 3472.0)
    v_gnd_level_3 = (4048.0, 3487.5)
    v_gnd_pulldown = v_gnd_level_3
    v_gnd_level_dec = (4079.5, 3470.0)
    add(tracks, "gnd_dec", "GND", "2", 1.0, [pt(pads, "C_ESP_DEC", "2"), v_gnd_dec])
    add(tracks, "gnd_en", "GND", "2", 1.0, [pt(pads, "C_EN", "2"), v_gnd_en])
    add(tracks, "gnd_level_1", "GND", "2", 1.0, [pt(pads, "U_LEVEL", "1"), v_gnd_level_1])
    add(tracks, "gnd_level_3", "GND", "2", 1.0, [pt(pads, "U_LEVEL", "3"), (4051.169, 3483.559), v_gnd_level_3])
    add(tracks, "gnd_pulldown", "GND", "2", 1.0, [pt(pads, "R_LEVEL_PD", "2"), v_gnd_pulldown])
    add(tracks, "gnd_level_dec", "GND", "2", 1.0, [pt(pads, "C_LEVEL", "2"), v_gnd_level_dec])
    add(tracks, "gnd_tvs", "GND", "2", 1.2, [pt(pads, "D_VBUS", "2"), v_gnd_tvs])

    areas = {}
    for layer, suffix, order in (("1", "top", "8"), ("2", "bottom", "9")):
        gid = PREFIX + f"gnd_{suffix}_pour"
        areas[gid] = {
            "gId": gid,
            "layerid": layer,
            "net": "GND",
            "name": f"V2 One-Side {suffix.title()} GND Pour",
            "order": order,
            "pathStr": pathstr(POUR),
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
        }

    vias = [
        via(PREFIX + "via_5v_main", "5V", *v5_main),
        via(PREFIX + "via_5v_reg", "5V", *v5_reg),
        via(PREFIX + "via_5v_fixture", "5V", *v5_fixture),
        via(PREFIX + "via_3v3", "3V3", *v3_esp),
        via(PREFIX + "via_led", "LED_DATA_3V3", *v_led),
        via(PREFIX + "via_boot", "BOOT", *v_boot),
        via(PREFIX + "via_boot_fixture", "BOOT", *v_boot_fixture),
        via(PREFIX + "via_tx", "UART_TX", *v_tx),
        via(PREFIX + "via_tx_fixture", "UART_TX", *v_tx_fixture),
        via(PREFIX + "via_rx", "UART_RX", *v_rx),
        via(PREFIX + "via_rx_fixture", "UART_RX", *v_rx_fixture),
        via(PREFIX + "via_en", "ESP_EN", *v_en),
        via(PREFIX + "via_en_fixture", "ESP_EN", *v_en_fixture),
        via(PREFIX + "via_gnd_dec", "GND", *v_gnd_dec),
        via(PREFIX + "via_gnd_en", "GND", *v_gnd_en),
        via(PREFIX + "via_gnd_tvs", "GND", *v_gnd_tvs),
        via(PREFIX + "via_gnd_level_1", "GND", *v_gnd_level_1),
        via(PREFIX + "via_gnd_level_3", "GND", *v_gnd_level_3),

        via(PREFIX + "via_gnd_level_dec", "GND", *v_gnd_level_dec),
    ]
    # Project-standard 0.305-mm drills keep the thermal matrix inside the
    # verified DRC envelope. Bottom-side tenting remains a CAM/order requirement
    # for user reflow and is not inferred from DRC.
    for ix, x in enumerate((4048.937, 4055.433, 4061.929), 1):
        for iy, y in enumerate((3449.0035, 3455.4995, 3461.9955), 1):
            vias.append(via(PREFIX + f"via_gnd_thermal_{ix}_{iy}", "GND", x, y))
    return tracks, areas, vias


def validate_geometry(tracks: dict, areas: dict, vias: list[dict]) -> None:
    for obj in tracks.values():
        assert obj["layerid"] in {"1", "2"}
        for point in obj["pointArr"]:
            assert point_in_poly((float(point["x"]), float(point["y"])), BOARD), (obj["gId"], point)
    for obj in vias:
        assert point_in_poly((float(obj["x"]), float(obj["y"])), BOARD), obj
    assert len(areas) == 2
    assert len([v for v in vias if v["net"] == "GND"]) == 15


def site_matches(obj: dict, plan: dict, tolerance: float = 0.03) -> bool:
    return obj.get("net") == plan["net"] and math.hypot(
        float(obj.get("x", 0)) - plan["x"], float(obj.get("y", 0)) - plan["y"]
    ) <= tolerance


def apply_generated(src: dict, tracks: dict, areas: dict, vias: list[dict]) -> dict:
    order = src.setdefault("itemOrder", [])
    for key in ("TRACK", "COPPERAREA"):
        for gid in list(src.setdefault(key, {})):
            if gid.startswith(OLD_PREFIXES):
                del src[key][gid]
                while gid in order:
                    order.remove(gid)
    for gid, obj in list(src.setdefault("VIA", {}).items()):
        if gid.startswith(OLD_PREFIXES) or any(site_matches(obj, plan) for plan in vias):
            del src["VIA"][gid]
            while gid in order:
                order.remove(gid)
    for gid, obj in tracks.items():
        src["TRACK"][gid] = obj
        order.append(gid)
    for gid, obj in areas.items():
        src["COPPERAREA"][gid] = obj
        order.append(gid)
    for layer in ("1", "2"):
        src["layers"][layer]["config"] = True
        src["layers"][layer]["visible"] = True
    return src


def main() -> None:
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
        for plan in vias:
            if not any(site_matches(obj, plan) for obj in live.get("VIA", {}).values()):
                c.api("createShape", {"shapeType": "VIA", "jsonCache": {
                    "x": plan["x"], "y": plan["y"], "net": plan["net"],
                    "diameter": plan["diameter"], "holeR": plan["holeR"],
                    "layerid": 11, "locked": "0",
                }})
                time.sleep(0.2)
        time.sleep(3)
        refill = builder.source(c)
        for gid in areas:
            refill["COPPERAREA"][gid].pop("polygonArr", None)
        c.api("applySource", {"source": refill, "createNew": False})
        time.sleep(12)
        final = builder.source(c)
        missing_tracks = [gid for gid in tracks if gid not in final.get("TRACK", {})]
        missing_vias = [plan["gId"] for plan in vias if not any(site_matches(obj, plan) for obj in final.get("VIA", {}).values())]
        fills = {gid: len(final.get("COPPERAREA", {}).get(gid, {}).get("polygonArr", [])) for gid in areas}
        if missing_tracks or missing_vias or not all(fills.values()):
            raise RuntimeError({"missing_tracks": missing_tracks, "missing_vias": missing_vias, "fills": fills})
        c.api("doCommand", {"cmd": "file_save"})
        time.sleep(2)
        print(json.dumps({"tracks": len(tracks), "vias": len(vias), "areas": len(areas), "fills": fills}, indent=2))
    finally:
        c.close()


if __name__ == "__main__":
    main()
