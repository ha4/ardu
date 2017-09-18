set ::AutoPlotM::yfmt  "%.2g"
set ::AutoPlotM::plotcols(set2)  blue

pack [label .statusbar.msg -bd 1 -relief sunken -width 33 -anchor w] -side left
proc set_status {s} {.statusbar.msg configure -text $s}

# -- SERIAL DATA HANDLER

set chan ""
set sertime 3000

proc ser_cmd {cmd} {
	global chan

	puts  $chan $cmd
	flush $chan
}

proc ser_port_in {lchan port func} {
	global PortMsg

	     if {[catch {set rd [read $lchan 1]} errx]} {
	# error
		puts "read error"
		return
	     }

	foreach ch [split $rd {}] {
	  switch -regexp $ch {
          \x07 { }
          [\x0A\x0D] {
		 ser_watchdog $port $func
		 set str $PortMsg
		 set PortMsg ""
		 $func $str
 		}
          default { append PortMsg $ch  }
	  }
	}
}

proc ser_watchdog {port func} {
	global sertime
	global serredo

	if { [info exists serredo] } { after cancel $serredo }
	set serredo [after $sertime [list ser_restart $port $func]]

}

proc ser_restart {v_port v_func} {
	global serredo

	ser_stop
	ser_start $v_port $v_func
	ser_watchdog $v_port $v_func

	# generate first call
	$v_func ""
}

proc ser_start {v_port func} {
	global chan
	global ConfMode

	set chan [open $v_port r+]
	fconfigure $chan -mode $ConfMode -translation binary \
            -buffering none -blocking true

#	after 500

	fileevent $chan readable [list ser_port_in $chan $v_port $func]

}

proc ser_stop {} {
	global chan
	global serredo

	if { [info exists serredo] } { after cancel $serredo }

	if {$chan != ""} { close $chan }
        set chan ""
}


# -- PROCESSING DATA 

proc data_in {s} {             
	global chart
	global datax

# timing
	set t [get_clock]
	if {$s == ""} { return }
	foreach {a0 a1} [split $s \x20] {break}

# parse
	if {![string is double -strict $a0]} { return }
	if {![string is double -strict $a1]} { return }

# processing
 	set datax "$a0 $a1"

# store to file
	put_log "$t $a0 $a1"

# plot	
	$chart $t $a0
	$chart $t $a1 set2
# hoho
	set_status [integrate $t $a0]
}


proc integrate {t v} {
	global pre_t pre_v int_s
	if {[info exist int_s] && [info exist pre_v] && [info exist pre_t]} {
		set int_s [expr $int_s + ($t-$pre_t)*($v+$pre_v)*.5/3600000.0]
	} else { set int_s 0 }
	set pre_t $t
	set pre_v $v
	return $int_s
}

proc cmd_conn {} {
   global ConfPort

   ser_restart $ConfPort data_in
}

proc cmd_discon {} {
   ser_stop
}


