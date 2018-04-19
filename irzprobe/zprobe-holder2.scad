
module ring(r1,r2,h1) {
    difference() {
        cylinder(r=r1, h=h1, center=true, $fs=0.4);
        cylinder(r=r2, h=h1*1.01, center=true, $fs=0.4);
    }
}

module beam()
{
    union() {
        translate([0,0,-3]) cylinder(d=3, h=53, $fs=0.4);
        translate([0,0,16]) cylinder(d=4.2, h=7, $fs=0.4);
    }
}

module beams(a,dist) {
    union() {
        rotate([0,0,0])  beam();
        translate([0,0,dist/2]) rotate([0,-a,0]) beam();
        translate([0,0,-dist/2]) rotate([0,-a,0]) beam();
    }
}

module block() {
    difference() {
      intersection() {
        rotate([90,0,0])ring(r1=33,r2=11,h1=6);
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
        rotate([0,22.5,0])beams(45,5.5);
        translate([0,0,15])rotate([90,0,0])cylinder(d=3.2, h=10,center=true, $fs=0.4);
    }
}

// translate to print base
//translate([0,0,18])rotate([180,0,0])
holder();
//difference() { rotate([0,22.5,0])beams(45,5); translate([0,0,-6]) cube(12,center=true); }

