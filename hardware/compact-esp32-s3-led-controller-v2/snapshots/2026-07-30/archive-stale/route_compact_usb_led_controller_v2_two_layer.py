#!/usr/bin/env python3
"""Deterministically route the power-only, LDO-based V2 on two copper layers."""
from __future__ import annotations
import importlib.util,json,time
from pathlib import Path
ROOT=Path.home()/'.local/share/easyeda-agent-harness'
spec=importlib.util.spec_from_file_location('builder',ROOT/'build_esp32_lipo_led_board.py');assert spec and spec.loader
builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
# EasyEDA source units are 0.254 mm. Layer 1=top, 2=bottom.
TRACKS={
 # Raw USB power: both receptacle VBUS contacts, TVS, then resettable PPTC.
 'agent_v2_raw_left':('VBUS_RAW','1',2.0,[(4128.551,3503.36),(4128.551,3498),(4112,3498),(4108,3500.952)]),
 'agent_v2_raw_tvs_fuse':('VBUS_RAW','1',3.0,[(4108,3500.952),(4112,3504),(4112,3508),(4105.297,3512)]),
 'agent_v2_raw_right_top':('VBUS_RAW','1',2.0,[(4147.45,3503.36),(4147.45,3496)]),
 'agent_v2_raw_right_bottom':('VBUS_RAW','2',3.0,[(4147.45,3496),(4147.45,3508),(4112,3508)]),
 # Protected 5 V: high-current LED branch plus lower-current logic branches.
 'agent_v2_vbus_led':('VBUS','1',5.0,[(4090.703,3512),(4078,3515),(4028.189,3515)]),
 'agent_v2_vbus_ldo_trunk':('VBUS','1',2.0,[(4090.703,3512),(4080,3507),(4080,3470),(4061.118,3463.739)]),
 'agent_v2_vbus_ldo_pins':('VBUS','1',2.0,[(4061.118,3463.739),(4061.118,3456.261),(4054,3452),(4034.063,3460)]),
 'agent_v2_vbus_level':('VBUS','1',1.5,[(4080,3492),(4069.244,3492),(4064,3484),(4053.441,3487.669)]),
 'agent_v2_vbus_test_top':('VBUS','1',2.0,[(4070,3515),(4070,3508)]),
 'agent_v2_vbus_test_bottom':('VBUS','2',1.5,[(4070,3508),(4070,3495.748)]),
 # 3.3 V LDO output and bias branches.
 'agent_v2_3v3_main':('3V3','1',2.0,[(4050.882,3463.739),(4058,3460),(4066.063,3460),(4078,3454),(4087.244,3438.5),(4100.539,3438.767)]),
 'agent_v2_3v3_boot_en':('3V3','1',1.0,[(4087.244,3438.5),(4087.034,3447),(4087.034,3489)]),
 # Boot, enable, UART, and LED control.
 'agent_v2_boot_top':('BOOT','1',1.0,[(4100.539,3442.113),(4092.966,3447),(4098,3452)]),
 'agent_v2_boot_bottom':('BOOT','2',1.0,[(4098,3452),(4082,3464),(4070,3480)]),
 'agent_v2_en_esp_top':('ESP_EN','1',1.0,[(4155.461,3432.075),(4159,3432.075)]),
 'agent_v2_en_bottom':('ESP_EN','2',1.0,[(4159,3432.075),(4160,3486),(4108,3486)]),
 'agent_v2_en_rc_top':('ESP_EN','1',1.0,[(4108,3486),(4103,3489),(4092.966,3489)]),
 'agent_v2_en_test_bottom':('ESP_EN','2',1.0,[(4108,3486),(4077.874,3487.874)]),
 'agent_v2_tx_top':('UART_TX','1',1.0,[(4155.461,3452.153),(4159,3452.153)]),
 'agent_v2_tx_bottom':('UART_TX','2',1.0,[(4159,3452.153),(4148,3470),(4080,3470),(4070,3487.874)]),
 'agent_v2_rx_top':('UART_RX','1',1.0,[(4155.461,3448.807),(4159,3448.807)]),
 'agent_v2_rx_bottom':('UART_RX','2',1.0,[(4159,3448.807),(4160,3498),(4077.874,3498),(4077.874,3495.748)]),
 'agent_v2_led_gpio_top':('LED_DATA_3V3','1',1.0,[(4124.654,3483.157),(4124.654,3488)]),
 'agent_v2_led_gpio_bottom':('LED_DATA_3V3','2',1.0,[(4124.654,3488),(4100,3500),(4062,3500)]),
 'agent_v2_led_gpio_buf':('LED_DATA_3V3','1',1.0,[(4062,3500),(4056,3500),(4056,3496.331),(4042.534,3500),(4042.534,3488)]),
 'agent_v2_led_buffer_out':('LED_BUF_OUT','1',1.0,[(4058.559,3487.669),(4064,3492),(4066.966,3502)]),
 'agent_v2_led_data':('LED_DATA','1',1.5,[(4061.034,3502),(4060,3506),(4040,3506),(4040,3515)]),
 # USB-C sink configuration; data contacts intentionally remain unconnected.
 'agent_v2_cc1':('USB_CC1','1',1.0,[(4139,3493.466),(4138,3497),(4133.079,3503.36)]),
 'agent_v2_cc2':('USB_CC2','1',1.0,[(4157,3493.466),(4160,3495),(4160,3500),(4144.89,3500),(4144.89,3503.36)]),
}
# net, x, y, diameter, hole radius in source units.
VIAS=[
 ('VBUS_RAW',4147.45,3496,3.2,.8),('VBUS_RAW',4112,3508,3.2,.8),('VBUS',4070,3508,3.2,.8),
 ('BOOT',4098,3452,2.4,.6),('ESP_EN',4159,3432.075,2.4,.6),('ESP_EN',4108,3486,2.4,.6),
 ('UART_TX',4159,3452.153,2.4,.6),('UART_RX',4159,3448.807,2.4,.6),('LED_DATA_3V3',4124.654,3488,2.4,.6),('LED_DATA_3V3',4062,3500,2.4,.6),
 *[('GND',x,y,2.4,.6) for x in (4121.504,4128.0,4134.496) for y in (3449.003,3455.499,3461.995)],
 ('GND',4045,3460,2.4,.6),('GND',4064,3460,2.4,.6),('GND',4077,3460,2.4,.6),('GND',4095,3438.5,2.4,.6),
 ('GND',4106,3489,2.4,.6),('GND',4050,3496,2.4,.6),('GND',4078,3492,2.4,.6),('GND',4048.466,3488,2.4,.6),
 ('GND',4108,3492,2.4,.6),('GND',4139,3484,2.4,.6),('GND',4157,3484,2.4,.6),
 ('GND',4024,3440,2.4,.6),('GND',4024,3490,2.4,.6),('GND',4090,3520,2.4,.6),('GND',4160,3518,2.4,.6),
]
BOARD='M 4025.906 3425 L 4161.732 3425 L 4161.732 3523.425 L 4020 3523.425 L 4020 3430.906 L 4025.906 3425 Z'
def area(gid,layer,order,name):return {'gId':gid,'layerid':layer,'net':'GND','name':name,'order':str(order),'pathStr':BOARD,'clearanceWidth':1,'fillStyle':'solid','strokeWidth':1,'thermal':'spoke','keepIsland':'none','locked':0,'gridTrackWidth':1,'gridClearance':1,'toBoardOutline':1,'fabricationImprove':'yes','spoke_width':1}
AREAS=[area('agent_v2_gnd_bottom','2',20,'V2 Bottom GND Plane'),area('agent_v2_gnd_top','1',10,'V2 Top GND Fill')]

def main():
 c=builder.CDP()
 try:
  s=builder.source(c)
  if s.get('head',{}).get('docType')!='3':raise RuntimeError('active document is not a PCB')
  if any(s['layers'][lid].get('config') for lid in ('21','22')):raise RuntimeError('inner layers are still enabled')
  old={g for typ in ('TRACK','VIA','COPPERAREA') for g in s.get(typ,{}) if g.startswith('agent_v2_')}
  for typ in ('TRACK','VIA','COPPERAREA'):
   for g in list(s.get(typ,{})):
    if g.startswith('agent_v2_'):s[typ].pop(g,None)
  s['itemOrder']=[g for g in s.get('itemOrder',[]) if g not in old]
  for gid,(net,layer,width,pts) in TRACKS.items():
   s.setdefault('TRACK',{})[gid]={'gId':gid,'layerid':layer,'net':net,'pointArr':[{'x':x,'y':y} for x,y in pts],'strokeWidth':width,'locked':0};s.setdefault('itemOrder',[]).append(gid)
  for a in AREAS:s.setdefault('COPPERAREA',{})[a['gId']]=a;s.setdefault('itemOrder',[]).append(a['gId'])
  c.api('applySource',{'source':s,'createNew':False});time.sleep(12)
  live=builder.source(c)
  for net,x,y,d,hr in VIAS:
   if not any(v.get('net')==net and abs(float(v['x'])-x)<.03 and abs(float(v['y'])-y)<.03 for v in live.get('VIA',{}).values()):
    c.api('createShape',{'shapeType':'VIA','jsonCache':{'x':x,'y':y,'net':net,'diameter':d,'holeR':hr,'layerid':11,'locked':'0'}});time.sleep(.25)
  time.sleep(8);f=builder.source(c)
  found=sum(any(v.get('net')==net and abs(float(v['x'])-x)<.03 and abs(float(v['y'])-y)<.03 for v in f.get('VIA',{}).values()) for net,x,y,_,_ in VIAS)
  fills={a['gId']:len(f.get('COPPERAREA',{}).get(a['gId'],{}).get('polygonArr',[])) for a in AREAS}
  missing=[g for g in TRACKS if g not in f.get('TRACK',{})]
  if missing or found!=len(VIAS) or not all(fills.values()):raise RuntimeError({'missing':missing,'vias':(found,len(VIAS)),'fills':fills})
  c.api('doCommand',{'cmd':'file_save'});time.sleep(3)
  print(json.dumps({'tracks':len(TRACKS),'vias':found,'ground_fill_polygons':fills,'layers':{k:f['layers'][k]['config'] for k in ('1','2','21','22')}},indent=2))
 finally:c.close()
if __name__=='__main__':main()
