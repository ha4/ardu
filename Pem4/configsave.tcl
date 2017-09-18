proc config_save {filename varlist} {
	set f [open $filename w+]
	foreach v $varlist {
		global $v
		set w [set $v]
		regsub -all {\[} $w {\\\[} w
		regsub -all {\]} $w {\\\]} w
		regsub -all {\\} $w {\\\\} w
		regsub -all {\"} $w {\\\"} w
		regsub -all {\n} $w {\\n} w
		regsub -all {\t} $w {\\t} w
		puts $f "set $v \"$w\""
	}
#	set v data_in
#	puts $f "proc $v \{[info args $v]\} \{ [info body $v]\}"
	close $f
}
