#!/bin/sh
# the next line restarts using the correct interpreter \
exec wish "$0" "$@"

package require Thread 2.1
package require modbus
source [file dirname [info script]]/tkinputer.tcl
source [file dirname [info script]]/configsave.tcl

# --------------------
# VARIABLES
#

set config_file ".dataserial.conf"
set config_vars {ConfPort ConfMode ConfScan ConfTo slaveID}

# -- DAQ variables
# The terminal bindings
set ConfPort [switch $tcl_platform(platform) \
	windows {string range "\\.\\\\COM3" 0 end } \
	default {string range "/dev/ttyUSB0" 0 end}]
set ConfMode "9600,n,8,2"
set ConfScan "1000"
set ConfTo "2000"
set errcount 0
set do_log 0
set do_log_fn "log1.txt"
set do_log_fields "opx(16)"

# --------------------
# --- WINDOWS
proc hr {wn} {frame $wn -height 2 -borderwidth 2 -relief groove}
#proc hr {wn} {frame $wn -height 2 -borderwidth 1 -relief sunken}

menu  .mbar
frame .t
wm title . "MODBUS display over Serial"
. configure -menu .mbar

menu .mbar.fl -tearoff 0

.mbar add cascade -menu .mbar.fl -label File -underline 0

.mbar.fl add command -label "Connect"   -command { cmd_conn }
.mbar.fl add command -label "Disconnect"   -command { cmd_discon }
.mbar.fl add command -label "Restart"   -command { modbustask }
.mbar.fl add command -label "Send"   -command { send_modbus $slaveID s ""}
.mbar.fl add command -label "Start log"   -command { cmd_logger }
.mbar.fl add separator
.mbar.fl add command -label "Console"   -command { cmd_cons }
.mbar.fl add separator
.mbar.fl add command -label Exit -command { cmd_exit }

frame .toolbar -bd 1 -relief raised

# An exit button with a text
button .toolbar.conn  -text "Connect" -command {cmd_conn} -width 8
entry .toolbar.port    -textvariable ConfPort -width 10
entry .toolbar.portmod -textvariable ConfMode -width 12
entry .toolbar.scanms  -textvariable ConfScan -width 8
entry .toolbar.tmout   -textvariable ConfTo   -width 8
entry .toolbar.slave   -textvariable slaveID  -width 5

pack   .toolbar.conn  -side left
pack   .toolbar.port  -side left
pack   [label  .toolbar.l2 -text " mode"] -side left -padx 5
pack   .toolbar.portmod  -side left
pack   [label  .toolbar.l3 -text " scan\[ms]"] -side left -padx 5
pack   .toolbar.scanms  -side left
pack   [label  .toolbar.l4 -text " timeout\[ms]"] -side left -padx 5
pack   .toolbar.tmout  -side left
pack   [label  .toolbar.l5 -text " slave ID"] -side left -padx 5
pack   .toolbar.slave -side left

frame .statusbar -bd 1 -relief raised -height 20
frame .statusbar.fill -bd 2 -relief sunken -height 20
pack .statusbar.fill -fill x -expand true -side right

# The toolbar is packed to the root window. It is horizontally stretched.
pack .toolbar -fill x -expand false
pack .statusbar -fill x -expand false -side bottom


event add <<SendVal>> <Double-1>
event add <<SendVal>> <Return>


labelframe .t.oper -text "Operational"
labelframe .t.parm -text "Parameters"
labelframe .t.adc  -text "ADC"

proc mbentry {wn descr adr vtype vname} {
  label $wn.l$adr -text $descr
  entry $wn.e$adr -textvariable $vname
  bind $wn.e$adr <<SendVal>> [concat send_modbus $adr $vtype \$$vname]
  return [list $wn.l$adr $wn.e$adr]
}

set opx(scan) 1
grid [checkbutton .t.oper.lsc -text "scan" -variable opx(scan)]
grid {*}[mbentry .t.oper "controller on" 0 s opx(0)]
grid {*}[mbentry .t.oper "samples n-cal" 1 s opx(1)]
grid {*}[mbentry .t.oper "error count"   2 s opx(2)]
grid {*}[mbentry .t.oper "ADC FSM state" 3 s opx(3)]
grid [hr .t.oper.lh1] - -pady 6 -padx 6 -sticky we
grid {*}[mbentry .t.oper "PI PWM signal" 4 f opx(4)]
grid {*}[mbentry .t.oper "PI integral"   6 f opx(6)]
grid {*}[mbentry .t.oper "PI delta-t"    8 f opx(8)]
grid [hr .t.oper.lh2] - -pady 6 -padx 6 -sticky we
grid {*}[mbentry .t.oper "datawait" 10 s opx(10)]
grid {*}[mbentry .t.oper "dataready" 11 s opx(11)]
grid [hr .t.oper.lh3] - -pady 6 -padx 6 -sticky we
grid {*}[mbentry .t.oper "Temperature" 12 f opx(12)]
grid {*}[mbentry .t.oper "PEM current" 14 f opx(14)]
grid [hr .t.oper.lh4] - -pady 6 -padx 6 -sticky we
grid {*}[mbentry .t.oper "milliseconds" 16 i opx(16)]

set par(scan) 0
grid [checkbutton .t.parm.l0 -text "scan" -variable par(scan)]
grid {*}[mbentry .t.parm "crc" 500 s par(500)]
grid [hr .t.parm.lh1] - -pady 6 -padx 6 -sticky we
grid {*}[mbentry .t.parm "Regulator T\[deg]" 501 f par(501)]
grid {*}[mbentry .t.parm "Span T\[deg]"      503 f par(503)]
grid {*}[mbentry .t.parm "Itegrate T\[s]"    505 f par(505)]
grid {*}[mbentry .t.parm "Tcorr+"     507 f par(507)]
grid {*}[mbentry .t.parm "Tcorr*"     509 f par(509)]
grid {*}[mbentry .t.parm "Tconv_mult" 511 f par(511)]
grid {*}[mbentry .t.parm "Tconv_base" 513 f par(513)]
grid {*}[mbentry .t.parm "PEM Amp offs"  515 f par(515)]
grid {*}[mbentry .t.parm "PEM Amp trans" 517 f par(517)]
grid [hr .t.parm.lh2] - -pady 6 -padx 6 -sticky we
grid {*}[mbentry .t.parm "ZS1"    519 i par(519)]
grid {*}[mbentry .t.parm "FS1"    521 i par(521)]
grid {*}[mbentry .t.parm "ZS2"    523 i par(523)]
grid {*}[mbentry .t.parm "FS2"    525 i par(525)]
grid {*}[mbentry .t.parm "Baud"   527 i par(527)]
grid [hr .t.parm.lh3] - -pady 6 -padx 6 -sticky we
grid {*}[mbentry .t.parm "Format" 529 s par(529)]
grid {*}[mbentry .t.parm "N-cal"  530 s par(530)]

set adcx(chan) 0
grid [checkbutton .t.adc.l0 -text "scan" -variable adcx(scan)] \
 [label .t.adc.lc -anchor e -text "channel:"] \
 [spinbox .t.adc.ec -from 0 -to 7 -incr 1 -width 2 -textvariable adcx(chan)]
#[entry .t.adc.ec -textvariable adcx(chan) -relief sunken -width 2]
grid [hr .t.parm.lh0] - -pady 6 -padx 6 -sticky we
grid [checkbutton .t.adc.cmm -text "use" -variable adcx(cm)] - -
grid [label .t.adc.l1 -text "CMM"]   [entry .t.adc.e1 -textvariable adcx(0)] -
grid [label .t.adc.l2 -text "MODE"]  [entry .t.adc.e2 -textvariable adcx(1)] -
grid [label .t.adc.l3 -text "FILTH"] [entry .t.adc.e3 -textvariable adcx(2)] -
grid [label .t.adc.l4 -text "FILTL"] [entry .t.adc.e4 -textvariable adcx(3)] -
grid [hr .t.adc.lh1] - - -pady 6 -padx 6 -sticky we
grid [label .t.adc.l5 -text "DATA"]       [entry .t.adc.e5 -textvariable adcx(4)] [checkbutton .t.adc.ccd -text "use" -variable adcx(d0)]
grid [label .t.adc.l6 -text "DATA/FSYNC"] [entry .t.adc.e6 -textvariable adcx(5)] [checkbutton .t.adc.ccs -text "use" -variable adcx(ds)]
grid [hr .t.adc.lh2] - - -pady 6 -padx 6 -sticky we
grid [label .t.adc.l7 -text "DATA/Conv"]  [entry .t.adc.e7 -textvariable adcx(6)] [checkbutton .t.adc.ccc -text "use" -variable adcx(dc)]
grid [hr .t.adc.lh3] - - -pady 6 -padx 6 -sticky we
grid [checkbutton .t.adc.cal -text "use" -variable adcx(cal)] - -
grid [label .t.adc.l8 -text "OFFSET"]     [entry .t.adc.e8 -textvariable adcx(7)] [checkbutton .t.adc.cco -text "cal" -variable adcx(co)]
grid [label .t.adc.l9 -text "GAIN"]       [entry .t.adc.e9 -textvariable adcx(8)] [checkbutton .t.adc.ccg -text "cal" -variable adcx(cg)]

bind .t.adc.e1 <<SendVal>> {send_modbus [expr 300+$adcx(chan)] s $adcx(0)}
bind .t.adc.e2 <<SendVal>> {send_modbus [expr 308+$adcx(chan)] s $adcx(1)}
bind .t.adc.e3 <<SendVal>> {send_modbus [expr 316+$adcx(chan)] s $adcx(2)}
bind .t.adc.e4 <<SendVal>> {send_modbus [expr 324+$adcx(chan)] s $adcx(3)}
bind .t.adc.e5 <<SendVal>> {send_modbus [expr 332+$adcx(chan)*2] i $adcx(4)}
bind .t.adc.e6 <<SendVal>> {send_modbus [expr 348+$adcx(chan)*2] i $adcx(5)}
bind .t.adc.e7 <<SendVal>> {send_modbus [expr 428+$adcx(chan)*2] f $adcx(6)}
bind .t.adc.e8 <<SendVal>> {
	send_modbus [expr 364+$adcx(chan)*4+32*$adcx(co)] i $adcx(7)
}
bind .t.adc.e9 <<SendVal>> {
	send_modbus [expr 366+$adcx(chan)*4+32*$adcx(cg)] i $adcx(8)
}

# -- final packing
grid .t.oper .t.parm .t.adc -sticky news  -padx 8 -pady 8
pack .t -side top -fill both -expand true


# --------------------
# -- PROGRAM COMMANDS

proc cmd_cons {} {
catch {console show}
}

proc cmd_exit {} {
  global config_file config_vars
  set retc [tk_messageBox -message "Really exit?" \
	-type yesno -icon warning -title "DataSerial Exit"]
  if {$retc == yes} {
     config_save $config_file $config_vars
     exit
  }
}


# ------------------------
# -- AUX TOOLS

proc get_clock {} {
	global StartT

	set clk [clock milliseconds]
	if { ![info exist StartT] } {
		set StartT $clk
	}
	return [expr ($clk-$StartT)/1000.0]
}

proc append_str {f s} {
	catch { set m [open $f a] }
        if {[info exist m]} {
		puts $m $s
		close $m
	}
}


# --- Widgets SETUP
# load config
catch {source $config_file}

wm protocol . WM_DELETE_WINDOW { .mbar.fl invoke Exit }
catch {console hide}
catch {cmd_conn}


# -- MODBUS READER/THREAD EXTENSION

set slaveID 2

proc ::modbus::cmd {fun args} {
	global errcount
	variable Priv
	set fun [string range 00[expr $fun] end-1 end]
	set mode [string tolower $Priv(-mode)]

	lassign [::modbus::cmd${fun}_pack {*}$args] reqCmd rspLen
	lassign [::modbus::pack_$mode $reqCmd $rspLen ] reqCmd rspLen

	set rspCmd [::modbus::port_send $reqCmd $rspLen]
	
	set rspCmd [::modbus::unpack_$mode $reqCmd $rspCmd]
	if {$rspCmd == ""} {
		incr errcount
		return ""
	}
	if {$mode == "tcp"} {set reqCmd [string range $reqCmd 6 end]}
	return [::modbus::cmd${fun}_unpack $reqCmd $rspCmd]
}

proc to_float {i} {
   if { [catch {set ss [binary format s2 $i]}] == 1 } { return [list] }
   if {[binary scan $ss f r]==0} {return [list]}
   return $r
}

proc float_to {i} {
   if { [catch {set ss [binary format f $i]}] == 1 } { return [list] }
   binary scan $ss s2 r
   return $r
}

proc to_long {i} {
   if { [catch {set ss [binary format s2 $i]}] == 1 } { return [list] }
   if {[binary scan $ss i r]==0} {return [list]}
   return $r
}
proc to_ulong {i} {
   if { [catch {set ss [binary format s2 $i]}] == 1 } { return [list] }
   if {[binary scan $ss iu r]==0} {return [list]}
   return $r
}
proc long_to {i} {
   if { [catch {set ss [binary format i $i]}] == 1 } { return [list] }
   binary scan $ss s2 r
   return $r
}


proc modbus_oper {} {
    global slaveID
    global opx

    if {$opx(scan)!=1} { return }
    set r1 [::modbus::cmd 3 $slaveID 0 4]
    if {[llength $r1]!=0} {
	foreach q $r1 p {0 1 2 3} { set opx($p) $q }
    }

    set r2 [::modbus::cmd 3 $slaveID 4 6]
    if {[llength $r2]!=0} {
	foreach {l h} $r2 p {4 6 8} { set opx($p) [to_float [list $l $h]] }
    }

    set r3 [::modbus::cmd 3 $slaveID 10 2]
    if {[llength $r3]!=0} {
	foreach q $r3 p {10 11} { set opx($p) $q }
    }

    set r4 [::modbus::cmd 3 $slaveID 12 4]
    if {[llength $r4]!=0} {
	foreach {l h} $r4 p {12 14} { set opx($p) [to_float [list $l $h]] }
    }

    set r5 [::modbus::cmd 3 $slaveID 16 2]
    if {[llength $r5]!=0} {
	foreach {l h} $r5 p {16} { set opx($p) [to_ulong [list $l $h]] }
    }
}

proc modbus_parm {} {
    global slaveID
    global par

    if {$par(scan)!=1} { return }
    set r1 [::modbus::cmd 3 $slaveID 500 1]
    if {[llength $r1]!=0} {
	foreach q $r1 p {500} { set par($p) [format "0x%4.4x" $q] }
    }

    set r2 [::modbus::cmd 3 $slaveID 501 18]
    if {[llength $r2]!=0} {
	foreach {l h} $r2 p {501 503 505 507 509 511 513 515 517} {
		set par($p) [to_float [list $l $h]] 
	}
    }

    set r3 [::modbus::cmd 3 $slaveID 519 10]
    if {[llength $r3]!=0} {
	foreach {l h} $r3 p {519 521 523 525 527} { 
		set par($p) [to_ulong [list $l $h]]
	}
    }

    set r4 [::modbus::cmd 3 $slaveID 529 2]
    if {[llength $r4]!=0} {
	foreach q $r4 p {529 530} { set par($p) $q }
    }
}

proc modbus_adcx {} {
    global slaveID
    global adcx

    if {$adcx(scan)!=1} { return }
    set ch $adcx(chan)
    if {$ch < 0 || $ch>7} { return }

    if {$adcx(cm)==1} {
    set r1 [::modbus::cmd 3 $slaveID [expr 300+$ch] 1]
    if {[llength $r1]!=0} { foreach q $r1 p {0} { 
		set adcx($p) [format "0x%4.4x" $q] 
		} }
    set r2 [::modbus::cmd 3 $slaveID [expr 308+$ch] 1]
    if {[llength $r2]!=0} { foreach q $r2 p {1} {
		set adcx($p) [format "0x%4.4x" $q]
		} }
    set r3 [::modbus::cmd 3 $slaveID [expr 316+$ch] 1]
    if {[llength $r3]!=0} { foreach q $r3 p {2} {
		set adcx($p) [format "0x%4.4x" $q] 
		} }
    set r4 [::modbus::cmd 3 $slaveID [expr 324+$ch] 1]
    if {[llength $r4]!=0} { foreach q $r4 p {3} {
		set adcx($p) [format "0x%4.4x" $q] 
		} }
    }


    if {$adcx(d0)==1} {
    set r5 [::modbus::cmd 3 $slaveID [expr 332+$ch*2] 2]
    if {[llength $r5]!=0} { foreach {l h} $r5 p {4} {
	set adcx($p) [to_ulong [list $l $h]] 
	} }
    }

    if {$adcx(ds)==1} {
    set r6 [::modbus::cmd 3 $slaveID [expr 348+$ch*2] 2]
    if {[llength $r6]!=0} { foreach {l h} $r6 p {5} {
	set adcx($p) [to_ulong [list $l $h]] 
	} }
    }

    if {$adcx(dc)==1} {
    set r7 [::modbus::cmd 3 $slaveID [expr 428+$ch*2] 2]
    if {[llength $r7]!=0} { foreach {l h} $r7 p {6} {
	set adcx($p) [to_float [list $l $h]] 
	} }
    }


    if {$adcx(cal)==1} {
    set r8 [::modbus::cmd 3 $slaveID [expr 364+($ch*4)+(32*$adcx(co))] 2]
    if {[llength $r8]!=0} { foreach {l h} $r8 p {7} {
	set adcx($p) [to_ulong [list $l $h]] 
	} }
    set r9 [::modbus::cmd 3 $slaveID [expr 366+($ch*4)+(32*$adcx(cg))] 2]
    if {[llength $r9]!=0} { foreach {l h} $r9 p {8} {
	set adcx($p) [to_ulong [list $l $h]] 
	} }
    }
}

proc modbustask {} {
    global ConfScan

    modbus_oper
    modbus_parm
    modbus_adcx
    logger

    after $ConfScan modbustask
}

proc co_conn {} {
   global ConfPort
   global ConfMode
   global ConfTo

   ::modbus::configure -mode "RTU" -com $ConfPort \
		-settings $ConfMode -timeout $ConfTo
   ::modbus::port_open
# open "" -handshake none
   set_status "connection ok"
}

proc cmd_conn {} {
   co_conn
   modbustask
#   set_status "connection ok"
   .toolbar.conn configure -text " Close " -command {cmd_discon}
}


proc cmd_discon {} {
   ::modbus::port_close
   set_status "disconnectiod ok"
   .toolbar.conn configure -text "Connect" -command {cmd_conn}
}

proc logger {} {
   global do_log
   global do_log_fn
   global do_log_fields

   if {!$do_log} { return }

   set r [get_clock]
   foreach v $do_log_fields {
	set vg [regsub {\(.*\)$} $v {}]
	global $vg
	lappend r [set $v]
   }
   append_str $do_log_fn "$r"
}

proc cmd_logger {} {
   global do_log
   global do_log_fn
   global do_log_fields

   set p12 [tk_inputer .logrun "Start data logger" \
		{File Fields} [list $do_log_fn $do_log_fields]]
   if { [llength $p12] == 0 } { return }
   foreach {do_log_fn do_log_fields} $p12 {break}

   set_status "saving $do_log_fn \[$do_log_fields]"
   set do_log 1
   tk_messageBox -title "Logger" -type ok -icon info \
	-message "Log selected records\nPress OK when finish"
   
   set do_log 0
}


proc send_modbus {addr fmt data} {
   global slaveID
   set p12 [tk_inputer .parservar "MODBUS send data" \
		{SlaveID Function{6|16} Address Format{s|i|f} Data} \
		[list $slaveID 16 $addr $fmt $data]]
   if { [llength $p12] == 0 } { return }
   foreach {si fu addr fmt xta} $p12 {break}
   switch $fmt {
	s {set data $xta}
        i {set data [long_to $xta]} 
        f {set data [float_to $xta]} 
        default {set st "invalid data type"}
   }
   set c [list $fu $si $addr]
   lappend c {*}$data
   set st [::modbus::cmd {*}$c]
   if {[llength $st]==0} { set st "modbus command error" }
   set_status $st
}

# -- STATUSBAR 

label .statusbar.msg -bd 2 -relief sunken -width 55 -anchor w -text "wait.."
entry .statusbar.msgerr -bd 2 -relief sunken -width 30 -textvariable errcount
pack .statusbar.msg -before .statusbar.fill -side left -expand false -fill x 
pack .statusbar.msgerr -before .statusbar.fill -side left -expand false -fill x
proc set_status {s} { .statusbar.msg configure -text $s }
proc set_errors {} { global errcount; .statusbar.msg configure -text $errcount }

set windowsTID [thread::id]
set modbusTID [thread::create {thread::wait}]

