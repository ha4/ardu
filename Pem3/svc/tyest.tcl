#!/bin/sh
# the next line restarts using the correct interpreter \
exec wish "$0" "$@"

source [file dirname [info script]]/tkinputer.tcl

proc hr {wn} {frame $wn -height 2 -borderwidth 2 -relief groove}

frame .t

event add <<SendVal>> <Double-1>
event add <<SendVal>> <Return>


labelframe .t.oper -text "Operational"
labelframe .t.parm -text "Parameters"
labelframe .t.adc  -text "ADC"

proc mbentry {wn descr adr vtype vname} {
  label $wn.l$adr -text $descr
  entry $wn.e$adr -textvariable $vname
  bind $wn.e$adr <<SendVal>> [concat dlg_modbus $adr $vtype \$$vname]
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

bind .t.adc.e1 <<SendVal>> {dlg_modbus [expr 300+$adcx(chan)] s $adcx(0)}
bind .t.adc.e2 <<SendVal>> {dlg_modbus [expr 308+$adcx(chan)] s $adcx(1)}
bind .t.adc.e3 <<SendVal>> {dlg_modbus [expr 316+$adcx(chan)] s $adcx(2)}
bind .t.adc.e4 <<SendVal>> {dlg_modbus [expr 324+$adcx(chan)] s $adcx(3)}
bind .t.adc.e5 <<SendVal>> {dlg_modbus [expr 332+$adcx(chan)*2] i $adcx(4)}
bind .t.adc.e6 <<SendVal>> {dlg_modbus [expr 348+$adcx(chan)*2] i $adcx(5)}
bind .t.adc.e7 <<SendVal>> {dlg_modbus [expr 428+$adcx(chan)*2] f $adcx(6)}
bind .t.adc.e8 <<SendVal>> {
	dlg_modbus [expr 364+$adcx(chan)*4+32*$adcx(co)] i $adcx(7)
}
bind .t.adc.e9 <<SendVal>> {
	dlg_modbus [expr 366+$adcx(chan)*4+32*$adcx(cg)] i $adcx(8)
}

# -- final packing
grid .t.oper .t.parm .t.adc -sticky news  -padx 8 -pady 8
pack .t -side top -fill both -expand false


proc dlg_modbus {addr fmt data} {
   set p12 [tk_inputer .parservar "MODBUS send data" \
		{SlaveID Function{6|16} Address Format{s|i|f} Data} \
		[list 1 16 $addr $fmt $data]]
   if { [llength $p12] == 0 } { return }
}
