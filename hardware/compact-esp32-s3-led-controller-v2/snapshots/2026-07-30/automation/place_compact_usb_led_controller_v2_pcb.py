#!/usr/bin/env python3
"""Deterministically place and mechanically normalize the V2 PCB."""
from __future__ import annotations
import importlib.util,json,time
from pathlib import Path
ROOT=Path.home()/'.local/share/easyeda-agent-harness'
spec=importlib.util.spec_from_file_location('builder',ROOT/'build_esp32_lipo_led_board.py');assert spec and spec.loader
builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
STATE=json.loads((ROOT/'compact_usb_led_controller_v2_state.json').read_text())
# EasyEDA source units are 10 mil (0.254 mm). Portrait board is 18 x 32 mm:
# x=4020..4090.866, y=3425..3550.984. The MINI-1 defines the width and
# its antenna projects above y=3425; USB-C is centered at the foot.
POSITIONS={
 'U_ESP':(4055.433,3455.5),'J_USB':(4055.433,3540.659),'J_LED':(4024.5,3516),
 'D_VBUS':(4078,3516),'F_VBUS':(4038,3514),
 'U_3V3':(4048,3492),'U_LEVEL':(4062,3495),
 'R_CC1':(4050,3519),'R_CC2':(4063,3519),
 'R_EN':(4072,3505),'C_EN':(4082,3505),'R_BOOT':(4038,3447),
 'R_LEVEL_PD':(4023.0,3490),'R_DATA':(4050.5,3506),'C_LEVEL':(4082,3496),
 'C_VBUS_IN':(4032.5,3492),'C_3V3_OUT1':(4082,3488.3),
 'C_ESP_DEC':(4035,3438.5),
 # Compact 2 columns x 3 rows at 2.00-mm / 7.874-source-unit pitch.
 'TP_BOOT':(4052,3508),'TP_GND':(4059.874,3508),
 'TP_TX':(4052,3515.874),'TP_EN':(4059.874,3515.874),
 'TP_5V':(4052,3523.748),'TP_RX':(4059.874,3523.748),
}
TPS={'TP_5V','TP_GND','TP_TX','TP_RX','TP_EN','TP_BOOT'}
# EasyEDA records rotate-right as a 90-degree decrement.
ROTATIONS={'R_CC1':270,'R_CC2':270,'D_VBUS':270,'U_3V3':270,'F_VBUS':270,'R_LEVEL_PD':270,'R_DATA':180}

def rotate_to(c,alias,target):
 src=builder.source(c);h=src['FOOTPRINT'][STATE['ids'][alias]]['head'];cur=float(h.get('rotation') or 0)%360
 if abs(cur-target)<.02:return
 c.api('select',{'ids':[STATE['ids'][alias]]});time.sleep(.25)
 for _ in range(int(((cur-target)%360)/90)):
  c.api('doCommand',{'cmd':'rotate_right'});time.sleep(.6)

def set_pad_geometry(p,x,y,pad_mm=2.2,drill_mm=1.2):
 p['x']=x;p['y']=y;p['holeCenter']={'x':x,'y':y}
 p['width']=pad_mm/.254;p['height']=pad_mm/.254;p['holeR']=(drill_mm/2)/.254
 if p.get('shape')=='RECT':
  r=(pad_mm/2)/.254;p['pointArr']=[{'x':x-r,'y':y-r},{'x':x+r,'y':y-r},{'x':x+r,'y':y+r},{'x':x-r,'y':y+r}]
 else:p['pointArr']=[]

def main():
 if set(POSITIONS)!=set(STATE['ids']):
  raise RuntimeError(f"placement mismatch missing={set(STATE['ids'])-set(POSITIONS)} extra={set(POSITIONS)-set(STATE['ids'])}")
 c=builder.CDP()
 try:
  src=builder.source(c)
  if src.get('head',{}).get('docType')!='3' or len(src.get('FOOTPRINT',{}))!=24:
   raise RuntimeError('activate the canonical 24-footprint V2 PCB before placement')
  outlines=[o for o in src.get('TRACK',{}).values() if str(o.get('layerid'))=='10']
  old_rectangle=[{'x':4020,'y':3425},{'x':4161.732,'y':3425},{'x':4161.732,'y':3523.425},{'x':4020,'y':3523.425},{'x':4020,'y':3425}]
  old_chamfered=[{'x':4025.906,'y':3425},{'x':4161.732,'y':3425},{'x':4161.732,'y':3523.425},{'x':4020,'y':3523.425},{'x':4020,'y':3430.906},{'x':4025.906,'y':3425}]
  rectangle=[{'x':4020,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3550.984},{'x':4020,'y':3550.984},{'x':4020,'y':3425}]
  chamfered=[{'x':4025.906,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3550.984},{'x':4020,'y':3550.984},{'x':4020,'y':3430.906},{'x':4025.906,'y':3425}]
  if len(outlines)!=1 or outlines[0].get('pointArr') not in (old_rectangle,old_chamfered,rectangle,chamfered):raise RuntimeError('V2 outline does not match the old or portrait canonical extents')
  for alias,(x,y) in POSITIONS.items():
   c.api('moveObjsTo',{'objs':[STATE['ids'][alias]],'x':x,'y':y});time.sleep(.14)
  time.sleep(2)
  for alias,target in ROTATIONS.items():rotate_to(c,alias,target)
  final=builder.source(c)
  # A 1.50-mm top-left chamfer provides physical fixture anti-reversal keying
  # without adding a hole, fastener, or recurring BOM item.
  [outline]=[o for o in final.get('TRACK',{}).values() if str(o.get('layerid'))=='10']
  outline['pointArr']=chamfered
  # Pogo pads: underside bare copper, exact 1.20-mm squares, no BOM/PnP.
  for alias in TPS:
   fp=final['FOOTPRINT'][STATE['ids'][alias]];fp['head']['layerid']='2';fp['head']['add_into_bom']='no'
   pads=fp.get('PAD',{})
   for p in pads.values():
    p['width']=1.2/.254;p['height']=1.2/.254
    r=.6/.254;p['pointArr']=[{'x':p['x']-r,'y':p['y']-r},{'x':p['x']+r,'y':p['y']-r},{'x':p['x']+r,'y':p['y']+r},{'x':p['x']-r,'y':p['y']+r}]
   for typ,objs in fp.items():
    if not isinstance(objs,dict):continue
    for obj in objs.values():
     if not isinstance(obj,dict) or 'layerid' not in obj:continue
     # Normalize absolutely; swapping 1<->2 on every run made the fixture
     # copper alternate between the top and bottom layers.
     if typ=='PAD':obj['layerid']='2'
     elif str(obj['layerid']) in {'3','4'}:obj['layerid']='4'
  # Replace the provisional JST-VH identity/geometry with a compact manual
  # three-wire landing: 3.00-mm pitch, 2.20-mm pads, 1.20-mm finished holes.
  # The printed enclosure, not PCB holes, supplies the cable clamp.
  j=final['FOOTPRINT'][STATE['ids']['J_LED']];jx,jy=POSITIONS['J_LED']
  j['head']['c_para']='package`CUSTOM_3WIRE_PIGTAIL_3MM`Supplier`Manual`Manufacturer`Generic`Manufacturer Part`JST-SM female pigtail, 20AWG, pre-tinned`spicePre`J`spiceSymbolName`3-wire soldered pigtail`'
  j['head']['add_into_bom']='yes';j['head']['uuid']='';j['head']['uuid_3d']=''
  pitch=3.0/.254
  pads={str(p.get('number')):p for p in j.get('PAD',{}).values()}
  set_pad_geometry(pads['1'],jx,jy-pitch);set_pad_geometry(pads['2'],jx,jy);set_pad_geometry(pads['3'],jx,jy+pitch)
  for typ in list(j):
   if typ not in {'PAD','head','gId','itemOrder'} and isinstance(j[typ],dict):j[typ]={}
  c.api('applySource',{'source':final,'createNew':False});time.sleep(8)
  check=builder.source(c);bad=[]
  for alias,(x,y) in POSITIONS.items():
   h=check['FOOTPRINT'][STATE['ids'][alias]]['head']
   if abs(float(h['x'])-x)>.02 or abs(float(h['y'])-y)>.02:bad.append(alias)
  if bad:raise RuntimeError(f'post-apply placement mismatch: {bad}')
  jp=check['FOOTPRINT'][STATE['ids']['J_LED']];actual=sorted((str(p['number']),round(float(p['x']),4),round(float(p['y']),4),round(float(p['width'])*.254,3),round(float(p['holeR'])*2*.254,3)) for p in jp['PAD'].values())
  c.api('doCommand',{'cmd':'file_save'});time.sleep(3)
  print(json.dumps({'placed':len(POSITIONS),'bottom_pogo_pads':len(TPS),'board_mm':[18,32],'pigtail_pads':actual,'antenna_overhang_mm':5.05},indent=2))
 finally:c.close()
if __name__=='__main__':main()
