#
# Auto Multi Plot with common axis
#

namespace eval ::AutoPlotM {
# scale  x/y (name, ab amin amax astep vmin vmax)
# scale param(name, fmt offset anchor color gcolor)
# dset    : mix axis and data (dset, xaxis yaxis color)
# pixel=ab[1]*value+ab[0]
# functions create, clear, createaxis, PlotData
	variable scale
	variable pscale
	variable dset
	variable lastpt
}

proc ::AutoPlotM::masaxis {dmin dmax} {
# MASHTAB AXIS
# Result: List of three elements: low, high, step

# min degree of range
	if {$dmax > $dmin} {
		set ords [expr {floor(log10($dmax-$dmin))}]
	} elseif {$dmax == $dmin} {
		set ords -1
	} else {return [list]}

# prelim. step degree is less by degree
	set ddd [expr pow(10,$ords-1)]

# calculate max num of entries
	set ord [expr {ceil(($dmax-$dmin)/$ddd)}]

	set astp [expr {$ddd*(($ord<10)?1:(($ord<20)?2:(($ord<50)?5:10)))}]
	set amin [expr {$astp * floor($dmin / $astp)}]
	set amax [expr {$astp *  ceil($dmax / $astp)}]

	return [list $amin $amax $astp]
}                                                                               

proc ::AutoPlotM::scaler {dmin dmax pixels} {
	set a [expr {($dmin==$dmax)?$pixels:($pixels/($dmax-$dmin))}]
	set b [expr {-$dmin*$a - (($pixels<0)?$pixels:0)}]
	return [list $b $a]
}                                                                               

proc ::AutoPlotM::rescaler {name new} {
	variable scale

	foreach {nb na} $new break
	set rc {0 1}
	if [info exist scale($name,ab)] {
	 foreach {b a} $scale($name,ab) break

	 set s [expr {$na/$a}]
	 set o [if {$s==1} {expr {$b-$nb}} else {expr {($b*$s-$nb)/($s-1.0)}}]

	 set rc [list $o $s]
	}
	set scale($name,ab) [list $nb $na]
	return $rc
}

proc ::AutoPlotM::pix {ab coord} {
	foreach {b a} $ab break
	expr {int($a*$coord+$b)}
}

proc ::AutoPlotM::vproc {v body} {upvar $v _
	if [info exist _] {uplevel 1 "set _ $_;" $body}}

# draw axis/grid, size:in pixels,
# sel: {0 1} for x or {1 2} for y - coordinate selection
proc ::AutoPlotM::DrawAxis {wnd sel size fixed atg} {
	variable scale
	variable pscale

	set ta [list -tag $atg]
	set ga [concat -dash {{2 2}} $ta]
	foreach a {g t} {vproc pscale($atg,${a}color) {lappend ${a}a -fill $_}}
	vproc pscale($atg,anchor) {lappend ta -anchor $_}
	vproc pscale($atg,offset) {set fixed [expr {$fixed+$_}]}
	vproc pscale($atg,fmt) {set fmt $_}

	set i $scale($atg,astep)
	set out [expr {$scale($atg,amax)+0.5*$i}]
	for {set v $scale($atg,amin)} {$v < $out} {set v [expr {$v+$i}]} {
		set vpix [pix $scale($atg,ab) $v]
		foreach {x y} [lrange [list $vpix $fixed $vpix] {*}$sel] break
		set atxt [if [info exist fmt] {format $fmt $v} else {set v}]

		$wnd create line {*}[lrange [list $x 0 $y] {*}$sel] \
			{*}[lrange [list $x $size $y] {*}$sel] {*}$ga
		$wnd create text [expr {$x+3}] [expr {$y-3}] -text $atxt {*}$ta
	}
}

proc ::AutoPlotM::nfirst {i} {lindex [split $i ,] 0}

# return list of data sets for axis
proc ::AutoPlotM::getset {a t} {
	variable dset

	set l {}
	foreach v [array names dset $t] {if {$dset($v) eq $a} {
		lappend l [nfirst $v]}}
	return $l
}

proc ::AutoPlotM::replot {wnd {atg "all"}} {
	variable dset
	variable scale

# total rescale
	if {$atg eq "all"} {
		foreach v [array names scale *,ab] {replot $wnd [nfirst $v]}
		return
	}

	if {![info exist scale($atg,vmin)] || \
		![info exist scale($atg,vmax)] } return

	$wnd delete $atg

	foreach {p q r} [masaxis $scale($atg,vmin) $scale($atg,vmax)] break
	array set scale [list $atg,amin $p $atg,amax $q $atg,astep $r]
	if {![info exist scale($atg,amin)]} return

	if [llength [set slst [getset $atg *,xaxis]]] {set ta {0 1}
	} elseif {[llength [set slst [getset $atg *,yaxis]]]} {set ta {1 2}
	} else return

	set wh [winfo height $wnd]
	set ww  [winfo width $wnd]
	set pix [if [lindex $ta 0] {expr {-$wh}} else {set ww}]
	set tmp [scaler $scale($atg,amin) $scale($atg,amax) $pix]
	puts "ba: $tmp"

	foreach {mov sc} [rescaler $atg $tmp] break
	puts "move: $mov scale: $sc"
	foreach tg $slst {$wnd scale $tg {*}[lrange [list $mov 0 $mov] {*}$ta]\
		{*}[lrange [list $sc 1 $sc] {*}$ta]}

	set p [if [lindex $ta 0] {list $ww 0} else {list $wh $wh}]
	DrawAxis $wnd $ta {*}$p $atg
	$wnd lower $atg
}


proc ::AutoPlotM::clear {wnd} {
	variable scale
	variable lastpt

	$wnd delete all

	unset -nocomplain lastpt
	unset -nocomplain scale

	bind $wnd <Configure> {::AutoPlotM::DoResize %W}
}

proc ::AutoPlotM::createaxis {xy {dstg set1} {fmt %g} {anchor w}} {
	variable pscale
	variable dset

	switch [string range $xy 0 0] {"x" {set a xaxis} "y" {set a yaxis} \
		default return}
	set dset($dstg,$a) $xy
	array set pscale [list $xy,fmt $fmt  $xy,anchor $anchor \
		$xy,tcolor black   $xy,gcolor darkgray]
}

proc ::AutoPlotM::create {wnd} {
	variable scale
	variable dset

	set newplot "automchart_$wnd"
	interp alias {} $newplot {} ::AutoPlotM::PlotData $wnd

	clear $wnd
	createaxis x set1 %g s 
	createaxis y

	return $newplot
}

proc ::AutoPlotM::DoResize {wnd} {
	global redo

	if [info exists redo($wnd)] {after cancel $redo($wnd)}
	set redo($wnd) [after 50 [list ::AutoPlotM::replot $wnd]]
}

# Draw the line piece
proc ::AutoPlotM::plotXYline {wnd x y tg c xsc ysc} {
	variable lastpt

	if {![info exists lastpt($tg)]} return

	set xp [pix $xsc [lindex $lastpt($tg) 0]]
	set yp [pix $ysc [lindex $lastpt($tg) 1]]
	$wnd create line $xp $yp [pix $xsc $x] [pix $ysc $y] -fill $c -tag $tg
}

# Draw the cross point
proc ::AutoPlotM::plotXYdot {wnd x y tg c xsc ysc} {
	set px [pix $xsc $x]
	set py [pix $ysc $y]

	$wnd create line $px [expr $py-1] $px [expr $py+1] -fill $c -tag $tg
	$wnd create line [expr $px-1] $py [expr $px+1] $py -fill $c -tag $tg
}

proc ::AutoPlotM::minmax {v vmin vmax} {upvar $vmin mi $vmax ma
	if {![info exist ma] || ![info exist mi]} {set ma [set mi $v]
		return true}
	if {$v > $ma} {set ma $v; return true }
	if {$v < $mi} {set mi $v; return true }
	return false
}

proc ::AutoPlotM::qset {vn vv} {upvar $vn v
	if [info exist v] {set v} else {set v $vv}}
 
# PlotData -- real data insert/draw
proc ::AutoPlotM::PlotData {wnd x y {dstg set1} {typ line}} {
	variable dset
	variable scale
	variable lastpt

	set xn [qset dset($dstg,xaxis) x]
	set yn [qset dset($dstg,yaxis) y]

# is any changes in data range? is scale factor changed?
	minmax $x scale($xn,vmin) scale($xn,vmax)
	if { $scale($xn,vmin) < ($scale($xn,vmax) - 300) } { 
		set scale($xn,vmin) [expr ($scale($xn,vmax) - 150)]
		set scale($xn,vmax) [expr ($scale($xn,vmax) + 150)]
	}
	if [minmax $x scale($xn,amin) scale($xn,amax)] {replot $wnd $xn}

	minmax $y scale($yn,vmin) scale($yn,vmax)
	if [minmax $y scale($yn,amin) scale($yn,amax)] {replot $wnd $yn}

	set c [qset dset($dstg,color) black]

	if {[info exist scale($xn,ab)] && [info exist scale($yn,ab)]} {
		plotXY$typ $wnd $x $y $dstg $c $scale($xn,ab) $scale($yn,ab)}
	set lastpt($dstg) [list $x $y]
}
