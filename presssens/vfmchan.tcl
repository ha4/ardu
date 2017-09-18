
#
# VFM frequency meter tiny2313
#


namespace eval ::VFM {
	variable fd
	variable port
	variable mode
	variable rsp
	variable reqstr
	variable ststr
	variable fu
	namespace export   channel cmd port pmode popen pclose request decode
}

proc lforeach {a l e} {set _ {};foreach $a $l {lappend _ {*}[eval $e]};return $_}

proc ::VFM::channel { chN funct } {
	variable fd
	variable fu
	variable mode
	variable ststr
	variable rsp
	variable reqstr

	set newchan "xtvfm.$chN"
	interp alias {} $newchan {} ::VFM::cmd $chN

	set fd($chN) ""
	set rsp($chN) ""
	set reqstr($chN) "F"
	set ststr($chN) "F0BjK0"
	set fu($chN) $funct
	set mode($chN) 9600,n,8,1

	return $newchan
}



proc ::VFM::cmd { chN v args} {
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
	decode { decode $chN }
	chN { return $chN }
	}
}

proc ::VFM::request {chN msg} {
	variable fd
	variable rsp

	set rsp($chN) ""
	if {$fd($chN) == ""} {return}
	puts -nonewline $fd($chN) $msg
	flush $fd($chN)
}

proc ::VFM::port_in {chN} {
	variable fd
	variable fu
	variable rsp

        if {[eof $fd($chN)]} { return }

	if {[catch {set rd [read $fd($chN)]} errx]} {
		puts "read error $errx channel $chN"
		return
	}

	foreach ch [split $rd {}] {
		if {$ch != "\x0D" && $ch != "\x0A" && $ch != "\x07"} { append rsp($chN) $ch }
		if {$ch == "\x0D" } {
			$fu($chN) "xtvfm.$chN"
		}
	}
}

proc ::VFM::port {chN {p ""} } {
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

proc ::VFM::pmode {chN {m ""}} {
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

proc ::VFM::popen {chN} {
	variable fd
	variable port
	variable mode

	if {$fd($chN) != ""} { pclose $chN }

	if {[catch {set fd($chN) [open $port($chN) r+]}]} { return }

	fconfigure $fd($chN) -mode $mode($chN) -translation binary \
            -buffering none -blocking 0

	fileevent $fd($chN) readable [list ::VFM::port_in $chN]
}

proc ::VFM::pclose {chN} {
	variable fd

	catch {close $fd($chN)}
	set fd($chN) ""
}

proc ::VFM::a_freq {v lim stf} {
	return [
	  if {[regexp {[Ff]} $stf]} {
		switch -- [string index $stf 1] \
		"0" {expr 1000.0*$v} \
		"1" {expr 100.0*$v} \
		"2" {expr 10.0*$v} \
		"4" {expr 0.1*$v} \
		"3" - default {expr 1.0*$v} 
	  } else {
		expr 0
	  }
	]
}

proc ::VFM::decode {chN} {
	variable reqstr
	variable ststr
	variable rsp

	set msg $rsp($chN)
	set rsp($chN) ""

	# puts "$reqstr($chN) $ststr($chN) $msg"

	if {[regexp {(.\d[Bb][Kk][Jj]\d)$} $msg -> fst] } {
		set ststr($chN) $fst
		if {[string index $ststr($chN) 1] == 3} {
		        set reqstr($chN) D
		} else {
		        set reqstr($chN) C
		}
	} elseif {[regexp {([0-9A-F]{8})$} $msg -> fhex]} {
		return [a_freq 0x$fhex 0xFFFFFFFF $ststr($chN)]
	        set reqstr($chN) D
	} elseif {[regexp {([0-9A-F]{6})$} $msg -> fhex]} {
		return [a_freq 0x$fhex 0x7FFFFF $ststr($chN)]
	        set reqstr($chN) D
	} else {
	        set reqstr($chN) F
	}
	return ""
}

# pack [label .t -text "MEASURING"]
# proc getx {self} { .t configure -text [$self decode]; $self req }
# proc getx {self} { puts [$self decode]; $self req }
# set ch1 [::VFM::channel 1 getx]
# $ch1 port "\\.\\\\COM6"
# $ch1 open            
# $ch1 req
# after 1500
# $ch1 close
# console show
