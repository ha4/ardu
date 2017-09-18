global tfd
proc tclose {} { global tfd; close $tfd; destroy .trm; set tfd "" }
proc tshow {c} { .trm.ttt insert end $c }
proc tpipe {c} { global tfd; puts -nonewline $tfd $c }
proc terminal {line {mode 9600,n,8,1}} {
	global tfd

	toplevel .trm -borderwidth 5
	text .trm.ttt
	pack .trm.ttt

	raise .trm
	focus .trm.ttt

	set tfd [open $line r+]
	fconfigure $tfd -mode $mode -translation binary \
            -buffering none -blocking 0

	wm protocol .trm WM_DELETE_WINDOW {tclose}
	fileevent $tfd readable [list tshow [read $tfd 1]]
	bind .trm.ttt <Key> {tpipe %A; break}

}

