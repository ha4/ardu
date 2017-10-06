
set port "\\\\.\\com10"
set ai 1
set bi 1
set ci 1
set di 1
set av 255
set bv 255
set cv 255
set dv 255

proc showargs {args} {puts $args}

proc traceHandler {name _ op} {  ;# we ignore name2 for a non-array trace
    global fd
    upvar #0 $name v
    set cmd [string index $name 0]
    upvar #0 "${cmd}i" i
    if {$i} {set t $v} else {set t [expr {255-$v}]}
    puts $fd "${cmd}${t}"
}

frame .t
foreach x {a b c d} {
  entry .t.${x}v -width 4 -textvariable ${x}v
  scale .t.${x} -orient horizontal  -from 0 -to 255  -variable ${x}v
  checkbutton .t.${x}i -text inverse -variable ${x}i
  trace add variable ${x}v {write} traceHandler
}

button .t.ex -text exit -command {exit}
button .t.co -text cons -command {console show}

grid [label .t.al -text "A analog3"] .t.av .t.a .t.ai -sticky news
grid [label .t.bl -text "B digitl4"] .t.bv .t.b .t.bi -sticky news
grid [label .t.cl -text "C analog5"] .t.cv .t.c .t.ci -sticky news
grid [label .t.dl -text "D analog6"] .t.dv .t.d .t.di -sticky news
grid .t.ex .t.co - - 
grid columnconfigure    .t 2 -weight 1

pack .t -expand true -fill both
raise .

set fd [open $port r+]
fconfigure $fd -mode 57600,n,8,1 -translation binary -buffering none -blocking 0
#fileevent $fd readable [list port_in $fd]
