#!/usr/bin/env python3
"""Fail-fast quarantine for the superseded V2 two-layer route geometry."""
from __future__ import annotations

MESSAGE=(
 "STALE GEOMETRY: route_compact_usb_led_controller_v2_two_layer.py was "
 "written for the superseded USB-C/CC-resistor V2 geometry and must not be "
 "run against the 18x32-mm portrait board. Rebuild/place the no-USB "
 "5V_IN/GND pigtail version first, then write a new route from that PCB "
 "source."
)

def main():
 raise RuntimeError(MESSAGE)

if __name__=='__main__':main()
