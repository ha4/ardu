
#
# bitstream DAQ
#

namespace eval ::BITS {
	variable fd
	variable port
        variable mode
	variable rsp
	variable fu
	variable daqredo

	namespace export channel cmd req popen pclose restart status
# port_cmd -> req, start->popen, stop->pclose
}


proc ::BITS::channel {chN funct} {
	variable fd
	variable fu

	if {[info exists fd($chN)]} {return [list] }

	interp alias {} $chN {} ::BITS::cmd $chN

	set rsp($chN) ""
	set fd($chN) ""
	set fu($chN) $funct

	return $chN
}



proc ::BITS::cmd {chN v args} {
	variable rsp

	switch $v {
	req  { ::BITS::req $chN {*}$args}
	port { ::BITS::port $chN {*}$args}
	open { ::BITS::popen $chN }
	close { ::BITS::pclose $chN }
	restart { ::BITS::restart $chN }
	decode { ::BITS::decode $rsp($chN) }
	rsp  { return $rsp($chN) }
	chN { return $chN }
	status { return [::BITS::status $chN]}
	}
}

proc ::BITS::status {chN} {
	variable fd

	if {![info exists fd($chN)]} {return "deleted"}
	if {$fd($chN) == ""} { return "disconnected" }
	if {[eof $fd($chN)]} { return "error" }
	return "connected"
}


proc ::BITS::req {chN cmd} {
	variable fd
	variable rsp

	if {$fd($chN) == ""} {return}
	puts -nonewline $fd($chN) $cmd
	flush $fd($chN)
}


proc ::BITS::decode {msg} {
	return $msg
}

proc ::BITS::port_in {chN} {
	variable fd
	variable fu
	variable rsp

        if {[eof $fd($chN)]} { return }
	if {[catch {set rd [read $fd($chN)]} errx]} {
	    puts "read error: $errx"
	    return
	}

	watchdog $chN
	foreach ch [split $rd {}] {
		set rsp($chN) $ch
		$fu($chN) $chN
		set rsp($chN) ""
	}
}

proc ::BITS::port {chN p {m "9600,n,8,1"}} {
	variable fd
	variable port
	variable mode

	set port($chN) $p
	set mode($chN) $m

	if {$fd($chN) != ""} {
		pclose $chN
		popen $chN
	}
}

proc ::BITS::popen {chN} {
	variable fd
	variable fu
	variable rsp
	variable port
	variable mode


	if {$fd($chN) != ""} { pclose $chN }

	if {![catch {set fd($chN) [open $port($chN) r+]}]} {
		fconfigure $fd($chN) -mode $mode($chN) -translation binary \
	          -buffering none -blocking 0
		fileevent $fd($chN) readable [list ::BITS::port_in $chN]
	}

	watchdog $chN

# generate first call
	set rsp($chN) ""
	$fu($chN) $chN
}

proc ::BITS::pclose {chN} {
	variable daqredo
	variable fd
	variable fu

	if [info exists daqredo($chN)] { after cancel $daqredo($chN) }

	catch {close $fd($chN)}
	set fd($chN) ""
# last call
	set rsp($chN) ""
	$fu($chN) $chN
}

proc ::BITS::watchdog {chN} {
	variable daqredo

	if [info exists daqredo($chN)] { after cancel $daqredo($chN) }
	set daqredo($chN) [after 2000 [list ::BITS::restart $chN]]
}

proc ::BITS::restart {chN} {
	pclose $chN
	popen $chN
}


