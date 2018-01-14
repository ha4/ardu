set ConfMode "115200,n,8,1"

set ::AutoPlotM::dset(setT,yaxis)  "yT"
set ::AutoPlotM::dset(setTD,yaxis)  "yTD"
set ::AutoPlotM::dset(set0,yaxis)   "y0"
set ::AutoPlotM::dset(set1,yaxis)   "y1"
set ::AutoPlotM::dset(set4,yaxis)   "y4"
set ::AutoPlotM::dset(set5,yaxis)   "y5"

set ::AutoPlotM::dset(setT,color)  "navy"
set ::AutoPlotM::dset(setTD,color) "magenta"

set ::AutoPlotM::dset(set0,color)  "black"
set ::AutoPlotM::dset(set1,color)  "saddle brown"
set ::AutoPlotM::dset(set2,color)  "firebrick"
set ::AutoPlotM::dset(set3,color)  "orange red"
set ::AutoPlotM::dset(set4,color)  "gold"
set ::AutoPlotM::dset(set5,color)  "forest green"
set ::AutoPlotM::dset(set6,color)  "royal blue"
set ::AutoPlotM::dset(set7,color)  "blue violet"


set ::AutoPlotM::pscale(y0,tcolor) "black"        
set ::AutoPlotM::pscale(y1,tcolor) "saddle brown" 
set ::AutoPlotM::pscale(y2,tcolor) "firebrick"    
set ::AutoPlotM::pscale(y3,tcolor) "orange red"   
set ::AutoPlotM::pscale(y4,tcolor) "gold"         
set ::AutoPlotM::pscale(y5,tcolor) "forest green" 
set ::AutoPlotM::pscale(y6,tcolor) "royal blue"   
set ::AutoPlotM::pscale(y7,tcolor) "blue violet"  
set ::AutoPlotM::pscale(yT,tcolor)  "navy"
set ::AutoPlotM::pscale(yT,gcolor)  "sky blue"
set ::AutoPlotM::pscale(yT,offset)  45
set ::AutoPlotM::pscale(yTD,tcolor)  "magenta"
set ::AutoPlotM::pscale(yTD,gcolor)  "pink"
set ::AutoPlotM::pscale(yTD,offset)  60
set ::AutoPlotM::pscale(yTD,fmt)  "%5.3f"
set ::AutoPlotM::pscale(y,fmt)  "%7.3f"

global chan
global sertime
global pdat
global cdat
global clst

# -- SERIAL DATA HANDLER

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

proc data_in {s} {             
	global chart
	global datax
	global lnum
	global pdat
	global cdat
	global clst

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
	set vx [lrange $s 1 8]
	foreach {q p} [list T end-1 TD end] {
		set x [lindex $s $p]
		if {$cdat && [string is double -strict $x]} {$chart $t $x set$q}
	}
	if {$pdat} {puts "$t $vc $vx"}
 	set datax [lindex $vx $vc]

# store to file
	put_log "$t $s"

# plot	
	if {$cdat} { foreach c $clst {$chart $t [lindex $vx $c] set$c} }
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
# -- COMMAND EXTENSTION
.mbar.fl insert 6 command -label "Reload Extension" -command { source dataserial2-ext.tcl }
.mbar.fl insert 7 command -label "Reset controller" -command { cmd_rst }
.mbar.dat insert 1 command -label "replot" -command cmd_replot
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
# checkbutton .c -text "Click me" -indicatoron false
pack [checkbutton .toolbar.cmdR  -text "REF" -indicatoron false -command {bcmd .toolbar.cmdR r R}] -side left
pack [checkbutton .toolbar.cmdO  -text "OUT" -indicatoron false -command {bcmd .toolbar.cmdO o O}] -side left
pack [entry    .toolbar.clst -textvar clst] -side left

}
