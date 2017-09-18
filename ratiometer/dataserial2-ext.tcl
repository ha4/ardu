
# -- SERIAL DATA HANDLER

proc ser_ext_init {} {
	global chan
	global sertime

	set chan ""
	set sertime 3000
}

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
	global avg_do

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

        set xs [regexp -all -inline {\S+} $s]
	set xz [llength $xs]

# 0mV 1code
	if {$xz==2} {
	        set x [lindex $xs 1]
		if {![string is double -strict $x]} { return }
	 	set datax [lindex $xs 0]
		$chart $t $x
		return
	}


# 0samplesN 1code.self 2avg.code 3+- 4stddev 5code.sys 6avg.code 7+- 8stddev 9koeff
	if {$xz==10} { 
	        set x [lindex $xs 1]
	        set x2 [lindex $xs 5]
		if {![string is double -strict $x]} { return }
		if {![string is double -strict $x2]} { return }
	 	set datax [lindex $xs 9]
		$chart $t $x  seta
		$chart $t $x2 setb
		return
	}
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

proc menu_ext_init {} {
.mbar.fl insert 1 command -label "Connection set-up" -command { cmd_setup }

# -- COMMAND EXTENSTION
.mbar.dat add command -label "Print input" -command { set ConfPrint [expr 1-$ConfPrint] }
.mbar.dat add separator
.mbar.dat add command -label "AD7714 info" -command { adccmd ? }
.mbar.dat add separator
.mbar.dat add command -label "AD7714 bipolar" -command { adccmd b }
.mbar.dat add command -label "AD7714 unipolar" -command { adccmd u }
.mbar.dat add command -label "AD7714 channel.." -command { adccmd c 1 }
.mbar.dat add command -label "AD7714 filter ratio.." -command { adccmd r 1250 }
.mbar.dat add command -label "AD7714 filter ratio 1250" -command { adccmd r1250 }
.mbar.dat add command -label "AD7714 filter ratio 3125" -command { adccmd r3125 }
.mbar.dat add command -label "AD7714 gain.." -command { adccmd p 1 }
.mbar.dat add separator
.mbar.dat add command -label "AD7714 Init" -command { adccmd i }
.mbar.dat add command -label "AD7714 FSYNC" -command { adccmd f }
.mbar.dat add separator
.mbar.dat add command -label "AD7714 zero-scale.." -command { adccmd o 500 }
.mbar.dat add command -label "AD7714 full-scale.." -command { adccmd g 600 }
.mbar.dat add command -label "AD7714 self-calibrate" -command { adccmd a }
.mbar.dat add separator
.mbar.dat add command -label "AD7714 mode normal" -command { adccmd d }
.mbar.dat add command -label "AD7714 mode self-zero" -command { adccmd z }
.mbar.dat add command -label "AD7714 mode sele-scale" -command { adccmd f }
.mbar.dat add command -label "AD7714 mode sys-zero" -command { adccmd Z }
.mbar.dat add command -label "AD7714 mode sys-scale" -command { adccmd F }

.mbar.fl add command -label "Reload Extension" -command { source dataserial2-ext.tcl }
}

proc adccmd {str {parm ""}} {
  if {$parm ne ""} {
   set p [tk_inputer .adcmd "Send Command" \
	{Command } \
	[list [append str $parm]]]

   if { [llength $p] == 0 } { return } else { set str [lindex $p 0] }
  }
  ser_cmd $str
}

# -- STATUSBAR
proc status_ext_init {} {
label .statusbar.msg -bd 1 -relief sunken -width 35 -anchor w -text "wait.."
label .statusbar.msg2 -bd 1 -relief sunken -width 85 -anchor w
# tk_optionMenu .statusbar.msg2 x500 nothing
pack .statusbar.msg  -before .statusbar.fill -side left -expand false -fill x 
pack .statusbar.msg2 -before .statusbar.fill -side left -expand false -fill x 
proc set_status  {s} { .statusbar.msg  configure -text $s }
proc set_status2 {s} { .statusbar.msg2 configure -text $s }
# proc set_status2 {s} { .statusbar.msg2.menu insert 0 radiobutton -label $s }
}

# -- SERIAL COMMANDS PUSHER
proc serpush_ext_init {} {
label .toolbar.llcmd -text "cmd:"
entry .toolbar.lcmd -width 10 -textvariable comtxt 
pack .toolbar.llcmd -after .toolbar.clrb -side left
pack .toolbar.lcmd -after .toolbar.llcmd -pady 4 -padx 10 -side left
bind .toolbar.lcmd <Return> { ser_cmd $comtxt }
}

proc extension_init {} {
	global config_vars
	global ::AutoPlotM::yfmt
	global ConfPrint
	set config_vars {ConfPort ConfMode ConfFile ConfGenerateFn sertime ConfPrint}

	set ::AutoPlotM::yfmt  "%.4g"
	set ConfPrint 0

	ser_ext_init
	menu_ext_init
	status_ext_init
	serpush_ext_init
}