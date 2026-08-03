#!/usr/bin/env python3
"""Deterministically place and mechanically normalize the V2 PCB."""
from __future__ import annotations
import importlib.util,json,time
from pathlib import Path
ROOT=Path.home()/'.local/share/easyeda-agent-harness'
spec=importlib.util.spec_from_file_location('builder',ROOT/'build_esp32_lipo_led_board.py');assert spec and spec.loader
builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
STATE=json.loads((ROOT/'compact_usb_led_controller_v2_state.json').read_text())
# EasyEDA source units are 10 mil (0.254 mm). Connector-first candidate is
# 18 x 27 mm: x=4020..4090.866, y=3425..3531.299. The MINI-1 and RF geometry
# stay fixed while both cable groups move to one unobstructed bottom row.
POSITIONS={
 'U_ESP':(4055.433,3455.5),'J_LED':(4041.6535,3521.4567),
 'J_5V_IN':(4075.1181,3521.4567),
 'D_VBUS':(4086.5,3509.0),'F_VBUS':(4073.7266,3502.0),
 'U_3V3':(4048,3492),'U_LEVEL':(4045,3505),
 'R_EN':(4070,3490),'C_EN':(4082,3490),'R_BOOT':(4038,3447),
 'R_LEVEL_PD':(4023.0,3490),'R_DATA':(4047,3514),'C_LEVEL':(4032,3502),
 'C_VBUS_IN':(4032.5,3492),'C_3V3_OUT1':(4057,3491),
 'C_ESP_DEC':(4035,3438.5),
 # Height-efficient 3 x 2 field: 2.00-mm column pitch, 3.00-mm row pitch.
 'TP_BOOT':(4043.6220,3497.5000),'TP_GND':(4051.4961,3497.5000),'TP_EN':(4059.3701,3497.5000),
 'TP_5V':(4043.6220,3509.3110),'TP_TX':(4051.4961,3509.3110),'TP_RX':(4059.3701,3509.3110),
}
TPS={'TP_5V','TP_GND','TP_TX','TP_RX','TP_EN','TP_BOOT'}
BOTTOM_PARTS={'C_ESP_DEC','R_BOOT'}
# EasyEDA records rotate-right as a 90-degree decrement.
ROTATIONS={'D_VBUS':270,'U_3V3':270,'F_VBUS':180,'R_BOOT':180,'R_LEVEL_PD':270,'R_DATA':180,'C_VBUS_IN':180}

def rotate_to(c,alias,target):
 src=builder.source(c);h=src['FOOTPRINT'][STATE['ids'][alias]]['head'];cur=float(h.get('rotation') or 0)%360
 if abs(cur-target)<.02:return
 c.api('select',{'ids':[STATE['ids'][alias]]});time.sleep(.25)
 for _ in range(int(((cur-target)%360)/90)):
  c.api('doCommand',{'cmd':'rotate_right'});time.sleep(.6)

def set_pad_geometry(p,x,y,pad_mm=2.0,drill_mm=1.2):
 p['x']=x;p['y']=y;p['holeCenter']={'x':x,'y':y}
 p['width']=pad_mm/.254;p['height']=pad_mm/.254;p['holeR']=(drill_mm/2)/.254
 if p.get('shape')=='RECT':
  r=(pad_mm/2)/.254;p['pointArr']=[{'x':x-r,'y':y-r},{'x':x+r,'y':y-r},{'x':x+r,'y':y+r},{'x':x-r,'y':y+r}]
 else:p['pointArr']=[]

def normalize_input_landing(fp,x,y):
 fp['head']['c_para']='package`CUSTOM_2WIRE_5V_INPUT_PIGTAIL_3MM`Supplier`Manual`Manufacturer`Generic`Manufacturer Part`regulated 5V two-wire pigtail, pre-tinned`spicePre`J`spiceSymbolName`2-wire 5V input pigtail landing`'
 fp['head']['layerid']='1';fp['head']['add_into_bom']='yes'
 fp['head']['uuid']='';fp['head']['uuid_3d']=''
 pads={str(p.get('number')):p for p in fp.get('PAD',{}).values()}
 if set(pads)!={'1','2','3'}:raise RuntimeError('input pigtail carrier must provide three source pads')
 pitch=3.0/.254
 for n,px,net,shape in [('1',x+pitch/2,'5V_IN','RECT'),('2',x-pitch/2,'GND','ELLIPSE')]:
  p=pads[n];p['net']=net;p['shape']=shape;p['layerid']='11';set_pad_geometry(p,px,y)
 # Retain the source library's third logical pin as a concentric, drill-free
 # GND SMD land under pad 2. This keeps EasyEDA's offline serializer from
 # dropping the custom LIB while the fabrication geometry remains two-hole.
 p=pads['3'];px=x-pitch/2
 p.update({'net':'GND','shape':'ELLIPSE','layerid':'1','x':px,'y':y,'width':.8/.254,'height':.8/.254,'holeR':0,'holeCenter':{'x':px,'y':y},'pointArr':[]})
 for typ in list(fp):
  if typ not in {'PAD','head','gId','itemOrder'} and isinstance(fp[typ],dict):fp[typ]={}

def normalize_bottom(fp):
 fp['head']['layerid']='2'
 layer_map={'1':'2','3':'4','5':'6','7':'8'}
 for typ,objs in fp.items():
  if not isinstance(objs,dict):continue
  for obj in objs.values():
   if isinstance(obj,dict) and str(obj.get('layerid')) in layer_map:
    obj['layerid']=layer_map[str(obj['layerid'])]

def main():
 if set(POSITIONS)!=set(STATE['ids']):
  raise RuntimeError(f"placement mismatch missing={set(STATE['ids'])-set(POSITIONS)} extra={set(POSITIONS)-set(STATE['ids'])}")
 c=builder.CDP()
 try:
  src=builder.source(c)
  expected=len(POSITIONS)
  if src.get('head',{}).get('docType')!='3' or len(src.get('FOOTPRINT',{}))!=expected:
   raise RuntimeError(f'activate the canonical {expected}-footprint no-USB V2 PCB before placement')
  outlines=[o for o in src.get('TRACK',{}).values() if str(o.get('layerid'))=='10']
  old_rectangle=[{'x':4020,'y':3425},{'x':4161.732,'y':3425},{'x':4161.732,'y':3523.425},{'x':4020,'y':3523.425},{'x':4020,'y':3425}]
  old_chamfered=[{'x':4025.906,'y':3425},{'x':4161.732,'y':3425},{'x':4161.732,'y':3523.425},{'x':4020,'y':3523.425},{'x':4020,'y':3430.906},{'x':4025.906,'y':3425}]
  portrait_rectangle=[{'x':4020,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3550.984},{'x':4020,'y':3550.984},{'x':4020,'y':3425}]
  portrait_chamfered=[{'x':4025.906,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3550.984},{'x':4020,'y':3550.984},{'x':4020,'y':3430.906},{'x':4025.906,'y':3425}]
  compact_rectangle=[{'x':4020,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3536.0},{'x':4020,'y':3536.0},{'x':4020,'y':3425}]
  compact_chamfered=[{'x':4025.906,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3536.0},{'x':4020,'y':3536.0},{'x':4020,'y':3430.906},{'x':4025.906,'y':3425}]
  release_rectangle=[{'x':4020,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3539.1732},{'x':4020,'y':3539.1732},{'x':4020,'y':3425}]
  release_chamfered=[{'x':4025.906,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3539.1732},{'x':4020,'y':3539.1732},{'x':4020,'y':3430.906},{'x':4025.906,'y':3425}]
  candidate_rectangle=[{'x':4020,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3531.299},{'x':4020,'y':3531.299},{'x':4020,'y':3425}]
  candidate_chamfered=[{'x':4025.906,'y':3425},{'x':4090.866,'y':3425},{'x':4090.866,'y':3531.299},{'x':4020,'y':3531.299},{'x':4020,'y':3430.906},{'x':4025.906,'y':3425}]
  if len(outlines)!=1 or outlines[0].get('pointArr') not in (old_rectangle,old_chamfered,portrait_rectangle,portrait_chamfered,compact_rectangle,compact_chamfered,release_rectangle,release_chamfered,candidate_rectangle,candidate_chamfered):raise RuntimeError('V2 outline does not match a recognized canonical extent')
  for alias,(x,y) in POSITIONS.items():
   c.api('moveObjsTo',{'objs':[STATE['ids'][alias]],'x':x,'y':y});time.sleep(.14)
  time.sleep(2)
  for alias,target in ROTATIONS.items():rotate_to(c,alias,target)
  final=builder.source(c)
  # A 1.50-mm top-left chamfer provides physical fixture anti-reversal keying
  # without adding a hole, fastener, or recurring BOM item.
  [outline]=[o for o in final.get('TRACK',{}).values() if str(o.get('layerid'))=='10']
  outline['pointArr']=candidate_chamfered
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
  for alias in BOTTOM_PARTS:normalize_bottom(final['FOOTPRINT'][STATE['ids'][alias]])
  # Regulated 5 V input: one persistent two-pad pigtail landing at 3.00-mm
  # pitch, with 2.00-mm pads and 1.20-mm finished holes.
  normalize_input_landing(final['FOOTPRINT'][STATE['ids']['J_5V_IN']],*POSITIONS['J_5V_IN'])
  # Replace the provisional JST-VH identity/geometry with a compact manual
  # three-wire landing: 3.00-mm pitch, 2.00-mm pads, 1.20-mm finished holes.
  # The printed enclosure, not PCB holes, supplies the cable clamp.
  j=final['FOOTPRINT'][STATE['ids']['J_LED']];jx,jy=POSITIONS['J_LED']
  j['head']['c_para']='package`CUSTOM_3WIRE_PIGTAIL_3MM`Supplier`Manual`Manufacturer`Generic`Manufacturer Part`JST-SM female pigtail, 20AWG, pre-tinned`spicePre`J`spiceSymbolName`3-wire soldered pigtail`'
  j['head']['add_into_bom']='yes';j['head']['uuid']='';j['head']['uuid_3d']=''
  pitch=3.0/.254
  pads={str(p.get('number')):p for p in j.get('PAD',{}).values()}
  # Horizontal LED row is GND, DATA, protected 5 V from left to right.
  set_pad_geometry(pads['1'],jx+pitch,jy);set_pad_geometry(pads['2'],jx,jy);set_pad_geometry(pads['3'],jx-pitch,jy)
  for typ in list(j):
   if typ not in {'PAD','head','gId','itemOrder'} and isinstance(j[typ],dict):j[typ]={}
  # Placement is a separate gate. Remove every old routed object while
  # retaining the single outline; routers will rebuild owned copper later.
  final['TRACK']={gid:o for gid,o in final.get('TRACK',{}).items() if str(o.get('layerid'))=='10'}
  final['VIA']={}
  final['COPPERAREA']={}
  c.api('applySource',{'source':final,'createNew':False});time.sleep(8)
  check=builder.source(c);bad=[]
  for alias,(x,y) in POSITIONS.items():
   h=check['FOOTPRINT'][STATE['ids'][alias]]['head']
   if abs(float(h['x'])-x)>.02 or abs(float(h['y'])-y)>.02:bad.append(alias)
  if bad:raise RuntimeError(f'post-apply placement mismatch: {bad}')
  wrong_sides=[a for a in BOTTOM_PARTS if str(check['FOOTPRINT'][STATE['ids'][a]]['head'].get('layerid'))!='2']
  wrong_rotations=[a for a,r in ROTATIONS.items() if abs(float(check['FOOTPRINT'][STATE['ids'][a]]['head'].get('rotation') or 0)%360-r)>.02]
  if wrong_sides or wrong_rotations:raise RuntimeError({'wrong_sides':wrong_sides,'wrong_rotations':wrong_rotations})
  jp=check['FOOTPRINT'][STATE['ids']['J_LED']];actual=sorted((str(p['number']),round(float(p['x']),4),round(float(p['y']),4),round(float(p['width'])*.254,3),round(float(p['holeR'])*2*.254,3)) for p in jp['PAD'].values())
  input_pads=check['FOOTPRINT'][STATE['ids']['J_5V_IN']]['PAD'].values()
  inp=sorted((str(p['number']),p['net'],round(float(p['x']),4),round(float(p['y']),4),round(float(p['width'])*.254,3),round(float(p.get('holeR',0))*2*.254,3)) for p in input_pads if str(p['number']) in {'1','2'})
  if len(inp)!=2 or round(((inp[1][2]-inp[0][2])**2+(inp[1][3]-inp[0][3])**2)**.5*.254,3)!=3.0:raise RuntimeError(f'input landing mismatch: {inp}')
  aux=next(p for p in input_pads if str(p['number'])=='3')
  if aux['net']!='GND' or str(aux['layerid'])!='1' or float(aux.get('holeR',0))!=0:raise RuntimeError(f'input auxiliary pad mismatch: {aux}')
  c.api('doCommand',{'cmd':'file_save'});time.sleep(3)
  print(json.dumps({'placed':len(POSITIONS),'bottom_pogo_pads':len(TPS),'board_mm':[18,27],'input_pigtail_pads':inp,'led_pigtail_pads':actual,'cable_row_y':3521.4567,'antenna_overhang_mm':5.05},indent=2))
 finally:c.close()
if __name__=='__main__':main()
