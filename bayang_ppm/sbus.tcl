set ConfPort "\\\\\.\\COM19"
set ConfMode "100000,e,8,2"

# -- SERIAL DATA HANDLER

set chan ""
set chanpps "ready"
set chancnt 0
set chanpha 0
set sertime 5
set chans(f) 0
set PortMsg ""

proc sbus_decode0 {msg} {
	global chans
	binary scan $msg {c*} chanx
	if {[llength $chanx] != 23} return
	set rest 8
	set indx 0
	for {set i 1} {$i <= 16} {incr i} {
		set bits 11
		set ch 0
		while {$bits > 0} {
			set j [lindex $chanx $indx]
			set v [expr ($j&0xFF)>>(8-$rest)]
			if {$bits >= $rest} {
				set ch [expr $ch|($v<<(11-$bits))]
				set bits [expr $bits-$rest]
				set rest 8
				incr indx
			} else {
				set vv [expr $v&(0xFF>>(8-$bits))]
				set ch [expr $ch|($vv<<(11-$bits))]
				set rest [expr $rest-$bits]
				set bits 0
			}
		}
		set chans($i) $ch
	}
	set j [lindex $chanx $indx]
	set chans(f) [expr $j&0xFF]
}

proc sbus_decode {msg} {
	global chans
	if {[binary scan $msg b184 x] != 1} return
	set y [regexp -all -inline {.{8,11}} $x]
	for {set i 1} {$i <= 16} {incr i} {
		binary scan [binary format b11 [lindex $y $i-1]] s chans($i)
	}
	binary scan [binary format b11 [lindex $y 16]] s chans(f)
}

proc ser_port_in {} {
	global PortMsg
	global chan
	global chanf
	global chanpha
	global chancnt

	if {[catch {set rd [read $chan 1]}]} return
	binary scan $rd c m
	set m [expr $m&0xFF]
	if {$chanpha==0 && $m==0x0f} {
		incr chanpha
		incr chancnt
		set PortMsg ""
	} elseif {$chanpha<24} {
		append PortMsg $rd
		incr chanpha
	} else {
		set chanpha 0
		sbus_decode $PortMsg
		set PortMsg ""
	}
}

proc ser_start {} {
	global chan
	global ConfPort
	global ConfMode

	set chan [open $ConfPort r+]
	fconfigure $chan -mode $ConfMode -translation binary \
            -buffering none -blocking 0
	fileevent $chan readable ser_port_in

}

proc ser_stop {} {
	global chan

	if {$chan != ""} { close $chan }
        set chan ""
}
                                                                       
proc pps_check {} {
	global chanpps
	global chancnt
	after 1000 pps_check
	set chanpps $chancnt
	set chancnt 0
}

proc frames {} {
	frame .t
	labelframe .t.cfg
	entry .t.cfg.p -textvariable ConfPort
	entry .t.cfg.m -textvariable ConfMode
	button .t.cfg.o -text "Open" -command ser_start
	button .t.cfg.c -text "Close" -command ser_stop
	button .t.cfg.n -text "console" -command {catch {console show}}
	foreach f [list 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16] {
		entry .t.e$f -textvariable chans($f)
	}
	entry .t.flag -textvariable chans(f)
	label .t.lpps -text "pps:"
	entry .t.pps -textvariable chanpps

	grid .t.cfg - - - -stick news
	grid .t.e1 .t.e2 .t.e3 .t.e4  -stick news
	grid .t.e5 .t.e6 .t.e7 .t.e8  -stick news
	grid .t.e9 .t.e10 .t.e11 .t.e12  -stick news
	grid .t.e13 .t.e14 .t.e15 .t.e16 -stick news
	grid .t.flag x .t.lpps .t.pps  -stick news
	pack .t.cfg.p -side left
	pack .t.cfg.m -side left
	pack .t.cfg.o -side left
	pack .t.cfg.c -side left
	pack .t.cfg.n -side left
	pack .t -expand true -fill both
}

frames
pps_check
