#!/usr/bin/env python3
"""Generate source-derived CAD-ready V2 pogo fixture artifacts."""
from __future__ import annotations
import csv,importlib.util,json,math,subprocess
from pathlib import Path
ROOT=Path.home()/'.local/share/easyeda-agent-harness'
OUT=Path('/home/carlos/Documents/easyeda-exports/Compact-ESP32-S3-USB-LED-Controller-V2')
spec=importlib.util.spec_from_file_location('builder',ROOT/'build_esp32_lipo_led_board.py');assert spec and spec.loader
builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
STATE=json.loads((ROOT/'compact_usb_led_controller_v2_state.json').read_text())
UNIT=.254; X0=4020.;Y0=3425.;W=36.;H=25.;CH=1.5
SIGNALS=['TP_BOOT','TP_GND','TP_TX','TP_EN','TP_5V','TP_RX']
LABEL={'TP_BOOT':'BOOT','TP_GND':'GND','TP_TX':'TX','TP_EN':'EN','TP_5V':'5V','TP_RX':'RX'}
SUPPORTS=[('S1',3.,4.),('S2',17.,4.),('S3',3.,18.),('S4',19.,22.)]
CLAMPS=[('CLAMP_LEFT',0.,12.),('CLAMP_BOTTOM',18.,25.)]
MOUNT=[('M1',3.5,3.5),('M2',44.5,3.5),('M3',3.5,37.5),('M4',44.5,37.5)]

def f3(x):return f'{x:.3f}'
def dxf_line(layer,x1,y1,x2,y2):return f'0\nLINE\n8\n{layer}\n10\n{x1}\n20\n{y1}\n30\n0\n11\n{x2}\n21\n{y2}\n31\n0\n'
def dxf_circle(layer,x,y,r):return f'0\nCIRCLE\n8\n{layer}\n10\n{x}\n20\n{y}\n30\n0\n40\n{r}\n'
def dxf_text(layer,x,y,text,h=1.2):return f'0\nTEXT\n8\n{layer}\n10\n{x}\n20\n{y}\n30\n0\n40\n{h}\n1\n{text}\n'

def main():
 c=builder.CDP()
 try:s=builder.source(c)
 finally:c.close()
 if s.get('head',{}).get('docType')!='3':raise RuntimeError('activate V2 PCB')
 outlines=[o for o in s.get('TRACK',{}).values() if str(o.get('layerid'))=='10']
 expected=[{'x':4025.906,'y':3425},{'x':4161.732,'y':3425},{'x':4161.732,'y':3523.425},{'x':4020,'y':3523.425},{'x':4020,'y':3430.906},{'x':4025.906,'y':3425}]
 if len(outlines)!=1 or outlines[0].get('pointArr')!=expected:raise RuntimeError('canonical chamfered 36 x 25 mm outline not active')
 rows=[]
 for alias in SIGNALS:
  fp=s['FOOTPRINT'][STATE['ids'][alias]];pads=list(fp.get('PAD',{}).values())
  if len(pads)!=1 or str(fp['head'].get('layerid'))!='2':raise RuntimeError(f'{alias} is not one underside pad')
  p=pads[0];x=(float(p['x'])-X0)*UNIT;y=(float(p['y'])-Y0)*UNIT
  rows.append({'signal':LABEL[alias],'board_top_x_mm':x,'board_top_y_mm':y,'board_bottom_view_x_mm':W-x,'board_bottom_view_y_mm':y,'jig_probe_plate_x_mm':x,'jig_probe_plate_y_mm':y,'pad_width_mm':float(p['width'])*UNIT,'pad_height_mm':float(p['height'])*UNIT,'pcb_side':'bottom'})
 # Enforce the exact 2.54-mm 2x3 grid and 1.50-mm pads.
 xs=sorted(set(round(r['board_top_x_mm'],3) for r in rows));ys=sorted(set(round(r['board_top_y_mm'],3) for r in rows))
 if len(xs)!=2 or len(ys)!=3 or abs(xs[1]-xs[0]-2.54)>.001 or any(abs(ys[i+1]-ys[i]-2.54)>.001 for i in range(2)):raise RuntimeError((xs,ys))
 if any(abs(r['pad_width_mm']-1.5)>.01 or abs(r['pad_height_mm']-1.5)>.01 for r in rows):raise RuntimeError('unexpected pad size')
 OUT.mkdir(parents=True,exist_ok=True)
 with (OUT/'pogo-fixture-coordinates.csv').open('w',newline='') as f:
  w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows({k:(f3(v) if isinstance(v,float) else v) for k,v in r.items()} for r in rows)
 mech=[]
 for n,x,y in SUPPORTS:mech.append({'feature':n,'frame':'board_top_projection','x_mm':x,'y_mm':y,'diameter_mm':3.0,'purpose':'nonconductive PCB support post'})
 for n,x,y in CLAMPS:mech.append({'feature':n,'frame':'board_top_projection','x_mm':x,'y_mm':y,'diameter_mm':'','purpose':'suggested compliant clamp center'})
 for n,x,y in MOUNT:mech.append({'feature':n,'frame':'fixture_base','x_mm':x,'y_mm':y,'diameter_mm':3.4,'purpose':'M3 fixture mounting clearance'})
 with (OUT/'pogo-fixture-mechanical.csv').open('w',newline='') as f:
  w=csv.DictWriter(f,fieldnames=list(mech[0]));w.writeheader();w.writerows(mech)
 params={'authority':'Generated from active EasyEDA V2 PCB source','units':'mm','board':{'width':W,'height':H,'origin':'carrier top-left extent; +X right, +Y toward USB/pigtail edge','outline_top_view':[[CH,0],[W,0],[W,H],[0,H],[0,CH],[CH,0]],'top_left_chamfer':CH},'orientation':{'antenna':'top edge; module antenna overhang 5.05 mm','usb_c':'bottom-right edge; shell overhang about 2.47 mm','pigtail':'bottom-left edge','anti_reversal':'1.50-mm top-left PCB chamfer plus asymmetric antenna/USB reliefs'},'pogo':{'pads':rows,'grid_columns':2,'grid_rows':3,'pitch_x':2.54,'pitch_y':2.54,'pad':[1.5,1.5],'recommended_probe_tip_max_diameter':1.0,'nominal_printed_bore':1.35,'bore_note':'Tune to the selected barrel/sleeve and printer using a hole coupon; do not order from this nominal alone.','working_compression_range':[1.3,1.7],'compression_note':'Model-dependent; verify selected probe data sheet and assembled stack.'},'supports':SUPPORTS,'clamps':CLAMPS,'fixture_base':{'width':48,'height':41,'thickness':6,'board_origin':[6,10],'mount_holes':MOUNT},'coordinate_warning':'board_bottom_view_x is mirrored artwork/viewing geometry. Actual probe-plate machining with the board TOP facing up uses jig_probe_plate_x/y, which equal the physical board projection.'}
 (OUT/'pogo-fixture-parameters.json').write_text(json.dumps(params,indent=2)+'\n')
 # Dimensioned SVG: top/component view and mirrored board-bottom view.
 S=10;ox1,oy=55,95;ox2=505
 def outline_pts(ox,mirror=False):
  pts=params['board']['outline_top_view'];
  if mirror:pts=[[W-x,y] for x,y in pts]
  return ' '.join(f'{ox+x*S},{oy+y*S}' for x,y in pts)
 svg=[f'<svg xmlns="http://www.w3.org/2000/svg" width="920" height="430" viewBox="0 0 920 430">','<style>text{font-family:monospace;font-size:12px}.board{fill:#eef6ff;stroke:#123b66;stroke-width:2}.pad{fill:#d6a500;stroke:#5b4500}.support{fill:none;stroke:#555;stroke-dasharray:4 3}.relief{fill:#ffd9d9;stroke:#a00;stroke-dasharray:5 3}.dim{stroke:#333;marker-start:url(#a);marker-end:url(#a)}</style>','<defs><marker id="a" markerWidth="6" markerHeight="6" refX="3" refY="3" orient="auto"><path d="M0,3 L6,0 L6,6 Z" fill="#333"/></marker></defs>',f'<text x="{ox1}" y="30" font-size="17">BOARD TOP / PHYSICAL PROJECTION</text>',f'<polygon class="board" points="{outline_pts(ox1)}"/>',f'<text x="{ox2}" y="30" font-size="17">MIRRORED BOTTOM VIEW</text>',f'<polygon class="board" points="{outline_pts(ox2,True)}"/>']
 # antenna and USB relief envelopes
 svg += [f'<rect class="relief" x="{ox1+19.732*S}" y="{oy-5.052*S}" width="{15.5*S}" height="{5.052*S}"/><text x="{ox1+20*S}" y="{oy-57}">ANTENNA OVERHANG 5.05</text>',f'<rect class="relief" x="{ox1+23.012*S}" y="{oy+25*S}" width="{9.854*S}" height="{2.473*S}"/><text x="{ox1+23*S}" y="{oy+278}">USB RELIEF</text>']
 for r in rows:
  for ox,keyx in ((ox1,'board_top_x_mm'),(ox2,'board_bottom_view_x_mm')):
   x=ox+r[keyx]*S;y=oy+r['board_top_y_mm']*S;left=r[keyx]<(W/2);tx=x-10 if left else x+10;anchor='end' if left else 'start';svg += [f'<rect class="pad" x="{x-7.5}" y="{y-7.5}" width="15" height="15"/>',f'<text x="{tx}" y="{y+4}" text-anchor="{anchor}">{r["signal"]}</text>']
 for n,x,y in SUPPORTS:svg.append(f'<circle class="support" cx="{ox1+x*S}" cy="{oy+y*S}" r="15"/><text x="{ox1+x*S-8}" y="{oy+y*S+4}">{n}</text>')
 svg += [f'<line class="dim" x1="{ox1}" y1="{oy+H*S+35}" x2="{ox1+W*S}" y2="{oy+H*S+35}"/><text x="{ox1+W*S/2-25}" y="{oy+H*S+55}">36.000 mm</text>',f'<line class="dim" x1="{ox1-30}" y1="{oy}" x2="{ox1-30}" y2="{oy+H*S}"/><text transform="translate({ox1-40},{oy+H*S/2+35}) rotate(-90)">25.000 mm</text>','<text x="55" y="425">Chamfered corner is TOP-LEFT. Coordinates are millimetres. Pad squares are 1.50 mm; pitch is 2.54 mm.</text>','</svg>']
 (OUT/'pogo-fixture-dimensioned.svg').write_text('\n'.join(svg)+'\n')
 # R12-style DXF in board-top coordinates.
 d=['0\nSECTION\n2\nENTITIES\n'];pts=params['board']['outline_top_view']
 for a,b in zip(pts,pts[1:]):d.append(dxf_line('BOARD',a[0],H-a[1],b[0],H-b[1]))
 for r in rows:
  x,y=r['board_top_x_mm'],H-r['board_top_y_mm'];d.append(dxf_circle('POGO',x,y,.75));d.append(dxf_circle('PROBE_BORE',x,y,.675));d.append(dxf_text('LABEL',x+1,y,LABEL['TP_'+r['signal']] if 'TP_'+r['signal'] in LABEL else r['signal']))
 for n,x,y in SUPPORTS:d.append(dxf_circle('SUPPORT',x,H-y,1.5));d.append(dxf_text('LABEL',x+1.7,H-y,n))
 d.append('0\nENDSEC\n0\nEOF\n');(OUT/'pogo-fixture-board-top.dxf').write_text(''.join(d))
 # OpenSCAD probe plate: actual physical projection, board top faces up in use.
 pscad='\n  '.join(f'[{f3(r["jig_probe_plate_x_mm"])},{f3(r["jig_probe_plate_y_mm"])}]{"," if i < len(rows)-1 else ""} // {r["signal"]}' for i,r in enumerate(rows))
 sscad=', '.join(f'[{x},{y}]' for _,x,y in SUPPORTS);mscad=', '.join(f'[{x},{y}]' for _,x,y in MOUNT)
 scad=f'''// Generated from V2 PCB source. Units: mm.\n$fn=48;\nbase=[48,41,6]; board_origin=[6,10]; board=[36,25];\nprobe_bore_d=1.35; support_d=3.0; support_h=2.0; wall_t=1.5; wall_h=3.0;\npogo=[\n  {pscad}\n];\nsupports=[{sscad}];\nmounts=[{mscad}];\nmodule fixture_plate() {{\n  difference() {{\n    union() {{\n      cube(base);\n      for(p=supports) translate([board_origin[0]+p[0],board_origin[1]+p[1],base[2]]) cylinder(d=support_d,h=support_h);\n      // Left locator; top locator stops before antenna envelope.\n      translate([board_origin[0]-wall_t,board_origin[1],base[2]]) cube([wall_t,22,wall_h]);\n      translate([board_origin[0],board_origin[1]-wall_t,base[2]]) cube([18,wall_t,wall_h]);\n      // Chamfer key wedge: prevents a 180-degree insertion.\n      translate([board_origin[0]-0.1,board_origin[1]-0.1,base[2]]) linear_extrude(wall_h) polygon([[0,0],[1.5,0],[0,1.5]]);\n      // Right locator stops above the USB-C relief.\n      translate([board_origin[0]+board[0],board_origin[1],base[2]]) cube([wall_t,18,wall_h]);\n    }}\n    for(p=pogo) translate([board_origin[0]+p[0],board_origin[1]+p[1],-0.1]) cylinder(d=probe_bore_d,h=base[2]+support_h+0.2);\n    for(p=mounts) translate([p[0],p[1],-0.1]) cylinder(d=3.4,h=base[2]+0.2);\n  }}\n}}\nfixture_plate();\n'''
 (OUT/'pogo-fixture-v2.scad').write_text(scad)
 readme=f'''# V2 pogo programming fixture\n\nAuthority: generated from the active EasyEDA PCB source. Units are millimetres.\n\n## Datum and orientation\n\n- Carrier PCB extents: **36.000 × 25.000 mm**.\n- Board-top datum: top-left carrier extent; +X right, +Y toward the USB/pigtail edge.\n- Top-left PCB corner has a **1.500 mm chamfer**, providing physical anti-reversal keying.\n- Embedded antenna: top edge, projecting **5.05 mm** beyond the carrier.\n- USB-C: bottom-right, projecting approximately **2.47 mm** beyond the carrier.\n- Three-wire pigtail exits bottom-left.\n\n## Pogo field\n\n- Six bottom-side pads, each **1.50 × 1.50 mm**.\n- Two columns × three rows, **2.54 mm** X/Y pitch.\n- Exact source-derived centers are in `pogo-fixture-coordinates.csv`.\n- `board_bottom_view_x_mm = 36.000 - board_top_x_mm`; Y is unchanged.\n- Important: mirrored bottom-view coordinates are for artwork/viewing. A probe plate machined while the PCB remains top-facing-up uses `jig_probe_plate_x_mm/y_mm`, the physical board projection.\n\nBoard-top physical map:\n\n```text\nBOOT   GND\nTX     EN\n5V     RX\n```\n\nFixture TX connects board RX; fixture RX connects board TX. Assert BOOT low while pulsing EN low, then release BOOT after reset to enter the ROM loader. Never fixture-power 5V while USB-C is also attached: this design has no source ORing. Use current-limited fixture power.\n\n## Probe/mechanical starting parameters\n\n- Suggested probe tip diameter: ≤1.0 mm.\n- Nominal printed probe-barrel bore: 1.35 mm. This is deliberately a starting value—not a selected-probe specification. Print a bore coupon and update it for the real barrel/sleeve and printer shrinkage.\n- Suggested working compression: 1.3–1.7 mm, subject to the selected probe datasheet.\n- Four nonconductive 3.0-mm support-post locations are in `pogo-fixture-mechanical.csv`.\n- Suggested clamps are away from the antenna overhang, USB shell, and pigtail exit.\n- Use nonconductive fixture material near the antenna; avoid conductive/carbon-filled filament.\n\n## Files\n\n- `pogo-fixture-coordinates.csv`: electrical pad coordinates in top, mirrored-bottom, and actual probe-plate frames.\n- `pogo-fixture-mechanical.csv`: support, clamp, and fixture mounting features.\n- `pogo-fixture-parameters.json`: full machine-readable geometry and caveats.\n- `pogo-fixture-dimensioned.svg`: dimensioned top and mirrored-bottom drawing.\n- `pogo-fixture-board-top.dxf`: board-top CAD geometry.\n- `pogo-fixture-v2.scad`: printable probe-plate source; OpenSCAD was not installed locally, so STL export remains to be run after selecting the real pogo barrel.\n'''
 (OUT/'pogo-fixture-readme.md').write_text(readme)
 print(json.dumps({'pads':len(rows),'x_values':xs,'y_values':ys,'files':[p.name for p in sorted(OUT.glob('pogo-fixture-*'))]},indent=2))
if __name__=='__main__':main()
