#!/usr/bin/env python3
"""Build the ESP32-S3 LiPo/USB-C/addressable-LED development board in EasyEDA Std.

This script talks only to the local EasyEDA renderer through the localhost CDP
bridge installed with easyeda-agent. It is intentionally project-specific.
"""

from __future__ import annotations

import json
import re
import time
import urllib.request
from pathlib import Path

import websocket

TARGET_PROJECT = "ESP32-S3 LiPo LED Dev Board"
STATE_PATH = Path.home() / ".local/share/easyeda-agent-harness/esp32_lipo_led_board_state.json"

PARTS = [
    # alias, UUID, exact library title, display value, canvas x, canvas y
    ("U_ESP", "e4dbf5a145df4956a87241c1189e5a10", "ESP32-S3-WROOM-1-N8", "ESP32-S3-WROOM-1-N8", 900, 180),
    ("U_CHG", "46a447ef12c94516fb8f06cc747e0133", "BQ24074RGTR", "BQ24074 power path", 500, -430),
    ("U_3V3", "ba5595edb5d048308f4b808c5b0fcb6e", "TPS63031DSKR", "TPS63031 3V3 buck-boost", 780, -80),
    ("U_LED", "a749e47206de4e0d95ef2aa6c436375f", "TPS61022RWUR", "TPS61022 5V boost", 790, -420),
    ("U_ESD", "1e3da20a1ce241038083d75a4c41e132", "TPD2EUSB30ADRTR", "TPD2EUSB30A", 285, -500),
    ("U_VBUS_ESD", "c3899403907b4d44acca211d57112631", "TPD1E10B06DYAR", "TPD1E10B06", 280, -650),
    ("U_LEVEL", "24e02e87f6c4452eb820ffa636d83507", "SN74AHCT1G125DCKR", "SN74AHCT1G125", 760, 520),
    ("J_USB", "df8405e2fa0e40a984c435ad4c8d5cf3", "TYPE-C-31-M-12", "USB-C", 90, -520),
    ("J_BAT", "584e69781c5242f391e8b163168a359e", "B3P-VH (LF)(SN)", "BAT+ NTC GND / 5A path", 500, -130),
    ("J_LED", "d6e25a7833a242939ca2953d9eadd31f", "B3B-XH-A-BK(LF)(SN)", "5V DATA GND", 1020, 520),
    ("SW_RESET", "2cbbccf860a84ef9a9976b6bb9db0243", "TS-1088-AR02016", "RESET", 660, 210),
    ("SW_BOOT", "2cbbccf860a84ef9a9976b6bb9db0243", "TS-1088-AR02016", "BOOT", 660, 300),
    ("F_USB", "f1c1bb2a289b4229bdb105f9a78fccbf", "MF-MSMF150-2", "1.5A PTC", 210, -620),
    ("F_LED", "9a19a5ed3bff4694908b63a910462dbf", "MF-MSMF200-2", "2A PTC", 900, 520),
    ("L_3V3", "8e4457ad4f1047f996166010bd548d0d", "LPS3015-152MRC", "1.5uH", 760, 40),
    ("L_LED", "c9112038b1a54f20966fcf34839ab274", "XGL5030-102MEC", "1uH high-current", 790, -600),
    ("J_H1", "94cf0a1beabe454cb91accc8fc518db8", "HMT-PM254-1*10Z-H85", "GPIO HEADER A", 120, 260),
    ("J_H2", "94cf0a1beabe454cb91accc8fc518db8", "HMT-PM254-1*10Z-H85", "GPIO HEADER B", 330, 260),
    ("J_UART", "15cb5a95b04c498f9dd012964e4326eb", "DS1023-1*4S21", "UART", 510, 260),
    # USB-C and native USB
    ("R_CC1", "7619bb22ba3343f38e9ddf8d7f9273a2", "RC0603FR-075K1L", "5.1k", 100, -330),
    ("R_CC2", "7619bb22ba3343f38e9ddf8d7f9273a2", "RC0603FR-075K1L", "5.1k", 180, -330),
    ("R_USB_DM", "1dc7f1cdfa0c49e2b962093bc3da0acd", "RC0603FR-0722RL", "22R", 350, -550),
    ("R_USB_DP", "1dc7f1cdfa0c49e2b962093bc3da0acd", "RC0603FR-0722RL", "22R", 350, -470),
    # BQ24074 configuration and bypassing
    ("R_ISET", "e546bbbffe6c46edbb3ba5d0af054fb5", "RC0603FR-071K78L", "1.78k / 500mA", 400, -250),
    ("R_ILIM", "c002b261b61842d78cc3c38498541485", "RC0603FR-071K18L", "1.18k / 1.3A", 460, -250),
    ("R_ITERM", "671b1284a66745d68c8150c36c349755", "RC0603FR-073K01L", "3.01k / 50mA", 520, -250),
    ("R_TMR", "a1e374228d544ba6ab7148ddf22500cb", "RC0603FR-0746K4L", "46.4k / 6.2h", 580, -250),
    ("R_CHG_PU", "9e0eced329e241a390924a11deca01dd", "RC0603FR-0710KL", "10k", 400, -170),
    ("R_PGOOD_PU", "9e0eced329e241a390924a11deca01dd", "RC0603FR-0710KL", "10k", 460, -170),
    ("R_CE_PD", "1f3ee760342e49189204cc773fbd6664", "RC0603FR-07100KL", "100k", 520, -170),
    ("R_EN1_PD", "1f3ee760342e49189204cc773fbd6664", "RC0603FR-07100KL", "100k / default USB500", 580, -170),
    ("R_EN2_PD", "1f3ee760342e49189204cc773fbd6664", "RC0603FR-07100KL", "100k", 640, -170),
    ("C_CHG_IN", "dd5edc453007461c89d3b27a9f359e83", "GRM188R61A475KE15D", "4.7uF", 380, -650),
    ("C_CHG_IN_HF", "e37ccc37b75c073f3d563908875dcb4a", "GRM188R71H104KA93D", "100nF", 430, -650),
    ("C_CHG_BAT", "5334081c90c14779ba11a1f8fc817a30", "GRM188R60J106ME84J", "10uF", 500, -650),
    ("C_CHG_SYS", "dd5edc453007461c89d3b27a9f359e83", "GRM188R61A475KE15D", "4.7uF", 570, -650),
    # TPS61022 5V LED boost reference circuit
    ("R_LED_FB_TOP", "0789a07686284d5288c840b875c2b548", "RC0603FR-07732KL", "732k", 690, -620),
    ("R_LED_FB_BOTTOM", "1f3ee760342e49189204cc773fbd6664", "RC0603FR-07100KL", "100k", 750, -620),
    ("R_LED_EN_PD", "1f3ee760342e49189204cc773fbd6664", "RC0603FR-07100KL", "100k", 810, -620),
    ("C_LED_FF", "958a25b7e498456cad8fc90b98c52f4c", "GRM1885C1H111JA01D", "110pF", 690, -560),
    ("C_LED_IN", "5334081c90c14779ba11a1f8fc817a30", "GRM188R60J106ME84J", "10uF", 700, -340),
    ("C_LED_OUT1", "f703d4878a194a199c38d96de3cd9ca1", "GRM21BR61A226ME44L", "22uF", 900, -430),
    ("C_LED_OUT2", "f703d4878a194a199c38d96de3cd9ca1", "GRM21BR61A226ME44L", "22uF", 960, -430),
    ("C_LED_OUT3", "f703d4878a194a199c38d96de3cd9ca1", "GRM21BR61A226ME44L", "22uF", 1020, -430),
    ("C_LED_BULK", "3c78e136e6014e0abfb55eb4f890be7b", "EEU-FR1A471B", "470uF 10V", 990, 650),
    # TPS63031 fixed 3V3 buck-boost and ESP support
    ("C_3V3_IN", "5334081c90c14779ba11a1f8fc817a30", "GRM188R60J106ME84J", "10uF", 690, -20),
    ("C_3V3_VINA", "e37ccc37b75c073f3d563908875dcb4a", "GRM188R71H104KA93D", "100nF", 720, 40),
    ("C_3V3_OUT1", "5334081c90c14779ba11a1f8fc817a30", "GRM188R60J106ME84J", "10uF", 850, -20),
    ("C_3V3_OUT2", "5334081c90c14779ba11a1f8fc817a30", "GRM188R60J106ME84J", "10uF", 900, -20),
    ("C_ESP_BULK", "5334081c90c14779ba11a1f8fc817a30", "GRM188R60J106ME84J", "10uF", 800, 80),
    ("C_ESP_DEC", "e37ccc37b75c073f3d563908875dcb4a", "GRM188R71H104KA93D", "100nF", 850, 80),
    ("R_EN", "9e0eced329e241a390924a11deca01dd", "RC0603FR-0710KL", "10k", 590, 170),
    ("C_EN", "f833c644d37a411a8cab614d8a672ae4", "GRM188R61A105KA61D", "1uF", 590, 230),
    ("R_BOOT", "9e0eced329e241a390924a11deca01dd", "RC0603FR-0710KL", "10k", 590, 300),
    # Battery measurement
    ("R_BAT_ADC_TOP", "98dca3e00c38402d8bab6807ac3b7465", "RC0603FR-07200KL", "200k", 500, 20),
    ("R_BAT_ADC_BOTTOM", "1f3ee760342e49189204cc773fbd6664", "RC0603FR-07100KL", "100k", 560, 20),
    ("C_BAT_ADC", "e37ccc37b75c073f3d563908875dcb4a", "GRM188R71H104KA93D", "100nF", 620, 20),
    # 5V logic level conversion
    ("R_LEVEL_PD", "1f3ee760342e49189204cc773fbd6664", "RC0603FR-07100KL", "100k", 760, 650),
    ("R_LED_DATA", "c60a055e3f194c1fa938043a93998eb9", "RC0603FR-0733RL", "33R", 840, 580),
    ("C_LEVEL", "e37ccc37b75c073f3d563908875dcb4a", "GRM188R71H104KA93D", "100nF", 700, 580),
]


def editor_target() -> dict:
    with urllib.request.urlopen("http://127.0.0.1:9223/json/list", timeout=5) as r:
        pages = json.load(r)
    for page in pages:
        if "easyeda.com/editor" in page.get("url", ""):
            return page
    raise RuntimeError("EasyEDA editor target not found on localhost:9223")


class CDP:
    def __init__(self) -> None:
        self.ws = websocket.create_connection(editor_target()["webSocketDebuggerUrl"], timeout=30)
        self.seq = 0

    def evaluate(self, expression: str):
        self.seq += 1
        ident = self.seq
        self.ws.send(json.dumps({
            "id": ident,
            "method": "Runtime.evaluate",
            "params": {"expression": expression, "awaitPromise": True, "returnByValue": True},
        }))
        while True:
            msg = json.loads(self.ws.recv())
            if msg.get("id") != ident:
                continue
            result = msg.get("result", {}).get("result", {})
            if "exceptionDetails" in msg.get("result", {}):
                raise RuntimeError(json.dumps(msg["result"]["exceptionDetails"]))
            return result.get("value")

    def api(self, command: str, args=None):
        return self.evaluate(
            f"window.easyedaAgentApi({json.dumps(command)}, {json.dumps(args)})"
        )

    def close(self) -> None:
        self.ws.close()


def source(cdp: CDP) -> dict:
    return cdp.api("getSource", {"type": "json"})


def set_display_value(cdp: CDP, schlib: dict, text: str) -> None:
    for annotation in schlib.get("annotation", {}).values():
        if annotation.get("mark") == "N":
            cdp.api("updateShape", {
                "shapeType": "annotation",
                "jsonCache": {"gId": annotation["gId"], "string": text},
            })
            return


def delete_ids(cdp: CDP, ids: list[str]) -> None:
    if not ids:
        return
    cdp.evaluate(
        "(()=>{window.easyedaAgentApi('select',{ids:"
        + json.dumps(ids)
        + "});window.easyedaAgentApi('delete');return true})()"
    )
    time.sleep(0.15)


def place_component(
    cdp: CDP,
    alias: str,
    uuid: str,
    title: str,
    display: str,
    x: int,
    y: int,
    used: set[str],
) -> str:
    current = source(cdp)
    # Reuse the original two manually placed core ICs.
    candidates = [
        gid for gid, obj in current.get("schlib", {}).items()
        if obj.get("head", {}).get("uuid") == uuid and gid not in used
    ]
    if candidates:
        gid = candidates[0]
    else:
        before = set(current.get("schlib", {}))
        # UUID insertion is the cleanest 6.5.51 form. Fall back to the
        # legacy shortUrl/title form for libraries that require it.
        cdp.api("createShape", {"shapeType": "schlib", "uuid": uuid, "x": 200, "y": 200})
        created: list[str] = []
        for _ in range(25):
            time.sleep(0.2)
            updated = source(cdp)
            created = [
                gid for gid, obj in updated.get("schlib", {}).items()
                if gid not in before and obj.get("head", {}).get("uuid") == uuid
            ]
            if created:
                break
        if not created:
            cdp.api("createShape", {
                "shapeType": "schlib",
                "shortUrl": uuid,
                "from": "lcsc",
                "title": title,
                "x": 200,
                "y": 200,
            })
            for _ in range(25):
                time.sleep(0.2)
                updated = source(cdp)
                created = [
                    gid for gid, obj in updated.get("schlib", {}).items()
                    if gid not in before and obj.get("head", {}).get("uuid") == uuid
                ]
                if created:
                    break
        if not created:
            raise RuntimeError(f"{alias}: component did not appear after insertion")
        gid = created[-1]
    cdp.api("moveObjsTo", {"objs": [gid], "x": x, "y": y})
    time.sleep(0.04)
    updated_obj = source(cdp)["schlib"][gid]
    set_display_value(cdp, updated_obj, display)
    used.add(gid)
    print(f"{alias:16s} {gid:12s} @ ({x:4d},{y:4d}) {display}")
    return gid


def safe_id(*parts: str) -> str:
    return "agent_" + "_".join(re.sub(r"[^A-Za-z0-9]+", "_", p).strip("_") for p in parts)


def pin_records(schlib: dict) -> list[dict]:
    records = []
    for pin in schlib.get("pin", {}).values():
        records.append({
            "gid": pin["configure"]["gId"],
            "number": str(pin.get("num", {}).get("text", "")),
            "name": str(pin.get("name", {}).get("text", "")),
            "x": float(pin["configure"]["x"]),
            "y": float(pin["configure"]["y"]),
            "rotation": int(float(pin["configure"].get("rotation") or 0)),
        })
    return records


def main() -> None:
    cdp = CDP()
    try:
        title = cdp.evaluate("document.body.innerText.includes(" + json.dumps(TARGET_PROJECT) + ")")
        if not title:
            raise RuntimeError(f"Open the {TARGET_PROJECT!r} project before running this script")

        # This project was created for this builder. Rebuild its generated
        # contents deterministically while retaining only the sheet frame.
        existing = source(cdp)
        generated = [gid for gid in existing.get("schlib", {}) if gid != "frame_lib_1"]
        generated += list(existing.get("wire", {}))
        generated += list(existing.get("netlabel", {}))
        generated += list(existing.get("noconnectflag", {}))
        delete_ids(cdp, generated)

        used: set[str] = set()
        ids: dict[str, str] = {}
        for alias, uuid, title, display, x, y in PARTS:
            ids[alias] = place_component(cdp, alias, uuid, title, display, x, y, used)

        cdp.api("doCommand", {"cmd": "file_save"})
        time.sleep(1)
        final = source(cdp)
        pin_map = {alias: pin_records(final["schlib"][gid]) for alias, gid in ids.items()}
        STATE_PATH.write_text(json.dumps({"ids": ids, "pins": pin_map}, indent=2), encoding="utf-8")
        print(f"Placed {len(ids)} components; state written to {STATE_PATH}")
    finally:
        cdp.close()


if __name__ == "__main__":
    main()
