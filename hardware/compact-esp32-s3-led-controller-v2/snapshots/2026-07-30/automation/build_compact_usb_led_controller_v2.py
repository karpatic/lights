#!/usr/bin/env python3
"""Build the clean-sheet V2 embedded-antenna USB LED controller schematic."""
from __future__ import annotations
import importlib.util,json,time
from pathlib import Path
ROOT=Path.home()/'.local/share/easyeda-agent-harness'
spec=importlib.util.spec_from_file_location('base_builder',ROOT/'build_esp32_lipo_led_board.py')
assert spec and spec.loader
base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base)
TARGET_PROJECT='Compact ESP32-S3 USB LED Controller V2'
STATE_PATH=ROOT/'compact_usb_led_controller_v2_state.json'
# alias, EasyEDA UUID, library title, displayed value, schematic x, schematic y
PARTS=[
 ('U_ESP','5033b192a6974b3a9787ebfd24605149','ESP32-S3-MINI-1-N8','ESP32-S3-MINI-1-N8 / embedded antenna',760,170),
 ('J_USB','df8405e2fa0e40a984c435ad4c8d5cf3','TYPE-C-31-M-12','USB-C 5V power only',100,-350),
 ('D_VBUS','c3899403907b4d44acca211d57112631','TPD1E10B06DYAR','TPD1E10B06 VBUS ESD',270,-500),
 ('F_VBUS','e64cc0b3af7c438589c7a8db8ae97782','BSMD1812-300-16V','BHFUSE 3A hold / 16V resettable PPTC / LCSC C883162',400,-500),
 ('U_3V3','ed114c3e62d8498cbfcaae87b4acc871','AP2112K-3.3TRG1','AP2112K 3.3V/600mA LDO',590,-210),
 ('U_LEVEL','24e02e87f6c4452eb820ffa636d83507','SN74AHCT1G125DCKR','SN74AHCT1G125',760,500),
 ('J_LED','584e69781c5242f391e8b163168a359e','B3P-VH (LF)(SN)','hand-soldered 3-wire pigtail: 5V DATA GND',1030,500),

 ('R_CC1','7619bb22ba3343f38e9ddf8d7f9273a2','RC0603FR-075K1L','5.1k 1%',100,-190),
 ('R_CC2','7619bb22ba3343f38e9ddf8d7f9273a2','RC0603FR-075K1L','5.1k 1%',180,-190),

 ('R_EN','9e0eced329e241a390924a11deca01dd','RC0603FR-0710KL','10k',620,100),
 ('C_EN','f833c644d37a411a8cab614d8a672ae4','GRM188R61A105KA61D','1uF',620,150),
 ('R_BOOT','9e0eced329e241a390924a11deca01dd','RC0603FR-0710KL','10k',620,230),
 ('R_LEVEL_PD','1f3ee760342e49189204cc773fbd6664','RC0603FR-07100KL','100k',760,620),
 ('R_DATA','c60a055e3f194c1fa938043a93998eb9','RC0603FR-0733RL','33R',880,500),
 ('C_LEVEL','e37ccc37b75c073f3d563908875dcb4a','GRM188R71H104KA93D','100nF',700,500),
 ('C_VBUS_IN','7a0c79967d0e7ae293044fb82790899a','CL21A106KAYNNNE','10uF 25V X5R 0805 / JLC basic C15850',470,-430),
 ('C_3V3_OUT1','f703d4878a194a199c38d96de3cd9ca1','GRM21BR61A226ME44L','22uF 10V',760,-250),
 ('C_ESP_DEC','e37ccc37b75c073f3d563908875dcb4a','GRM188R71H104KA93D','100nF',750,50),
]
TP_UUID='d5e8afd008a846e49e16c12550392416'
for i,(name,label) in enumerate([('TP_5V','5V FIXTURE'),('TP_GND','GND'),('TP_TX','UART TX'),('TP_RX','UART RX'),('TP_EN','EN RESET'),('TP_BOOT','GPIO0 BOOT')]):
 PARTS.append((name,TP_UUID,'PAD_1.5MM×1.5MM',label,430+(i%3)*70,430+(i//3)*70))

def main():
 cdp=base.CDP()
 try:
  if not cdp.evaluate('document.body.innerText.includes('+json.dumps(TARGET_PROJECT)+')'):
   raise RuntimeError(f'Open {TARGET_PROJECT!r} before running builder')
  src=base.source(cdp)
  if src.get('head',{}).get('docType')!='1':raise RuntimeError('active document is not a schematic')
  generated=[g for g in src.get('schlib',{}) if g!='frame_lib_1']+list(src.get('wire',{}))+list(src.get('netlabel',{}))+list(src.get('noconnectflag',{}))
  base.delete_ids(cdp,generated)
  ids={};used=set()
  for part in PARTS:ids[part[0]]=base.place_component(cdp,*part,used)
  final=base.source(cdp);esp=final['schlib'][ids['U_ESP']]
  datasheet='https://www.espressif.com/sites/default/files/documentation/esp32-s3-mini-1_mini-1u_datasheet_en.pdf'
  para=esp['head'].get('c_para','')
  if 'link`' in para:
   fields=para.split('`');
   for i,v in enumerate(fields[:-1]):
    if v=='link':fields[i+1]=datasheet
   esp['head']['c_para']='`'.join(fields)
  cdp.api('applySource',{'source':final,'createNew':False});time.sleep(2)
  cdp.api('doCommand',{'cmd':'file_save'});time.sleep(2)
  final=base.source(cdp)
  state={'target_project':TARGET_PROJECT,'module':{'uuid':'5033b192a6974b3a9787ebfd24605149','mpn':'ESP32-S3-MINI-1-N8','lcsc':'C2913206','jlcpcb_part_class':'Extended Part','verified_smt_stock_on_2026-07-29':3806},'ids':ids,'pins':{a:base.pin_records(final['schlib'][g]) for a,g in ids.items()}}
  STATE_PATH.write_text(json.dumps(state,indent=2),encoding='utf-8')
  print(json.dumps({'project':TARGET_PROJECT,'components':len(ids),'module':state['module'],'state':str(STATE_PATH)},indent=2))
 finally:cdp.close()
if __name__=='__main__':main()
