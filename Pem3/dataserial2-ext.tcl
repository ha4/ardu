set ConfZero 0
set ConfScan 650
set ConfTo 2000
set stb2 ""
set config_vars {ConfPort ConfMode ConfFile ConfGenerateFn ConfZero ConfScan ConfTo slaveID}

# -- TEMPERATURE LEVEL INDICATOR EXSTENSION

canvas .toolbar.level -width 100 -height 20 -bg darkgrey
.toolbar.level create rectangle 0 0 50 22 -tags levelbar -fill darkgreen -outline ""
.toolbar.level create text 50 11 -text "T C" -tags tmpr -fill yellow
pack .toolbar.level -after .toolbar.clrb -pady 2 -padx 10 -side left

proc setindicator {t w} {
#  puts "Senindicator $t $w"
 if {$t == "NaN"} {
	return
 } elseif {$t == "-NaN"} {
	return
 } elseif {$w < 0.01} {
	set bk red
 } elseif {$w > 0.99} {
	set bk darkred
 } else {
	set bk darkgreen 
 }

.toolbar.level coords levelbar 0 0 [expr 9+int($w*82)] 22
.toolbar.level itemconfigure levelbar -fill $bk
.toolbar.level itemconfigure tmpr -text "[format "%.2f" $t]C/[expr int($w*1000)]‰"
}

# -- AVERAGING EXTENSION

set avg_do 0
set avg ""
set avg_alpha 0.005
.mbar.dat add command -label "Average..." -command { cmd_avg }

proc cmd_avg {} {
	global avg
	global avg_do

	set avg ""
	set avg_do 1

	set fnlb [tk_inputer .avgvar "Average Accumulator to file" \
		{temperature file} {T measure.txt}]
	if { [llength fnlb] == 0 } { return }
	set fn  [lindex $fnlb 1]
	set lab [lindex $fnlb 0]

	set avg_do 0
	append_str $fn "$lab $avg"
}

proc avg_process {v} {
	global avg
	global chart
	global avg_alpha
	global t

	if {$avg == ""} {
		set avg [expr 1.0*$v]
	} else {
		set avg [expr (1.0-$avg_alpha)*$avg + $avg_alpha*$v]
	}

	set_stb2 [format "%.5f" $avg]
	$chart $t [format "%.5f" $avg] seta
}

# -- ZERO CORRECTION EXTENSION


.mbar.dat add command -label "Zero correction" -command { cmd_zero }
.mbar.dat add command -label "Reset Zero" -command { cmd_nozero }

proc cmd_zero {} {
	global ConfZero
	global avg
	global avg_do

	set avg {}
	set avg_do 2

	set retc [tk_messageBox -message "Zero correction started" -type okcancel -icon info -title "Zero"]
	puts "$retc"
	if {$retc == "ok"} { set ConfZero $avg }

	set avg_do 0
}

proc cmd_nozero {} {
	global ConfZero
	set ConfZero 0
}

# -- MODBUS READER/THREAD EXTENSION

package require Thread 2.1

set slaveID 2

# -- modbus convert, fmt: i, iu, f, s2
proc cvt_to {i ifmt ofmt} {
   if { [catch {set ss [binary format $ifmt $i]}] == 1 } { return [list] }
   binary scan $ss $ofmt r
   return $r
}


proc ::get_result {res} {
	global chart
	global datax
	global ConfZero
	global avg_do
	global t
	
	if {[llength $res] < 19} {
		puts "result: $res"
		return
	}
	foreach {cntron ncal errcnt fsm pwml pwmh pintl pinth pdtl pdth \
		rgwait rgnew tl th il ih msl msh} $res {break}
	if {$rgnew==0} {
#		puts "result: $res"
		return
	}
	set pwm   [cvt_to [list $pwml $pwmh] s2 f]
	set pint  [cvt_to [list $pintl $pinth] s2 f]
	set pdt   [cvt_to [list $pdtl $pdth] s2 f]
	set tstat [cvt_to [list $tl $th] s2 f]
	set ipem  [cvt_to [list $il $ih] s2 f]
	set msclk [cvt_to [list $msl $msh] s2 iu]
#	puts "pwm=$pwm ipem=$ipem temp=$tstat $msclk ms"

	setindicator $tstat $pwm

# timing
	set t [get_clock]

# store to file
	if {![string is double -strict $ipem]} { return }
	set x [expr $ipem - $ConfZero]

	put_log "$t $x"
# plot	
	set datax [format "%.4f" $x]
	$chart $t [format "%.5f" $x]

# processing
	if {$avg_do==1} {
		avg_process $x
	} elseif {$avg_do==2} {
		avg_process $ipem
	}
}

set windowsTID [thread::id]

set modbusTID [thread::create {
	package require modbus

	proc run {ID cmd} {
	  set res [eval $cmd]
	  eval [subst {thread::send -async $ID {::get_result [list $res]}}]
	}

	proc ::modbus::cmdx {fun args} {
	  variable Priv
	  set fun [string range 00[expr $fun] end-1 end]
	  set mode [string tolower $Priv(-mode)]

 	  lassign [::modbus::cmd${fun}_pack {*}$args] reqCmd rspLen
	  lassign [::modbus::pack_$mode $reqCmd $rspLen ] reqCmd rspLen

	  set rspCmd [::modbus::port_send $reqCmd $rspLen]
	
	  set rspCmd [::modbus::unpack_$mode $reqCmd $rspCmd]
	  if {$rspCmd == ""} {return ""}
	  if {$mode == "tcp"} {set reqCmd [string range $reqCmd 6 end]}
	  return [::modbus::cmd${fun}_unpack $reqCmd $rspCmd]
	}
	thread::wait
}]

proc modbustask {} {
   global modbusTID
   global windowsTID
   global slaveID
   global ConfScan
   thread::send -async $modbusTID [list run $windowsTID \
	 [list ::modbus::cmdx 3 $slaveID 0 18]]
   after $ConfScan modbustask
}

proc cmd_conn {} {
   global ConfPort
   global ConfMode
   global modbusTID
   global windowsTID
   global ConfTo
   thread::send $modbusTID [list run $windowsTID [list \
	::modbus::configure -mode "RTU" -com $ConfPort -settings $ConfMode]]
   thread::send $modbusTID [list run $windowsTID {::modbus::port_open}]
   set_status "connection ok"
}

proc cmd_discon {} {
   global modbusTID
   global windowsTID
   thread::send $modbusTID [list run $windowsTID {::modbus::port_close}]
}

proc cmd_setup {} {
   global ConfPort
   global ConfMode
   global ConfTo
   global slaveID
   global ConfScan

   set p [tk_inputer .sersetup "MODBUS connection setup" \
	{"Slave ID" "Scan time(ms)" Port "Port Mode" "Data timeout(ms)" } \
	[list $slaveID $ConfScan $ConfPort $ConfMode $ConfTo ]]

   if { [llength $p] == 0 } { return }
   foreach {slaveID ConfScan ConfPort ConfMode ConfTo} $p {break}
}

.mbar.fl insert 1 command -label "Connection set-up" -command { cmd_setup }
.mbar.fl insert 2 command -label "Restart scan" -command { modbustask }

# -- STATUSBAR 

label .statusbar.msg -bd 1 -relief sunken -width 55 -anchor w -text "wait.."
entry .statusbar.msgerr -bd 2 -relief sunken -width 30 -textvariable stb2
pack .statusbar.msg -before .statusbar.fill -side left -expand false -fill x 
pack .statusbar.msgerr -before .statusbar.fill -side left -expand false -fill x
proc set_status {s} { .statusbar.msg configure -text $s }
proc set_stb2 {s} { global stb2; set stb2 $s }
# -- start periodic
modbustask
