// Generated from V2 PCB source. Units: mm.
$fn=48;
base=[48,41,6]; board_origin=[6,10]; board=[36,25];
probe_bore_d=1.35; support_d=3.0; support_h=2.0; wall_t=1.5; wall_h=3.0;
pogo=[
  [12.192,13.462], // BOOT
  [14.732,13.462], // GND
  [12.192,16.002], // TX
  [14.732,16.002], // EN
  [12.192,18.542], // 5V
  [14.732,18.542] // RX
];
supports=[[3.0,4.0], [17.0,4.0], [3.0,18.0], [19.0,22.0]];
mounts=[[3.5,3.5], [44.5,3.5], [3.5,37.5], [44.5,37.5]];
module fixture_plate() {
  difference() {
    union() {
      cube(base);
      for(p=supports) translate([board_origin[0]+p[0],board_origin[1]+p[1],base[2]]) cylinder(d=support_d,h=support_h);
      // Left locator; top locator stops before antenna envelope.
      translate([board_origin[0]-wall_t,board_origin[1],base[2]]) cube([wall_t,22,wall_h]);
      translate([board_origin[0],board_origin[1]-wall_t,base[2]]) cube([18,wall_t,wall_h]);
      // Chamfer key wedge: prevents a 180-degree insertion.
      translate([board_origin[0]-0.1,board_origin[1]-0.1,base[2]]) linear_extrude(wall_h) polygon([[0,0],[1.5,0],[0,1.5]]);
      // Right locator stops above the USB-C relief.
      translate([board_origin[0]+board[0],board_origin[1],base[2]]) cube([wall_t,18,wall_h]);
    }
    for(p=pogo) translate([board_origin[0]+p[0],board_origin[1]+p[1],-0.1]) cylinder(d=probe_bore_d,h=base[2]+support_h+0.2);
    for(p=mounts) translate([p[0],p[1],-0.1]) cylinder(d=3.4,h=base[2]+0.2);
  }
}
fixture_plate();
