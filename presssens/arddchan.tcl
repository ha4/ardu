
#
# Arduino data
#


namespace eval ::ARDD {
	variable fd
	variable port
	variable mode
	variable rsp
	variable reqstr
	variable fu
	namespace export   channel cmd port pmode popen pclose request 
}

proc lforeach {a l e} {set _ {};foreach $a $l {lappend _ {*}[eval $e]};return $_}

proc ::ARDD::channel { chN funct } {
	variable fd
	variable fu
	variable mode

	set newchan "xtardd.$chN"
	interp alias {} $newchan {} ::ARDD::cmd $chN

	set fd($chN) ""
	set fu($chN) $funct
	set mode($chN) 115200,n,8,1

	return $newchan
}



proc ::ARDD::cmd { chN v args} {
	variable rsp
	variable reqstr

	switch $v {
	setrq { set reqstr($chN) {*}$args}
	req  { request $chN $reqstr($chN)}
	request { request $chN {*}$args}
	port { return [port $chN {*}$args]}
	mode { return [pmode $chN {*}$args]}
	open { popen $chN }
	close { pclose $chN }
	data {  set d  $rsp($chN)
		set rsp($chN) ""
		return $d }
	chN { return $chN }
	}
}

proc ::ARDD::request {chN msg} {
	variable fd
	variable rsp

	set rsp($chN) ""
	if {$fd($chN) == ""} {return}
	puts $fd($chN) $msg
	flush $fd($chN)
}

proc ::ARDD::port_in {chN} {
	variable fd
	variable fu
	variable rsp

        if {[eof $fd($chN)]} { return }

	if {[catch {set rd [read $fd($chN)]} errx]} {
		puts "read error $errx channel $chN"
		return
	}
	foreach ch [split $rd {}] {
		if {$ch != "\x0D" && $ch != "\x0A"} { append rsp($chN) $ch }
		if {$ch == "\x0A" } { $fu($chN) "xtardd.$chN" }
	}
}

proc ::ARDD::port {chN {p ""} } {
	variable fd
	variable port

	if {$p != ""} { 
		set port($chN) $p
        	if {$fd($chN) != ""} {
			pclose $chN
			popen $chN
		}
	}
	return $port($chN)
}

proc ::ARDD::pmode {chN {m ""}} {
	variable fd
	variable mode

	if {$m != ""} { 
		set mode($chN) $m
        		if {$fd($chN) != ""} {
			pclose $chN
			popen $chN
		}
	}
	return $mode($chN)
}

proc ::ARDD::popen {chN} {
	variable fd
	variable port
	variable mode

	if {$fd($chN) != ""} { pclose $chN }

	if {[catch {set fd($chN) [open $port($chN) r+]}]} { return }

	fconfigure $fd($chN) -mode $mode($chN) -translation binary \
            -buffering none -blocking 0

	fileevent $fd($chN) readable [list ::ARDD::port_in $chN]
}

proc ::ARDD::pclose {chN} {
	variable fd

	catch {close $fd($chN)}
	set fd($chN) ""
}

#console show
# pack [label .t -text "MEASURING"]
# proc getx {self} { .t configure -text [$self data] }
## proc getx {self} { puts [$self data] }
# set ch1 [::ARDD::channel 1 getx]
# $ch1 port "\\\\.\\COM10"
#puts "open"
# $ch1 open
# after 1500
#puts "gone"
# $ch1 close
