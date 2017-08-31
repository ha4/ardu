set config_vars {ConfPort ConfMode ConfFile ConfGenerateFn sertime ConfPrint}

set ::AutoPlotM::yfmt  "%.4g"
set ConfPrint 0

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
            -buffering none -blocking 0

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
	global ConfPrint

# timing
	set t [get_clock]
	if {$s == ""} { return }

	if {$ConfPrint} {
		puts "$t $s"
	}

# parse
	if {[string match "\#*" $s]} {
		set_status2 "$s"
		return
	}

# store to file
	put_log "$t $s"

	if {[string match ":*" $s]} {
		set_status "$s"
		return
	}

# 0code 1resist 2temp 3out
        set xs [split $s " "]
        set x [lindex $xs 0]
	if {![string is double -strict $x]} { return }
        set x2 [lindex $xs 1]
	if {![string is double -strict $x2]} { return }

# processing
 	set datax "$x $x2"

# plot	
	$chart $t $x
#	$chart $t $x2 seta
}

# -- SERIAL DATA COMMANDS

proc cmd_conn {} {
   global ConfPort

   ser_restart $ConfPort data_in
}

proc cmd_discon {} {
   ser_stop
}

proc cmd_setup {} {
   global ConfPort
   global ConfMode
   global sertime

   set p [tk_inputer .sersetup "Serial connection setup" \
	{Port "Port Mode" "Data timeout(ms)" } \
	[list $ConfPort $ConfMode $sertime ]]

   if { [llength $p] == 0 } { return }
   foreach {ConfPort ConfMode sertime} $p {break}
}

.mbar.fl insert 1 command -label "Connection set-up" -command { cmd_setup }

# -- STATUSBAR

label .statusbar.msg -bd 1 -relief sunken -width 55 -anchor w -text "wait.."
label .statusbar.msg2 -bd 1 -relief sunken -width 65 -anchor w
pack .statusbar.msg  -before .statusbar.fill -side left -expand false -fill x 
pack .statusbar.msg2 -before .statusbar.fill -side left -expand false -fill x 
proc set_status {s} { .statusbar.msg configure -text $s }
proc set_status2 {s} { .statusbar.msg2 configure -text $s }


# -- SERIAL COMMANDS PUSHER

label .toolbar.llcmd -text "cmd:"
entry .toolbar.lcmd -width 10 -textvariable comtxt 
pack .toolbar.llcmd -after .toolbar.clrb -side left
pack .toolbar.lcmd -after .toolbar.llcmd -pady 4 -padx 10 -side left
bind .toolbar.lcmd <Return> { ser_cmd $comtxt }
