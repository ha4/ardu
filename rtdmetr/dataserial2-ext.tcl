set ConfMode "115200,n,8,1"
set ::AutoPlotM::dset(set2,yaxis)  "y2"
set ::AutoPlotM::dset(set2,color)  "red"
set ::AutoPlotM::dset(set3,yaxis)  "y2"
set ::AutoPlotM::dset(set3,color)  "magenta"
set ::AutoPlotM::pscale(y2,tcolor)  "red"
set ::AutoPlotM::pscale(y2,gcolor)  "pink"
set ::AutoPlotM::pscale(y2,offset)  126
set ::AutoPlotM::pscale(y2,fmt)  "%5.3f"
set ::AutoPlotM::pscale(y,fmt)  "%7.3f"

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
            -buffering line -blocking 0

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
	global lnum

# timing
	set t [get_clock]
	if {$s == ""} { return }
	set x $s

# parse
	set re [regexp -all -inline {\S+} $s]
	if {[llength $re] == 2} {
		incr lnum
		set m [lindex $re 1]
		if [string is integer -strict $m] { $chart $lnum $m set1 } else { puts $s }
		return
	}

#	puts "$t $re"
	set vc [lindex $re 0]
	if {![string is double -strict $vc]} { return }
	set vx [lindex $re [expr 1+$vc]]
	if {![string is double -strict $vx]} { return }

	set vt [lindex $re end-1]
	if {![string is double -strict $vt]} { return }
	set vtd [lindex $re end]
	if {![string is double -strict $vtd]} { return }
	puts "$t $vc $vx $vtd"

# processing
 	set datax "$vx"

# store to file
	put_log "$t $s"

# plot	
	$chart $t $vx set1
	$chart $t $vt set2
	$chart $t $vtd set3
}

proc cmd_conn {} {
   global ConfPort

   ser_restart $ConfPort data_in
}

proc cmd_discon {} {
   ser_stop
}

proc cmd_send {} {
	set res [tk_inputer .cmdin "Command" {"*Enter command" ""} {"" ""}]
	puts "send: $res"
	ser_cmd $res
}

proc replot { arg val } {
	puts "replot $arg $val"
	if {$arg ne ""} {
		set ::AutoPlotM::$arg $val
	}
	::AutoPlotM::replot .t.c
}

proc cmd_replot {} {
	set res [tk_inputer .cmdrpl "Replot" {"parameter" "value"} {"scale(y,vmin)" "0"}]
	if {[llength $res]} {replot {*}$res}
}

proc extension_init {} {
# -- COMMAND EXTENSTION
.mbar.fl insert 6 command -label "Reload Extension" -command { source dataserial2-ext.tcl }
.mbar.dat insert 1 command -label "replot" -command cmd_replot
.mbar.dat add command -label "Send a2RO5 (diode)" -command { ser_cmd a2RO5 }
.mbar.dat add command -label "Send command" -command cmd_send
}
