#!/bin/sh
# the next line restarts using the correct interpreter \
exec wish "$0" "$@"

package require Tk

source arddchan.tcl
source vfmchan.tcl
source autoplotm.tcl
source tkinputer.tcl
source configsave.tcl

set config_file ".fmetr.conf"
set config_vars {ConfPort1 ConfSpeed1 ConfPing ConfPort2}

set ConfPort1 "\\\\.\\COM10"
set ConfPort2 "\\\\.\\COM6"
set ConfSpeed1 "115200,n,8,1"
set ConfPing 500

global chF dumpfl StartT RunT RecFile

raise .

menu .m -font "fixed"
menu .m.f -tearoff 0
menu .m.f.c -tearoff 0
.m add cascade -menu .m.f -label File -underline 0
.m.f add command -label Name... -command { cmd_open }
.m.f add command -label Record  -command { cmd_logger }
.m.f add command -label Close   -command { cmd_close }
.m.f add command -label Clear   -command { cmd_clr }
.m.f add separator
.m.f add cascade -menu .m.f.c -label connEction.. -underline 0
.m.f add command -label Setup -command { cmd_setup }
.m.f add command -label coNsole -command { cmd_cons }
.m.f add separator
.m.f add command -label Exit    -command { cmd_exit }
.m.f.c add command -label Open  -command { cmd_conn }
.m.f.c add command -label Close -command { cmd_disconn }
.m.f.c add command -label "Restart Ping" -command { cmd_start }

pack [frame  .t -bd 1 -relief raised -height 20] -fill x
pack [canvas .c -relief groove -xscrollincrement 1 -bg beige] -fill both -expand true 
pack [frame  .s -bd 1 -relief raised -height 20] -fill x

grid [label .t.t1 -width 10 -height 1 -borderwidth 2 -relief flat \
	-font {-family times -size 18} -bg pink] \
     [label .t.t2 -width 10 -height 1 -borderwidth 2 -relief flat -font \
	{-family times -size 18} -bg SpringGreen] \
     [label .t.t3 -width 10 -height 1 -borderwidth 2 -relief flat -font \
	{-family times -size 18} -bg orange] \
     [label .t.t4 -width 10 -height 1 -borderwidth 2 -relief flat -font \
	{-family times -size 18} -bg lightblue] \
     [frame .t.bx] -sticky news

pack [label .s.ind -bd 1 -relief sunken -width 1  -anchor w -text "C"] -side left
pack [entry .s.fnm -bd 1 -relief sunken -width 55 -textvariable RecFile] -side left
pack [label .s.msg -bd 1 -relief sunken -width 12 -anchor w] -side left -fill x -expand true 
# pack [frame .s.xxx -bd 1 -relief sunken -height 20] -side left

# pack  [button .bx.conf -text " setup " -command {cmd_cfg}] -side left
# pack  [button .bx.exit -text " Exit  " -command {cmd_exit}] -side left

. configure -menu .m
wm protocol . WM_DELETE_WINDOW { .m.f invoke Exit }
wm title . "Arduino data recorder"

proc set_indicator {c} { .s.ind configure -bg $c }
proc set_status {s} { .s.msg configure -text $s }
proc display1 {s} { .t.t1 configure -text $s }
proc display2 {s} { .t.t2 configure -text $s }
proc display3 {s} { .t.t3 configure -text $s }
proc display4 {s} { .t.t4 configure -text $s }


# set only first time
proc chk_const {v x} {
	upvar $v k

	if {[info exist k]} {
		return 0
	} else {
		set k $x
		return 1
	}
}

proc cmd_cons {} {
	catch {console show}
}

proc cmd_exit {} {
	global config_file config_vars

	set retc [tk_messageBox -message "Really exit?"\
		-type yesno -icon warning -title "Arduino recorder"]

  	if {$retc == yes} {
	 catch { config_save $config_file $config_vars }  
	 exit
	}
}

proc cmd_setup {} {
	global ConfPort1 ConfSpeed1 ConfPing ConfPort2
	global chF
	global chF2

	set p12 [tk_inputer .setup "Port Setup" \
	{"Channel1 port" "Channel speed" "Channel2 port" "Ping (ms)" }\
	[list $ConfPort1 $ConfSpeed1 $ConfPort2 $ConfPing]]

	if { [llength $p12] == 0 } { return }
	foreach {ConfPort1 ConfSpeed1 ConfPort2 ConfPing} $p12 {break}

	$chF port $ConfPort1
	$chF mode $ConfSpeed1
	$chF2 port $ConfPort2
}

proc cmd_exist {fn} {
	if {[file exist $fn]} {
		set retc [tk_messageBox -message \
			"File EXIST: $fn\n Overwrite?" \
			-type yesno -icon warning -title "Dual APPA file"]
		if { $retc != "yes" } { return 1 } else { return 0 }
	} else { return 0 }
}

proc cmd_open {} {
	global  dumpfl RecFile
	# upvar #0 RecFile rf
	set types {
	{{Data Files}       {.dat}        }
	{{Text Files}       {.txt}        }
	{{All Files}        *             }
	}

	if {[info exist dumpfl]} {
		tk_messageBox -message "Close File" -type ok -icon warning
		return
	}
	set filename [tk_getSaveFile -filetypes $types -defaultextension {.dat}]
	if { $filename != "" } { set RecFile $filename } else { return }
}

proc cmd_logger {} {
	global RecFile dumpfl StartT

	if {[info exist dumpfl]} { 
	 tk_messageBox -message "Close File" -type ok -icon warning
	 return
	}
	if {$RecFile == ""} {
	 tk_messageBox -message "Select File to write" -type ok -icon warning
	 return
	}
	if {[cmd_exist $RecFile]} { return }
	unset -nocomplain StartT
	set dumpfl [open $RecFile w+]
}


proc cmd_close {} {
	global  dumpfl

	if {![info exist dumpfl]} { return }

	set m $dumpfl
	unset dumpfl
	close $m
}

proc cmd_clr {} {
	::AutoPlotM::clear .c
}

proc cmd_err {m} {
	tk_messageBox -message "error: $m" -type ok -icon warning
}

proc cmd_conn {} {
	global chF
	global chF2
	if {[catch {$chF open} err]} { cmd_err " connect Fmeter $err" }
	if {[catch {$chF2 open} err]} { cmd_err " connect Fmeter2 $err" }
	$chF2 req
}


proc cmd_disconn {} {
	global chF
	if {[catch {$chF close} err]} { cmd_err " close F $err" }
}

proc cmd_start {} {
	global ConfPing
}

proc getx {self} {
	global dumpfl chart StartT RunT ConfNchan

	set clk [clock milliseconds]
	set_indicator springgreen
	chk_const StartT $clk
	set RunT [expr ($clk-$StartT)*.001]

	set x [$self data]
	set n [$self chN]

	display$n $x
	set nd($n) $x

	set v [lrange $x 0 0]
	if {![string is double -strict $x]} { puts "ard got $v"; return }
	$chart $RunT $v set$n

	if {![info exist dumpfl]} { return }

	set str [format "%.1f" $RunT]
	foreach m [lsort [array names nd]] { append str " " [lrange $nd($m) 0 0]}
	puts $dumpfl $str

	set_indicator pink

}

proc gety {self} {
	global dumpfl chart StartT RunT ConfNchan

	set clk [clock milliseconds]
	chk_const StartT $clk
	set RunT [expr ($clk-$StartT)*.001]

	set x [$self decode]
	$self req

	if {$x == ""} { return }
	set n [$self chN]
	display$n $x
	set nd($n) $x

	display$n $x
	set nd($n) $x
	set v [lrange $x 0 0]
	$chart $RunT $v set$n
}

# proc gety {self} { set x [$self decode]; $self req; puts "\[$x\]" }

display1 "Channel t"
display2 "Channel F"
display3 "Channel 3"
display4 "Channel 4"

set chart [::AutoPlotM::create .c]

set ::AutoPlotM::dset(set1,color) red
set ::AutoPlotM::dset(set2,color) darkgreen
set ::AutoPlotM::dset(set1,xaxis) x
set ::AutoPlotM::dset(set1,yaxis) y
set ::AutoPlotM::dset(set2,xaxis) x
set ::AutoPlotM::dset(set2,yaxis) y

catch {source $config_file}
catch {console hide}

set chF [::ARDD::channel 1 getx]
set chF2 [::VFM::channel 2 gety]

$chF port $ConfPort1
$chF mode $ConfSpeed1
$chF2 port $ConfPort2

cmd_conn
cmd_start
