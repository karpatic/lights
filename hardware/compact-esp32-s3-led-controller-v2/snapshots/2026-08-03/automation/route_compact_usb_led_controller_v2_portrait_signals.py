#!/usr/bin/env python3
"""Deterministic signal routing for the V2 18x27-mm connector-first PCB."""
from __future__ import annotations

import argparse
import importlib.util
import json
import math
import time
from pathlib import Path

ROOT = Path.home() / ".local/share/easyeda-agent-harness"
SNAPSHOT = Path("/tmp/easyeda-v2-power-clean-live-pcb.json")
STATE = ROOT / "compact_usb_led_controller_v2_state.json"
PREFIX = "agent_v2_portrait_sig_"
RETIRED_VIA_SITES = [
    {"net": "LED_DATA_3V3", "x": 4052.087, "y": 3485.8},
    {"net": "LED_DATA_3V3", "x": 4062.0, "y": 3504.8},
    {"net": "ESP_EN", "x": 4086.0, "y": 3432.0745},
    {"net": "ESP_EN", "x": 4075.0, "y": 3502.0},
    {"net": "ESP_EN", "x": 4064.0, "y": 3518.0},
    {"net": "ESP_EN", "x": 4076.5, "y": 3505.0},
    {"net": "LED_DATA_3V3", "x": 4066.0, "y": 3509.0},
    {"net": "ESP_EN", "x": 4080.0, "y": 3510.0},
    {"net": "LED_DATA_3V3", "x": 4062.0, "y": 3507.0},
    {"net": "ESP_EN", "x": 4068.0, "y": 3515.0},
    {"net": "ESP_EN", "x": 4086.0, "y": 3432.0745},
    {"net": "LED_DATA_3V3", "x": 4024.0, "y": 3497.0},
    {"net": "LED_DATA_3V3", "x": 4033.0, "y": 3470.0},
    {"net": "LED_DATA_3V3", "x": 4023.0, "y": 3483.5},
    {"net": "LED_DATA_3V3", "x": 4045.0, "y": 3470.0},
]
BOARD = [(4025.906, 3425.0), (4090.866, 3425.0), (4090.866, 3531.299), (4020.0, 3531.299), (4020.0, 3430.906)]


def load_json(path: Path) -> dict:
    with path.open() as f:
        return json.load(f)


def pad_index(src: dict, state: dict) -> dict[tuple[str, str], dict]:
    pads = {}
    for alias, gid in state["ids"].items():
        fp = src.get("FOOTPRINT", {}).get(gid)
        assert fp, f"missing footprint {alias}:{gid}"
        for pad in fp.get("PAD", {}).values():
            pads[(alias, str(pad.get("number")))] = pad
    return pads


def point(pads: dict[tuple[str, str], dict], alias: str, number: str) -> tuple[float, float]:
    pad = pads[(alias, number)]
    return float(pad["x"]), float(pad["y"])


def track(gid: str, net: str, layer: str, points: list[tuple[float, float]], width: float = 1.0) -> dict:
    return {
        "gId": gid,
        "layerid": layer,
        "net": net,
        "pointArr": [{"x": round(x, 4), "y": round(y, 4)} for x, y in points],
        "strokeWidth": width,
        "locked": 0,
    }


def via(gid: str, net: str, x: float, y: float) -> dict:
    return {"gId": gid, "net": net, "x": x, "y": y, "diameter": 2.4, "holeR": 0.6}


def build_objects(src: dict, state: dict) -> tuple[dict[str, dict], list[dict]]:
    pads = pad_index(src, state)
    expected = {
        ("R_EN", "2"): ("ESP_EN", 4072.966, 3490.0, "1"),
        ("C_EN", "1"): ("ESP_EN", 4079.244, 3490.0, "1"),
        ("R_DATA", "2"): ("LED_DATA", 4044.034, 3514.0, "1"),
        ("J_LED", "2"): ("LED_DATA", 4041.6535, 3521.4567, "11"),
        ("U_ESP", "4"): ("BOOT", 4027.972, 3442.1135, "1"),
        ("R_BOOT", "2"): ("BOOT", 4035.034, 3447.0, "2"),
        ("TP_BOOT", "1"): ("BOOT", 4043.622, 3497.5, "2"),
        ("U_ESP", "39"): ("UART_TX", 4082.894, 3452.1535, "1"),
        ("TP_TX", "1"): ("UART_TX", 4051.4961, 3509.311, "2"),
        ("U_ESP", "40"): ("UART_RX", 4082.894, 3448.8065, "1"),
        ("TP_RX", "1"): ("UART_RX", 4059.3701, 3509.311, "2"),
        ("U_LEVEL", "4"): ("LED_BUF_OUT", 4047.559, 3500.669, "1"),
        ("R_DATA", "1"): ("LED_BUF_OUT", 4049.966, 3514.0, "1"),
        ("U_ESP", "45"): ("ESP_EN", 4082.894, 3432.0745, "1"),
        ("TP_EN", "1"): ("ESP_EN", 4059.3701, 3497.5, "2"),
        ("U_ESP", "22"): ("LED_DATA_3V3", 4052.087, 3483.1575, "1"),
        ("U_LEVEL", "2"): ("LED_DATA_3V3", 4045.0, 3509.331, "1"),
        ("R_LEVEL_PD", "1"): ("LED_DATA_3V3", 4023.0, 3487.034, "1"),
    }
    for key, wanted in expected.items():
        pad = pads[key]
        got = (pad.get("net", ""), float(pad["x"]), float(pad["y"]), str(pad["layerid"]))
        exp = (wanted[0], float(wanted[1]), float(wanted[2]), wanted[3])
        assert (
            got[0] == exp[0]
            and abs(got[1] - exp[1]) <= 0.001
            and abs(got[2] - exp[2]) <= 0.001
            and got[3] == exp[3]
        ), f"{key}: expected {exp}, got {got}"

    ren = point(pads, "R_EN", "2")
    cen = point(pads, "C_EN", "1")
    rdata = point(pads, "R_DATA", "2")
    jled = point(pads, "J_LED", "2")
    esp_boot = point(pads, "U_ESP", "4")
    rboot = point(pads, "R_BOOT", "2")
    tpboot = point(pads, "TP_BOOT", "1")
    esp_tx = point(pads, "U_ESP", "39")
    tp_tx = point(pads, "TP_TX", "1")
    esp_rx = point(pads, "U_ESP", "40")
    tp_rx = point(pads, "TP_RX", "1")
    level_buf = point(pads, "U_LEVEL", "4")
    rdata_buf = point(pads, "R_DATA", "1")
    esp_en = point(pads, "U_ESP", "45")
    tp_en = point(pads, "TP_EN", "1")
    esp_led_3v3 = point(pads, "U_ESP", "22")
    level_led_3v3 = point(pads, "U_LEVEL", "2")
    r_level_pd = point(pads, "R_LEVEL_PD", "1")

    boot_via = (4031.5, 3442.1135)
    tx_via = (4086.0, 3452.1535)
    rx_via = (4086.0, 3448.8065)
    en_mcu_via = (4074.0, 3432.0745)
    en_rc_via = (4074.0, 3493.0)
    led_3v3_level_via = (4047.5, 3505.0)
    led_3v3_esp_via = (4052.087, 3478.5)
    led_3v3_pull_via = (4023.0, 3483.5)

    tracks = {
        PREFIX + "en_rc_top": track(PREFIX + "en_rc_top", "ESP_EN", "1", [ren, cen]),
        PREFIX + "led_data_output_top": track(
            PREFIX + "led_data_output_top", "LED_DATA", "1", [rdata, (4041.6535, 3514.0), jled]
        ),
        PREFIX + "boot_mcu_escape_top": track(PREFIX + "boot_mcu_escape_top", "BOOT", "1", [esp_boot, boot_via]),
        PREFIX + "boot_pullup_bottom": track(PREFIX + "boot_pullup_bottom", "BOOT", "2", [boot_via, rboot]),
        PREFIX + "boot_test_bottom": track(
            PREFIX + "boot_test_bottom",
            "BOOT",
            "2",
            [rboot, (4033.0, 3447.0), (4033.0, 3490.0), (4043.622, 3490.0), tpboot],
        ),
        PREFIX + "uart_tx_escape_top": track(PREFIX + "uart_tx_escape_top", "UART_TX", "1", [esp_tx, tx_via]),
        PREFIX + "uart_tx_transport_bottom": track(
            PREFIX + "uart_tx_transport_bottom",
            "UART_TX",
            "2",
            [tx_via, (4080.0, 3452.1535), (4080.0, 3504.0), (4055.0, 3504.0),
             (4055.0, 3513.0), (4051.4961, 3513.0), tp_tx],
        ),
        PREFIX + "uart_rx_escape_top": track(PREFIX + "uart_rx_escape_top", "UART_RX", "1", [esp_rx, rx_via]),
        PREFIX + "uart_rx_transport_bottom": track(
            PREFIX + "uart_rx_transport_bottom",
            "UART_RX",
            "2",
            [rx_via, (4089.0, 3448.8065), (4089.0, 3509.311), tp_rx],
        ),
        PREFIX + "led_buf_to_resistor_top": track(
            PREFIX + "led_buf_to_resistor_top", "LED_BUF_OUT", "1",
            [level_buf, (4061.0, 3499.0), (4061.0, 3512.0), (4049.966, 3512.0), rdata_buf],
        ),
        PREFIX + "en_mcu_escape_top": track(PREFIX + "en_mcu_escape_top", "ESP_EN", "1", [esp_en, en_mcu_via]),
        PREFIX + "en_transport_bottom": track(
            PREFIX + "en_transport_bottom", "ESP_EN", "2",
            [en_mcu_via, (4074.0, 3484.0), en_rc_via],
        ),
        PREFIX + "en_rc_tie_top": track(
            PREFIX + "en_rc_tie_top", "ESP_EN", "1", [en_rc_via, (4074.0, 3490.0), ren]
        ),
        PREFIX + "en_tp_bottom": track(
            PREFIX + "en_tp_bottom", "ESP_EN", "2", [en_rc_via, (4064.0, 3493.0), tp_en],
        ),
        PREFIX + "led_3v3_level_escape_top": track(
            PREFIX + "led_3v3_level_escape_top",
            "LED_DATA_3V3",
            "1",
            [level_led_3v3, (4045.0, 3505.0), led_3v3_level_via],
        ),
        PREFIX + "led_3v3_transport_bottom": track(
            PREFIX + "led_3v3_transport_bottom",
            "LED_DATA_3V3",
            "2",
            [led_3v3_level_via, (4047.5, 3502.0), (4047.5, 3480.0), led_3v3_esp_via],
        ),
        PREFIX + "led_3v3_esp_escape_top": track(
            PREFIX + "led_3v3_esp_escape_top", "LED_DATA_3V3", "1", [led_3v3_esp_via, esp_led_3v3]
        ),
        PREFIX + "led_3v3_pull_transport_bottom": track(
            PREFIX + "led_3v3_pull_transport_bottom",
            "LED_DATA_3V3",
            "2",
            [(4047.5, 3502.0), (4023.0, 3502.0), led_3v3_pull_via],
        ),
        PREFIX + "led_3v3_pull_down_top": track(
            PREFIX + "led_3v3_pull_down_top", "LED_DATA_3V3", "1", [led_3v3_pull_via, r_level_pd],
        ),
    }
    vias = [
        via(PREFIX + "via_boot_mcu", "BOOT", *boot_via),
        via(PREFIX + "via_uart_tx", "UART_TX", *tx_via),
        via(PREFIX + "via_uart_rx", "UART_RX", *rx_via),

        via(PREFIX + "via_en_mcu", "ESP_EN", *en_mcu_via),
        via(PREFIX + "via_en_rc", "ESP_EN", *en_rc_via),
        via(PREFIX + "via_led_3v3_level", "LED_DATA_3V3", *led_3v3_level_via),
        via(PREFIX + "via_led_3v3_esp", "LED_DATA_3V3", *led_3v3_esp_via),
        via(PREFIX + "via_led_3v3_pull", "LED_DATA_3V3", *led_3v3_pull_via),
    ]
    return tracks, vias


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


def validate_geometry(tracks: dict[str, dict], vias: list[dict]) -> None:
    for obj in tracks.values():
        assert obj["layerid"] in {"1", "2"}
        assert float(obj["strokeWidth"]) >= 1.0
        for p in obj["pointArr"]:
            assert point_in_poly((float(p["x"]), float(p["y"])), BOARD), (obj["gId"], p)
    for obj in vias:
        assert point_in_poly((float(obj["x"]), float(obj["y"])), BOARD), obj
        assert float(obj["diameter"]) >= 2.4 and float(obj["holeR"]) >= 0.6


def site_matches(obj: dict, plan: dict, tolerance: float = 0.03) -> bool:
    return obj.get("net") == plan["net"] and math.hypot(float(obj.get("x", 0)) - plan["x"], float(obj.get("y", 0)) - plan["y"]) <= tolerance


def apply_generated(src: dict, tracks: dict[str, dict], vias: list[dict]) -> dict:
    order = src.setdefault("itemOrder", [])
    for gid in list(src.setdefault("TRACK", {})):
        if gid.startswith(PREFIX):
            del src["TRACK"][gid]
            while gid in order:
                order.remove(gid)
    for gid, obj in list(src.setdefault("VIA", {}).items()):
        if gid.startswith(PREFIX) or any(site_matches(obj, plan) for plan in [*vias, *RETIRED_VIA_SITES]):
            del src["VIA"][gid]
            while gid in order:
                order.remove(gid)
    for gid, obj in tracks.items():
        src["TRACK"][gid] = obj
        order.append(gid)
    return src


def static_validate() -> None:
    src = load_json(SNAPSHOT)
    tracks, vias = build_objects(src, load_json(STATE))
    validate_geometry(tracks, vias)
    staged = apply_generated(json.loads(json.dumps(src)), tracks, vias)
    assert all(gid in staged["TRACK"] for gid in tracks)
    print(json.dumps({"signal_tracks": len(tracks), "planned_native_vias": len(vias)}, indent=2))


def normal_apply() -> None:
    spec = importlib.util.spec_from_file_location("builder", ROOT / "build_esp32_lipo_led_board.py")
    assert spec and spec.loader
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    c = builder.CDP()
    try:
        src = builder.source(c)
        tracks, vias = build_objects(src, load_json(STATE))
        validate_geometry(tracks, vias)
        c.api("applySource", {"source": apply_generated(src, tracks, vias), "createNew": False})
        time.sleep(10)
        live = builder.source(c)
        for plan in vias:
            if not any(site_matches(obj, plan) for obj in live.get("VIA", {}).values()):
                c.api("createShape", {"shapeType": "VIA", "jsonCache": {"x": plan["x"], "y": plan["y"], "net": plan["net"], "diameter": plan["diameter"], "holeR": plan["holeR"], "layerid": 11, "locked": "0"}})
                time.sleep(0.3)
        time.sleep(3)
        refill = builder.source(c)
        gnd_area_ids = {
            "agent_v2_portrait_gnd_bottom_pour",
            "agent_v2_portrait_gnd_top_lower_pour",
        }
        for area in refill.get("COPPERAREA", {}).values():
            if area.get("gId") in gnd_area_ids:
                area.pop("polygonArr", None)
        c.api("applySource", {"source": refill, "createNew": False})
        time.sleep(10)
        final = builder.source(c)
        missing = [gid for gid in tracks if gid not in final.get("TRACK", {})]
        missing_vias = [plan["gId"] for plan in vias if not any(site_matches(obj, plan) for obj in final.get("VIA", {}).values())]
        fill_counts = {
            gid: len(final.get("COPPERAREA", {}).get(gid, {}).get("polygonArr", []))
            for gid in gnd_area_ids
        }
        if missing or missing_vias or not all(fill_counts.values()):
            raise RuntimeError({"missing": missing, "missing_vias": missing_vias, "gnd_fill_counts": fill_counts})
        c.api("doCommand", {"cmd": "file_save"})
        time.sleep(2)
        print(json.dumps({"signal_tracks": len(tracks), "native_vias": len(vias), "gnd_fill_counts": fill_counts}, indent=2))
    finally:
        c.close()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--validate-static", action="store_true")
    args = parser.parse_args()
    static_validate() if args.validate_static else normal_apply()


if __name__ == "__main__":
    main()
