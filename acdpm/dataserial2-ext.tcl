set ConfMode "115200,n,8,1"

set ::AutoPlotM::dset(set1,color)  "black"
set ::AutoPlotM::dset(set2,yaxis)  "y"
set ::AutoPlotM::dset(set2,color)  "blue"
set ::AutoPlotM::dset(set3,yaxis)  "y2"
set ::AutoPlotM::dset(set3,color)  "red"
set ::AutoPlotM::pscale(y2,tcolor)  "red"
set ::AutoPlotM::pscale(y2,gcolor)  "tomato"
set ::AutoPlotM::pscale(y2,offset)  126

set ::AutoPlotM::dset(set4,yaxis)  "y4"
set ::AutoPlotM::dset(set4,color)  "magenta"
set ::AutoPlotM::pscale(y4,tcolor)  "magenta"
set ::AutoPlotM::pscale(y4,gcolor)  "pink"
set ::AutoPlotM::pscale(y4,offset)  200

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
	if {[llength $re] > 2 && [lindex $re 0] eq "pid"} {
		puts $s
		return
	}
	if {[llength $re] != 6} { return }

# 0:run 1:sv 2:pv 3:fan 4:mv
#	puts "$t $re"
	set ru [lindex $re 0]
	set hld [lindex $re 1]
	set sv [lindex $re 2]
	set pv [lindex $re 3]
	set fan [lindex $re 4]
	set mv [lindex $re 5]
	puts "$t $ru $hld $sv $pv $fan $mv"

# processing
 	set datax "$pv"

# store to file
	put_log "$t $s"

# plot	
	$chart $t $sv set1
	$chart $t $pv set2
	$chart $t $fan set3
	$chart $t $mv set4
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
.mbar.dat add command -label "Send command" -command cmd_send
}
