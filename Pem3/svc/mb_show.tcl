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

array set grp1 {0 {s "controller on"}
1 {s "samples n-cal"}
2 {s "error count"}
3 {s "ADC FSM state"}
4 {f "PI PWM signal"}
6 {f "PI integral"}
8 {f "PI delta-t"}
10 {s "datawait"}
11 {s "dataready"}
12 {f "Temperature"}
14 {f "PEM current"}
16 {i "milliseconds"}
}

array set grp2 {500 {s "crc"}
501 {f "Regulator T\[deg]"}
503 {f "Span T\[deg]"}
505 {f "Itegrate T\[s]"}
507 {f "Tcorr+"}
509 {f "Tcorr*"}
511 {f "Tconv_mult"}
513 {f "Tconv_base"}
515 {f "PEM Amp offs"}
517 {f "PEM Amp trans"}
519 {i "ZS1"}
521 {i "FS1"}
523 {i "ZS2"}
525 {i "FS2"}
527 {i "Baud"}
529 {s "Format"}
530 {s "N-cal"}
}

array set grp3 {
-9 {- "*channel"
      {spinbox $we -from 0 -to 7 -incr 1 -width 2 -textvariable adcx(chan)} }
300 {s "CMM"}
308 {s "MODE"  {checkbutton $we -variable adcx(xmode)} }
316 {s "FILTH" {checkbutton $we -variable adcx(xfilh)} }
324 {s "FILTL" {checkbutton $we -variable adcx(xfill)} }
332 {i "DATA"       {checkbutton $we -variable adcx(xdata) }  }
348 {i "DATA/fsync" {checkbutton $we -variable adcx(xdsync)}  }
364 {i "OFFSET"     {checkbutton $we -variable adcx(xoffs) }  }
366 {i "GAIN"       {checkbutton $we -variable adcx(xgain) }  }
396 {i "OFFSET/cal" {checkbutton $we -variable adcx(xcoffs)}  }
398 {i "GAIN/cal"   {checkbutton $we -variable adcx(xcgain)}  }
428 {f "DATA/conv"  {checkbutton $we -variable adcx(xdcvt) }  }
}

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


proc mbentry {wn descr adr vtype vname opt} {
  label $wn.l$adr -text $descr
  entry $wn.e$adr -textvariable $vname
  bind $wn.e$adr <<SendVal>> [concat send_modbus $adr $vtype \$$vname]
  set we $wn.eo$adr
  if {$opt != ""} {
	  eval $opt
	  return [list $we $wn.l$adr $wn.e$adr]
  } else {
	  return [list x $wn.l$adr $wn.e$adr]
  }
}

proc mbvar {wn descr iname vname opt} {
  set we $wn.ev$iname
  label $wn.lv$iname -text $descr
  if {$opt != ""} { eval $opt } else { entry $we -textvariable $vname }
  return [list x $wn.lv$iname $we]
}

proc mbgroup {wn grpdsc grplst vn} {
  upvar $vn mass
  upvar $grplst va

  labelframe $wn -text $grpdsc

  set prevt ""
  set lhc "0"
  grid [checkbutton $wn.lsc -text "scan" -variable [set vn](scan)] - x
  foreach a [lsort -integer [array names va]] {
	if {$a < 0} {
		foreach {xvar str xopt} $va($a) {break}
		grid {*}[mbvar $wn $str $lhc [set xvar] $xopt]
		incr lhc
		continue
	}
	foreach {vtyp str opt} $va($a) {break}
        if {$prevt !="" && $prevt != $vtyp} {
		grid [hr $wn.lh$lhc] - -pady 6 -padx 6 -sticky we
		incr lhc
	}
	grid {*}[mbentry $wn $str $a $vtyp [set vn]($a) $opt] -padx 6
	set prevt $vtyp
  }
}

set opx(scan) 1
mbgroup .t.oper "Operational" grp1 opx

set par(scan) 0
mbgroup .t.parm "Parameters" grp2 par

set adcx(chan) 0
set adcx(scan) 0
mbgroup .t.adc "ADC" grp3 adcx

# -- final packing
grid .t.oper .t.parm .t.adc -sticky news  -padx 8 -pady 8
pack .t -side top -fill both -expand true


# --------------------
# -- PROGRAM COMMANDS

proc cmd_cons {} { catch {console show} }

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

# -- modbus convert, fmt: i, iu, f, s2
proc cvt_to {i ifmt ofmt} {
   if { [catch {set ss [binary format $ifmt $i]}] == 1 } { return [list] }
   binary scan $ss $ofmt r
   return $r
}

proc grp_addr {grplst} {
  upvar $grplst va

  set l0 {}
  set l1 {}
  set prevt ""
  foreach a [lsort -integer [array names va]] {
	if {$a < 0} { continue }
	foreach {vtyp str} $va($a) {break}
        if {$prevt !="" && $prevt != $vtyp} {
		lappend l0 $l1
		set l1 {}
	}
	lappend l1 $a
	if {$vtyp=="f" || $vtyp=="i" || $vtyp=="iu"} { lappend l1 [expr $a+1] }
	set prevt $vtyp
  }
  if {[llength $l1]!=0} {lappend l0 $l1}
  return $l0
}

proc grp_aggregate {lst} {
  set l0 {}
  set preva ""
  set a1 ""
  foreach a [lsort -integer $lst] {
	if {$a1 == ""} {
	set a1 $a
	set al 0
	}
        if {$preva !="" && [expr $preva+1] != $a} {
		lappend l0 [list $a1 $al]
		set a1 $a
		set al 0
	}
	incr al
	set preva $a
  }
  if {$al!=0} {lappend l0 [list $a1 $al]}
  return $l0
}

proc grp_all {} {
	global grp1
	global grp2
	global grp3
	set l2 {}
	foreach ga [concat [grp_addr grp1] [grp_addr grp2] [grp_addr grp3]] {
		set l2 [concat $l2 [grp_aggregate $ga]]
	}
	return $l2
}

proc modbus_in_grp {addr dat grp dvar} {
	upvar $grp va
	upvar $dvar mass
	global mbin_tmp
	global mbin_atmp

	if {[info exist va($addr)]} {
		foreach {a i} $va($addr) {break}
		if {[lindex $i 0]=="s"} {
			set mass($addr) $dat
		} else {
			set mbin_tmp $dat
			set mbin_atmp $addr
		}
		return 1
	} elseif {[info exist va([expr $addr-1])]} {
		set pa [expr $addr-1]
		foreach {a i} $va($pa) {break}
		set ta [lindex $i 0]
		if {$ta==s} { return 0 }
		if {$pa!=$mbin_atmp} { return 0 }
		set mass($addr) [cvt_tp [list $mbin_tmp] s2 $ta]
	} else { return 0 }
}

proc modbus_in {addr dat} {
	foreach {g v} {grp1 opx grp2 parm grp3 adcx} {
		if {[modbus_in_grp $addr $dat $g $v]} {continue}
	}
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
	foreach {l h} $r2 p {4 6 8} { set opx($p) [cvt_to [list $l $h] s2 f] }
    }

    set r3 [::modbus::cmd 3 $slaveID 10 2]
    if {[llength $r3]!=0} {
	foreach q $r3 p {10 11} { set opx($p) $q }
    }

    set r4 [::modbus::cmd 3 $slaveID 12 4]
    if {[llength $r4]!=0} {
	foreach {l h} $r4 p {12 14} { set opx($p) [cvt_to [list $l $h] s2 f] }
    }

    set r5 [::modbus::cmd 3 $slaveID 16 2]
    if {[llength $r5]!=0} {
	foreach {l h} $r5 p {16} { set opx($p) [cvt_to [list $l $h] s2 i] }
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
		set par($p) [cvt_to [list $l $h] s2 f] 
	}
    }

    set r3 [::modbus::cmd 3 $slaveID 519 10]
    if {[llength $r3]!=0} {
	foreach {l h} $r3 p {519 521 523 525 527} { 
		set par($p) [cvt_to [list $l $h] s2 i]
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

    set r1 [::modbus::cmd 3 $slaveID [expr 300+$ch] 1]
    if {[llength $r1]!=0} { foreach q $r1 p {300} { 
		set adcx($p) [format "0x%4.4x" $q] 
		} }

    if {$adcx(xmode)==1} {
    set r2 [::modbus::cmd 3 $slaveID [expr 308+$ch] 1]
    if {[llength $r2]!=0} { foreach q $r2 p {308} {
		set adcx($p) [format "0x%4.4x" $q]
		} }
    }
    if {$adcx(xfilh)==1} {
    set r3 [::modbus::cmd 3 $slaveID [expr 316+$ch] 1]
    if {[llength $r3]!=0} { foreach q $r3 p {316} {
		set adcx($p) [format "0x%4.4x" $q] 
		} }
    }
    if {$adcx(xfill)==1} {
    set r4 [::modbus::cmd 3 $slaveID [expr 324+$ch] 1]
    if {[llength $r4]!=0} { foreach q $r4 p {324} {
		set adcx($p) [format "0x%4.4x" $q] 
		} }
    }


    if {$adcx(xdata)==1} {
    set r5 [::modbus::cmd 3 $slaveID [expr 332+$ch*2] 2]
    if {[llength $r5]!=0} { foreach {l h} $r5 p {332} {
	set adcx($p) [cvt_to [list $l $h] s2 i]
	} }
    }

    if {$adcx(xdsync)==1} {
    set r6 [::modbus::cmd 3 $slaveID [expr 348+$ch*2] 2]
    if {[llength $r6]!=0} { foreach {l h} $r6 p {348} {
	set adcx($p) [cvt_to [list $l $h] s2 iu]
	} }
    }

    if {$adcx(xoffs)==1} {
    set r8 [::modbus::cmd 3 $slaveID [expr 364+($ch*4)] 2]
    if {[llength $r8]!=0} { foreach {l h} $r8 p {364} {
	set adcx($p) [cvt_to [list $l $h] s2 iu]
	} }
    }
    if {$adcx(xgain)==1} {
    set r9 [::modbus::cmd 3 $slaveID [expr 366+($ch*4)] 2]
    if {[llength $r9]!=0} { foreach {l h} $r9 p {366} {
	set adcx($p) [cvt_to [list $l $h] s2 iu]
	} }
    }
    if {$adcx(xcoffs)==1} {
    set r8 [::modbus::cmd 3 $slaveID [expr 396+($ch*4)] 2]
    if {[llength $r8]!=0} { foreach {l h} $r8 p {396} {
	set adcx($p) [cvt_to [list $l $h] s2 iu]
	} }
    }
    if {$adcx(xcgain)==1} {
    set r9 [::modbus::cmd 3 $slaveID [expr 398+($ch*4)] 2]
    if {[llength $r9]!=0} { foreach {l h} $r9 p {398} {
	set adcx($p) [cvt_to [list $l $h] s2 iu]
	} }
    }

    if {$adcx(xdcvt)==1} {
    set r7 [::modbus::cmd 3 $slaveID [expr 428+$ch*2] 2]
    if {[llength $r7]!=0} { foreach {l h} $r7 p {428} {
	set adcx($p) [cvt_to [list $l $h] s2 f]
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
        i {set data [cvt_to $xta i s2]} 
        f {set data [cvt_to $xta f s2]} 
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

