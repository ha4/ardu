# --------------------
# --- WINDOWS

proc hr {w} {frame $w -height 2 -borderwidth 1 -relief sunken}
proc lb {w t} {global llc; label $w.l[incr llc] -an e -text $t}

proc frames {} {
global chart
global sysbg
wm protocol . WM_DELETE_WINDOW [list .mbar.bs invoke Exit]

# menu part
menu  .mbar
. configure -menu .mbar

.mbar add cascade -menu [menu .mbar.bs -tearoff 0] -label BinScope -underline 0
.mbar add cascade -menu [menu .mbar.fl -tearoff 0] -label File -underline 0
.mbar add cascade -menu [menu .mbar.dt -tearoff 0] -label Data -underline 0
.mbar.bs add command -label Console -command cmd_cons
.mbar.bs add command -label Setup -command cmd_setup
.mbar.bs add command -label About -command cmd_about
.mbar.bs add separator
.mbar.bs add command -label Exit -command cmd_exit
.mbar.fl add command -label "Save as.." -command cmd_fsel
.mbar.fl add command -label "Record" -command cmd_open
.mbar.fl add command -label "Stop Recording" -command cmd_close
.mbar.fl add command -label "Replay File.." -command cmd_fread

.mbar.dt add command -label Clear -command cmd_clear
.mbar.dt add command -label Connect -command cmd_conn
.mbar.dt add command -label Disconnect -command cmd_dconn

# .c is beige ivory {floral white} seashell black
canvas .c -relief sunken -bg ivory -borderwidth 1 -xscrollcommand {.xscroll set} -xscrollincrement 1
scrollbar .xscroll -orient horizontal -command {.c xview}
frame .toolbarl -bd 2 -relief flat

# image create photo img -file "exit.png"
# button .toolbar.exitButton -image img -relief flat -command {cmd_exit}

entry .toolbarl.port  -relief sunken -textvariable config_port -width 20
entry .toolbarl.stat  -relief sunken -width 20
entry .toolbarl.file  -relief sunken -textvariable config_logfile -width 20
foreach q {port stat} {pack .toolbarl.$q -side left -padx 2}
pack .toolbarl.file -side left -padx 2 -expand 1 -fill x

pack .c -side top -fill both -expand true
pack .xscroll -fill x -expand false
pack .toolbarl -fill x -expand false

set chart [::AutoPlotM::create .c]
array set ::AutoPlotM::dset {setnone,color black   setsrccal,color darkred \
  setsrcin,color darkblue   setsrcout,color darkgreen  setsrcd,color darkgrey}

set sysbg [.toolbarl.port cget -bg]
}

# --------------------

proc showstatus {s} {global sysbg
  .toolbarl.port configure -bg [if {$s ne "connected"} {list pink} else {set sysbg}]
}

