#!/usr/bin/env python3
"""Reset the V2 PCB to a clean two-layer placement/routing baseline."""
from __future__ import annotations
import importlib.util,json,time
from pathlib import Path
ROOT=Path.home()/'.local/share/easyeda-agent-harness'
spec=importlib.util.spec_from_file_location('builder',ROOT/'build_esp32_lipo_led_board.py');assert spec and spec.loader
builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
STATE=json.loads((ROOT/'compact_usb_led_controller_v2_state.json').read_text())

def main():
 c=builder.CDP()
 try:
  s=builder.source(c)
  if s.get('head',{}).get('docType')!='3':raise RuntimeError('active document is not a PCB')
  keep=set(STATE['ids'].values())
  extras=set(s.get('FOOTPRINT',{}))-keep
  for gid in extras:s['FOOTPRINT'].pop(gid,None)
  outline={g:o for g,o in s.get('TRACK',{}).items() if str(o.get('layerid'))=='10'}
  if len(outline)!=1:raise RuntimeError(f'expected one board outline, got {len(outline)}')
  removed=set(s.get('TRACK',{}))-set(outline)
  removed|=set(s.get('VIA',{}))|set(s.get('COPPERAREA',{}))
  s['TRACK']=outline;s['VIA']={};s['COPPERAREA']={}
  s['itemOrder']=[g for g in s.get('itemOrder',[]) if g not in removed and g not in extras]
  for lid in ('21','22'):
   if lid in s.get('layers',{}):s['layers'][lid]['config']=False;s['layers'][lid]['visible']=False
  c.api('applySource',{'source':s,'createNew':False});time.sleep(8)
  f=builder.source(c)
  if len(f.get('FOOTPRINT',{}))!=len(keep) or f.get('VIA') or f.get('COPPERAREA'):raise RuntimeError('two-layer reset did not converge')
  if any(str(o.get('layerid'))!='10' for o in f.get('TRACK',{}).values()):raise RuntimeError('stale routed track survived reset')
  c.api('doCommand',{'cmd':'file_save'});time.sleep(3)
  print(json.dumps({'footprints':len(f['FOOTPRINT']),'removed_extra_footprints':sorted(extras),'tracks':len(f['TRACK']),'vias':len(f.get('VIA',{})),'copper_areas':len(f.get('COPPERAREA',{})),'layers':{k:f['layers'][k]['config'] for k in ('1','2','21','22')}},indent=2))
 finally:c.close()
if __name__=='__main__':main()
