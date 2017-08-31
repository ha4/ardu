#!/bin/sh
# the next line restarts using the correct interpreter \
exec wish "$0" "$@"

set bsdir [file dirname [info script]]
source $bsdir/bitstream.tcl
source $bsdir/autoplotm.tcl
source $bsdir/visual.tcl
source $bsdir/commands.tcl
source $bsdir/defaults.tcl
catch {source $config_file}
source $bsdir/configsave.tcl
source $bsdir/tkinputer.tcl

proc bitplot {bit} {
	global config_bitrate
	global chart
	global Ntime
	global lastbit

	if {$lastbit != $bit} {$chart $Ntime $bit} 
	set Ntime [expr {$Ntime+$config_bitrate}]
	$chart  $Ntime $bit
	set lastbit $bit
}


proc data_get {self} { 
	global Ntime
	global lastbit

	if [info exists Ntime] {} else {set Ntime 0.0}         	                                              
	if {![info exists lastbit]} {set lastbit 0}

	set b [$self decode]
	if [binary scan $b B* x] {} else {
		showstatus [$self status]
		return
	}
	foreach bit [split $x {}] {bitplot $bit}
}

proc tstscr {} {
	global chart
	global gstime
	after 1 tstscr
#	for {set z 0} {$z < 20} {incr z} {
		$chart [incr gstime] [expr sin($gstime)]
#	}
}

::BITS::channel chb data_get
chb port $config_port $config_mode
frames
raise .
catch {console hide}
catch {chb open}
after 200 tstscr
