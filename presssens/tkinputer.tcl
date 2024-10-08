proc tk_inputer {w {title "Dialog"} {argnames ""} {argdefaults ""}} {
	variable _par

	toplevel $w -borderwidth 10

	foreach an $argnames ad $argdefaults {
	 set ::_par([incr m]) $ad
	 grid [label $w.pt$m -text $an] [entry $w.p$m -textvariable _par($m)] -\
		-sticky wns -padx 6 -pady 6
	}

	set ::xxxtkinp_res [array names ::_par]
	button $w.ok -text OK -command {set ::xxxtkinp_res $::xxxtkinp_res}
	button $w.cancel -text Cancel -command {set ::xxxtkinp_res {}}
	grid [label $w.dummy -text ""] x x
	grid x  $w.ok $w.cancel -sticky news -padx 6 -pady 6

	wm title $w $title
	wm protocol $w WM_DELETE_WINDOW {set ::xxxtkinp_res {}}
	bind $w <Return> [list $w.ok invoke]
	bind $w <Escape> [list $w.cancel invoke]
	raise $w
	grab set $w
	focus $w.p1

	vwait ::xxxtkinp_res
	destroy $w

	set _xres {}
	foreach m [lsort $::xxxtkinp_res] { lappend _xres $_par($m) }

	unset ::_par
	return $_xres
}


