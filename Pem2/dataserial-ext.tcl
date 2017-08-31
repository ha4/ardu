set config_vars {ConfPort ConfMode ConfFile ConfSep ConfCol ConfGenerateFn ConfZero}

proc data_in {s} {             
	global chart
	global datax
	global ConfSep
	global avg_do
	global avg

# timing
	set t [get_clock]
	if {$s == ""} { return }

# parse
        set xs [split $s $ConfSep]
	if {[lindex $xs 5]} { puts $xs }
	if {[llength $xs] < 3} { return	}
        set x [lindex $xs 2]

        set temprg [string trimright [lindex $xs 1] C]
        set pwm [lindex $xs 4]
	if {[string is double -strict $temprg] && [string is double -strict $pwm]} {
		 setindicator $tempr $pwm
	}

# processing
	if {![string is double -strict $x]} { return }

	if {$avg_do} {
		set avg [expr $avg + $x]
	 	set datax "$avg"
	} else {
	 	set datax "$x"
	}


# store to file
	put_log "$t $x"

# plot	
#	if {[string is double -strict $temprg]} { $chart $t $temprg }
#	if {[string is double -strict $pwm]}    { $chart $t $pwm seta }
	$chart $t $x
}

canvas .toolbar.level -width 100 -height 16 -bg darkgrey
.toolbar.level create rectangle 0 0 50 18 -tags bar -fill darkgreen -outline ""
.toolbar.level create text 50 9 -text "T C" -tags tmpr -fill yellow
pack .toolbar.level -after .toolbar.clrb -pady 4 -padx 10 -side left

proc setindicator {t w} {
 if {$w < 10} {
	set bc darkgrey;  set bk red
 } elseif {$w > 245} {
	set bc darkgrey;  set bk darkred
 } else {
	set bc darkgrey; set bk darkgreen 
 }

.toolbar.level coords bar 0 0 [expr 9+int($w/3.19)] 18
.toolbar.level itemconfigure bar -fill $bk
.toolbar.level configure -bg $bc
.toolbar.level itemconfigure tmpr -text $t
}

.mbar.dat add command -label "Zero correction" -command { cmd_zero }

proc cmd_zero {
	global avg
	global avg_do

	set avg {}
	set avg_do 1

	set retc [tk_messageBox -message "Zero correction started" -type okcancel -icon info -title "Zero"]
	if {$retc == yes} {
		set ConfZero
	}

	set avg_do 0
}
