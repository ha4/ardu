proc tk_inputer {w {title "Dialog"} {argnames ""} {argdefaults ""}} {
    set ::xxxtkinp_res {}

    toplevel $w -borderwidth 10
    wm title $w $title
    wm protocol $w WM_DELETE_WINDOW {set ::xxxtkinp_res {}}

    global _par
    set m 0
    foreach an $argnames ad $argdefaults {
	    incr m
	    set ::_par($m) $ad
	    label $w.pt$m -text $an
	    entry $w.p$m -textvariable _par($m)
	    grid $w.pt$m $w.p$m - -sticky wns -padx 6 -pady 6
    }

    button $w.ok -text OK -command {set ::xxxtkinp_res [array names ::_par]}
    button $w.cancel -text Cancel -command {set ::xxxtkinp_res {}}
    grid [label $w.dummy -text ""] x x
    grid x  $w.ok $w.cancel -sticky news -padx 6 -pady 6

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
    unset ::xxxtkinp_res
    return $_xres
}


