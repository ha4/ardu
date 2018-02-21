set ConfMode "115200,n,8,1"

set config_vars {ConfPort ConfMode ConfFile ConfGenerateFn zs fs}

set ::AutoPlotM::dset(set1,yaxis)   "y1"
set ::AutoPlotM::dset(set1,color)  "saddle brown"
set ::AutoPlotM::pscale(y1,tcolor) "saddle brown" 
set ::AutoPlotM::pscale(y1,offset)  45
set ::AutoPlotM::pscale(y1,fmt)  "%7.4f"

global chan
global sertime
global pdat
global cdat
global clst
global zs
global fs

# -- SERIAL DATA HANDLER

proc ser_cmd {cmd} {
	global chan

	puts  $chan $cmd
	flush $chan
}

proc ser_port_in {lchan port func} {
	if {[catch {set rd [read $lchan 1]} errx]} {puts "read error"; return}
	ser_watchdog $port $func
	foreach ch [split $rd {}] {global PortMsg
		switch -regexp $ch {
		[\x0A\x0D] {set str $PortMsg; set PortMsg ""; $func $str}
		default {append PortMsg $ch}}
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

	set chan [open $v_port "RDWR"]
	fconfigure $chan -mode $ConfMode -translation binary \
            -buffering line -blocking 0 -handshake none -ttycontrol {DTR 0}

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

proc filter {v} {
	global avgf10
	global avgv10
	set avgv10 [if {[info exist avgv10]} {expr ($avgv10+$v)} else {set v}]
	if {[incr avgf10] >= 10} {
		set v [expr {$avgv10/$avgf10}]; unset avgf10; unset avgv10; return $v
	} else {return [list]}
}

proc data_in {s} {             
	global chart
	global datax
	global lnum
	global pdat
	global cdat
	global clst
	global zs
	global fs

# timing
	set t [get_clock]
	if {$s == ""} { return }
	set x $s

# parse
	set vc [lindex $s 0]
	if {![string is integer -strict $vc]} {
		puts "$s"
		return
	}
	set fv [filter $vc]
	if {[llength $fv]} {
		if {[info exist zs]} {set vc [expr $vc-$zs]} else {average $vc}
		if {[info exist fs]} {set vc [expr $vc/$fs]}
	 	set datax $vc
		$chart $t $vc set1

# store to file
		put_log "$t $vc"
	}

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

proc cmd_rst {} {
	global chan

	fconfigure $chan -ttycontrol {DTR 1}
	after 100
	fconfigure $chan -ttycontrol {DTR 0}
}

proc cmd_zero {} {
	global zs
	unset -nocomplain zs
	set retc [tk_messageBox -message "Press OK when calibration complete" -type ok -icon warning -title "Zero Calibration"]
	set zs [average {}]
}

proc average {v} {
	global avgfn
	global avgvn

	if [llength $v] {
		incr avgfn
		set avgvn [if {[info exist avgvn]} {expr ($avgvn+$v)} else {set v}]
		return $v
	} else {
		set v [expr {$avgvn/$avgfn}];
		unset avgfn
		unset avgvn
		return $v
	}
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

proc bcmd {w off on} {
   upvar [$w cget -variable] x1 
   if {$x1} {ser_cmd $on} else {ser_cmd $off}
}

proc extension_init {} {
global chan
global sertime
global pdat
global cdat
global clst
global zs
global fs
# -- COMMAND EXTENSTION
.mbar.fl insert 6 command -label "Reload Extension" -command { source dataserial2-ext.tcl }
.mbar.fl insert 7 command -label "Reset controller" -command { cmd_rst }
.mbar.dat insert 1 command -label "replot" -command cmd_replot
.mbar.dat insert 2 command -label "Zero" -command cmd_zero
.mbar.dat add checkbutton -label "puts data" -variable pdat
.mbar.dat add checkbutton -label "chart data" -variable cdat
.mbar.dat add separator
.mbar.dat add command -label "Send a2RO5 (diode)" -command { ser_cmd a2RO5 }
.mbar.dat add command -label "Send Fn1 (nofilter)" -command { ser_cmd Fn1 }
.mbar.dat add command -label "Send command" -command cmd_send
set chan ""
set sertime 3000
set pdat 1
set cdat 1
set clst {0 1 2 3 4 5 6 7}
if {![info exist zs]} {set zs 0}
if {![info exist fs]} {set fs 62080.52}

# checkbutton .c -text "Click me" -indicatoron false
pack [checkbutton .toolbar.cmdR  -text "REF" -indicatoron false -command {bcmd .toolbar.cmdR r R}] -side left
pack [checkbutton .toolbar.cmdO  -text "OUT" -indicatoron false -command {bcmd .toolbar.cmdO o O}] -side left
pack [entry    .toolbar.clst -textvar clst] -side left

}
