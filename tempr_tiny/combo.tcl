#!/bin/sh
# the next line restarts using the correct interpreter \
exec wish "$0" "$@"

frame .statusbar -bd 1 -relief raised -height 18
frame .statusbar.fill -bd 1 -relief sunken -height 18

# The toolbar is packed to the root window. It is horizontally stretched.
pack .statusbar -fill x -expand false -side bottom
pack .statusbar.fill -fill x -expand true -side right


label .statusbar.msg -bd 1 -relief sunken -width 55 -anchor w -text "wait.."
tk_optionMenu .statusbar.msg2 x500
pack .statusbar.msg  -before .statusbar.fill -side left -expand false -fill x 
pack .statusbar.msg2 -before .statusbar.fill -side left -expand false -fill x 
proc set_status {s} { .statusbar.msg configure -text $s }
proc set_status2 {s} { .statusbar.msg2 configure -text $s }


set l {4 5 6}
set optname [tk_optionMenu .statusbar.menux varname junk ]
pack .statusbar.menux -side left
$optname delete 0
set j [llength $l]
for {set i 0} {$i < $j} {incr i} {
    set e [lindex $l $i]
    $optname insert $i radiobutton -label $e -variable menvar -command \
        {global menvar; set varname $menvar}
}
