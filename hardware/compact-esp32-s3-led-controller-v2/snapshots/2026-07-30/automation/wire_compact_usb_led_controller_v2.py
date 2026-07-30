#!/usr/bin/env python3
"""Wire the clean-sheet V2 embedded-antenna USB LED controller schematic."""
from __future__ import annotations
import importlib.util,json,re,time
from pathlib import Path
ROOT=Path.home()/'.local/share/easyeda-agent-harness'
spec=importlib.util.spec_from_file_location('builder',ROOT/'build_compact_usb_led_controller_v2.py');assert spec and spec.loader
builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
STATE_PATH=builder.STATE_PATH
# The current verified EasyEDA/LCSC symbol aliases the module's official
# ground lands 61-65 to one logical pin named GND. Validate the generated
# footprint geometry against the Espressif land pattern before release.
ESP_GND={str(i) for i in [1,2,42,43,*range(46,61)]}|{'GND'}
ESP_USED={'3':'3V3','4':'BOOT','22':'LED_DATA_3V3','39':'UART_TX','40':'UART_RX','45':'ESP_EN',**{p:'GND' for p in ESP_GND}}
ESP_ALL={str(i) for i in range(1,61)}|{'GND'}
NETS={
 'U_ESP':ESP_USED,
 'J_USB':{'A1B12':'GND','A4B9':'VBUS_RAW','A5':'USB_CC1','B5':'USB_CC2','B4A9':'VBUS_RAW','B1A12':'GND','1':'GND','2':'GND','3':'GND','4':'GND'},
 'D_VBUS':{'1':'VBUS_RAW','2':'GND'},'F_VBUS':{'1':'VBUS_RAW','2':'VBUS'},
 'U_3V3':{'1':'VBUS','2':'GND','3':'VBUS','5':'3V3'},
 'U_LEVEL':{'1':'GND','2':'LED_DATA_3V3','3':'GND','4':'LED_BUF_OUT','5':'VBUS'},
 'J_LED':{'1':'VBUS','2':'LED_DATA','3':'GND'},

 'R_CC1':{'1':'USB_CC1','2':'GND'},'R_CC2':{'1':'USB_CC2','2':'GND'},

 'R_EN':{'1':'3V3','2':'ESP_EN'},'C_EN':{'1':'ESP_EN','2':'GND'},'R_BOOT':{'1':'3V3','2':'BOOT'},
 'R_LEVEL_PD':{'1':'LED_DATA_3V3','2':'GND'},'R_DATA':{'1':'LED_BUF_OUT','2':'LED_DATA'},
 'C_LEVEL':{'1':'VBUS','2':'GND'},'C_VBUS_IN':{'1':'VBUS','2':'GND'},
 'C_3V3_OUT1':{'1':'3V3','2':'GND'},
 'C_ESP_DEC':{'1':'3V3','2':'GND'},
 'TP_5V':{'1':'VBUS'},'TP_GND':{'1':'GND'},'TP_TX':{'1':'UART_TX'},'TP_RX':{'1':'UART_RX'},'TP_EN':{'1':'ESP_EN'},'TP_BOOT':{'1':'BOOT'},
}
NC={'U_ESP':ESP_ALL-set(ESP_USED),'J_USB':{'A6','A7','A8','B6','B7','B8'},'U_3V3':{'4'}}
def safe_id(*parts):return 'agent_'+'_'.join(re.sub(r'[^A-Za-z0-9]+','_',p).strip('_') for p in parts)
def wire_label(cdp,alias,pin,net):
 x,y=pin['x'],pin['y'];rot=pin['rotation'];dx={0:1,180:-1}.get(rot%360,0)
 cdp.api('createShape',{'shapeType':'netlabel','jsonCache':{'gId':safe_id('label',alias,pin['number']),'pinDot':{'x':x,'y':y},'rotation':0,'fillColor':'#0000ff','name':net,'textAnchor':'start' if dx>=0 else 'end','x':str(x+(2 if dx>=0 else -2)),'y':str(y-2.5),'fontFamily':'Times New Roman','fontSize':'7pt','locked':0}})
def nc(cdp,alias,pin):
 x,y=pin['x'],pin['y'];gid=safe_id('nc',alias,pin['number']);cdp.api('createShape',{'shapeType':'noconnectflag','jsonCache':{'x':str(x),'y':str(y),'gId':gid,'pathString':f'M {x-4} {y-4} L {x+4} {y+4} M {x+4} {y-4} L {x-4} {y+4}','strokeColor':'#33cc33','locked':0}})
def main():
 state=json.loads(STATE_PATH.read_text());cdp=builder.base.CDP()
 try:
  src=builder.base.source(cdp);builder.base.delete_ids(cdp,list(src.get('wire',{}))+list(src.get('netlabel',{}))+list(src.get('noconnectflag',{})))
  assigned=ncc=0
  for alias,pins in state['pins'].items():
   by={p['number']:p for p in pins};configured=set(NETS.get(alias,{}))|NC.get(alias,set())
   if set(by)!=configured:raise RuntimeError(f'{alias}: unmapped={sorted(set(by)-configured)} invalid={sorted(configured-set(by))}')
   for number,net in NETS.get(alias,{}).items():wire_label(cdp,alias,by[number],net);assigned+=1
   for number in NC.get(alias,set()):nc(cdp,alias,by[number]);ncc+=1
  cdp.api('doCommand',{'cmd':'file_save'});time.sleep(2)
  final=builder.base.source(cdp);print(json.dumps({'components':len(final.get('schlib',{}))-1,'netlabels':len(final.get('netlabel',{})),'no_connect_flags':len(final.get('noconnectflag',{})),'assigned_pins':assigned,'expected_nc':ncc},indent=2))
 finally:cdp.close()
if __name__=='__main__':main()
