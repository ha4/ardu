
module ring(r1,r2,h1) {
    difference() {
        cylinder(r=r1, h=h1, center=true, $fs=0.4);
        cylinder(r=r2, h=h1*1.01, center=true, $fs=0.4);
    }
}

module beams() {
difference() {
    
    union() {
        rotate([0,22.5,0]) translate([0,0,-3]) {
            cylinder(d=3, h=50, $fs=0.4);
            translate([0,0,19]) cylinder(d=4.2, h=10, $fs=0.4);
        }
        translate([2.5,0,0]) rotate([0,-22.5,0]) translate([0,0,-3]){ 
            cylinder(d=3, h=50, $fs=0.4);
            translate([0,0,20]) cylinder(d=4.2, h=10, $fs=0.4);
        }
        translate([-2.5,0,0]) rotate([0,-22.5,0]) translate([0,0,-3]){ 
            cylinder(d=3, h=50, $fs=0.4);
            translate([0,0,18]) cylinder(d=4.2, h=10, $fs=0.4);
        }
    }
    
    translate([0,0,-6]) cube(12,center=true);
}
}

module block() {
    difference() {
      intersection() {
        rotate([90,0,0])ring(r1=33,r2=10,h1=6);
        translate([0,0,14])rotate([0,45,0])cube(20, center=true);
      }
      translate([-30,-15,18])cube([60,20,40]);
      translate([0,0,15])rotate([90,0,0])ring(20,12,8);
      cube(18,center=true);
    }
}

module holder() {
    difference() {
        block();
        beams();
        translate([0,0,15])rotate([90,0,0])cylinder(d=3.2, h=10,center=true, $fs=0.4);
    }
 }

// translate to print base
translate([0,0,18])rotate([180,0,0]) holder();
