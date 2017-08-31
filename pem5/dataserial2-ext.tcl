
# -- SERIAL DATA HANDLER

proc ser_ext_init {} {
	global chan

	set chan ""
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
		 set str $PortMsg
		 set PortMsg ""
		 $func $str
 		}
          default { append PortMsg $ch  }
	  }
	}
}

proc ser_restart {v_port v_func} {
	global serredo

	ser_stop
	ser_start $v_port $v_func

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
# store to file
	put_log "$t $s"

	if {![string is double -strict $s]} { return }
	set datav [format "%.3f" [expr 2500.0*($s-100000.0)/800000.0]]
	set datai [format "%.4f" [expr $datav*0.1317]]
	set datax "$s,$datav,$datai"
	$chart $t $datai
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
	{Port "Port Mode" } \
	[list $ConfPort $ConfMode ]]

   if { [llength $p] == 0 } { return }
   foreach {ConfPort ConfMode sertime} $p {break}
}

proc menu_ext_init {} {
.mbar.fl insert 1 command -label "Connection set-up" -command { cmd_setup }

# -- COMMAND EXTENSTION
.mbar.fl add command -label "Reload Extension" -command { source dataserial2-ext.tcl }
}

proc extension_init {} {
	global config_vars
	global ::AutoPlotM::yfmt
	global ConfPrint
	set config_vars {ConfPort ConfMode ConfFile ConfGenerateFn sertime ConfPrint}

	set ::AutoPlotM::pscale(y,fmt)  "%.4f"
	set ConfPrint 0

	ser_ext_init
	menu_ext_init
}