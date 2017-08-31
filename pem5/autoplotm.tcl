#
# Auto Multi Plot with common axis
#

namespace eval ::AutoPlotM {
# scale  x/y (name, ab amin amax astep vmin vmax)
# scale param(name, fmt offset anchor color gcolor)
# dset    : mix axis and data (dset, xaxis yaxis color)
# pixel=ab[1]*value+ab[0]
  variable scale
  variable pscale
  variable dset
  variable lastpt
  variable wbg
}

proc ::AutoPlotM::masaxis {dmin dmax} {
# MASHTAB AXIS
# Result: List of three elements: low, high, step

# min degree of range
  if {$dmax > $dmin} {
	set ords [expr floor(log10($dmax-$dmin))]
  } elseif {$dmax == $dmin} {
	set ords -1
  } else { return [list] }

# prelim. step degree is less by degree
  set ddd [expr pow(10,$ords-1)]

# calculate max num of entries
  set ordax [expr ceil(($dmax-$dmin)/$ddd)]

  set astp [expr $ddd*(($ordax<10)?1:(($ordax<20)?2:(($ordax<50)?5:10)))]
  set amin [expr $astp * floor($dmin / $astp)]
  set amax [expr $astp *  ceil($dmax / $astp)]

  return [list $amin $amax $astp]
}                                                                               

proc ::AutoPlotM::scaler {dmin dmax pixels} {
  set a [expr ($dmin==$dmax)?$pixels:($pixels/($dmax-$dmin))]
  set b [expr -$dmin*$a - (($pixels<0)?$pixels:0)]
  return [list $b $a]
}                                                                               

proc ::AutoPlotM::rescaler {name new} {
  variable scale

  foreach {nb na} $new {break}
  if {[info exist scale($name,ab)]} {
	foreach {lb la} $scale($name,ab) {break}

	if {$na == $la} { return [list] }
	set sc [expr $na/$la]
	set offs [expr ($sc==1)?0:(($lb*$sc-$nb)/($sc-1.0))]

	set scale($name,ab) [list $nb $na]
	return [list $offs $sc]
  } else {
	set scale($name,ab) [list $nb $na]
	return [list]
  }
}

proc ::AutoPlotM::toPixel {ab coord} {
  foreach {b a} $ab {break}
  return [expr int($a * $coord + $b)]
}

# Draw the line piece
proc ::AutoPlotM::plotXYline {wnd x y dtg} {
  variable dset
  variable scale
  variable lastpt

  set cfil black
  set xn x; if {[info exist dset($dtg,xaxis)]} {set xn $dset($dtg,xaxis)}
  set yn y; if {[info exist dset($dtg,yaxis)]} {set yn $dset($dtg,yaxis)}
  if {![info exist scale($xn,ab)] || ![info exist scale($yn,ab)]} { return }
  set xsc $scale($xn,ab)
  set ysc $scale($yn,ab)

  set px [toPixel $xsc $x]
  set py [toPixel $ysc $y]
  if {[info exists dset($dtg,color)]} {set cfil $dset($dtg,color)}

  if {[info exists lastpt($dtg)] } {
	set pxp [toPixel $xsc [lindex $lastpt($dtg) 0]]
	set pyp [toPixel $ysc [lindex $lastpt($dtg) 1]]
	$wnd create line $pxp $pyp $px $py -fill $cfil -tag $dtg
  }

  set lastpt($dtg) [list $x $y]
}

# Draw the cross point
proc ::AutoPlotM::plotXYdot {wnd x y dtg} {
  variable dset
  variable scale
  variable lastpt

  set cfil black
  set xn x; if {[info exist dset($dtg,xaxis)]} {set xn $dset($dtg,xaxis)}
  set yn y; if {[info exist dset($dtg,yaxis)]} {set yn $dset($dtg,yaxis)}
  set xsc $scale($xn,ab)
  set ysc $scale($yn,ab)

  set px [toPixel $xsc $x]
  set py [toPixel $ysc $y]
  if {[info exists dset($dtg,color)]} {set cfil $dset($dtg,color)}

  set pym [expr $py-1]
  set pyp [expr $py+1]
  set pxm [expr $px-1]
  set pxp [expr $px+1]
  $wnd create line $px $pym $px $pyp -fill $cfil -tag $dtg
  $wnd create line $pxm $py $pxp $py -fill $cfil -tag $dtg

  set lastpt($dtg) [list $x $y]
}

# draw axis/grid, size:in pixels,
# sel: {0 1} for x or {1 2} for y - coordinate selection
proc ::AutoPlotM::DrawAxis {wnd sel size fixed atg} {
  variable scale
  variable pscale

  set v  $scale($atg,amin)
  set out [expr $scale($atg,amax)+0.5*$scale($atg,astep)]
  set ta [list -tag $atg]
  set ga [list -dash {2 2} -tag $atg]
  set ad 0
  if {[info exist scale($atg,ab)] == 0} { return }
  if {[info exist pscale($atg,gcolor)]} {lappend ga -fill $pscale($atg,gcolor)}
  if {[info exist pscale($atg,tcolor)]} {lappend ta -fill $pscale($atg,tcolor)}
  if {[info exist pscale($atg,anchor)]} {lappend ta -anchor $pscale($atg,anchor)}
  if {[info exist pscale($atg,offset)]} {set fixed [expr $fixed+$pscale($atg,offset)]}
  if {[info exist pscale($atg,fmt)]} {set fmt $pscale($atg,fmt)}

  while { $v < $out } {
	set vpix [toPixel $scale($atg,ab) $v]
	foreach {x y} [lrange [list $vpix $fixed $vpix] {*}$sel] {break}
	if {[info exist fmt]} {set atxt [format $fmt $v]} else {set atxt $v}
	set v [expr $v+$scale($atg,astep)]

	$wnd create line {*}[lrange [list $x 0 $y] {*}$sel] \
		{*}[lrange [list $x $size $y] {*}$sel] {*}$ga
	$wnd create text [expr $x + 3] [expr $y - 6] -text $atxt {*}$ta
  }
}

# return list of data sets for axis
proc ::AutoPlotM::getset {axis} {
  variable dset

  if {$axis == "all"} {return [list all]}
  set l [list {0 1}]
  foreach v [array names dset *,xaxis] {
	if {$dset($v)==$axis} { lappend l [lindex [split $v ,] 0] }
  }
  if {[llength $l] > 1} { return $l }
  set l [list {1 2}]
  foreach v [array names dset *,yaxis] {
	if {$dset($v)==$axis} { lappend l [lindex [split $v ,] 0] }
  }
  if {[llength $l] > 1} { return $l }
  return [list]
}

proc ::AutoPlotM::minmax {v vmin vmax chg} {
  upvar $vmin mi
  upvar $vmax ma
  variable scale
  variable dset

  if {[info exist ma]==0 || [info exist mi]==0} {
	set ma [set mi $v]; return true
  }
  if {$v > $ma} {set ma $v; return true }
  if {$v < $mi} {set mi $v; return true }

  return $chg
}


proc ::AutoPlotM::replot { wnd {atg "all"} } {
  variable dset
  variable scale

# total rescale
  if {$atg == "all"} {
    foreach v [array names scale *,ab] { replot $wnd [lindex [split $v ,] 0] }
    return
  }

  if {[info exist scale($atg,vmin)]==0 ||
      [info exist scale($atg,vmax)]==0 } { return }

  foreach {p q r} [masaxis $scale($atg,vmin) $scale($atg,vmax)] {break}
  set scale($atg,amin)  $p
  set scale($atg,amax)  $q
  set scale($atg,astep) $r

  set wheight [winfo height $wnd]
  set wwidth  [winfo width $wnd]

  set typ  [getset $atg]
  set slst [lrange $typ 1 end]
  set typ  [lindex $typ 0]
  if {$typ == ""} { return }

  $wnd delete $atg
  if {[info exist scale($atg,amin)] == 0} { return }

  foreach pix [lrange [list $wwidth [expr -$wheight] 0] {*}$typ] {break}
  set tmp [scaler $scale($atg,amin) $scale($atg,amax) $pix]
  set tmp [rescaler $atg $tmp]

  if {[llength $tmp] == 2} {
	foreach {mov sc} $tmp {break}
	foreach tg $slst {
		$wnd scale $tg {*}[lrange [list $mov 0 $mov] {*}$typ] \
		{*}[lrange [list $sc 1 $sc] {*}$typ]
	}
  }

  if {[info exists scale($atg,ab)]} {
	if {[lindex $typ 0] == 0} {set p [list $wheight  $wheight]}
	if {[lindex $typ 0] == 1} {set p [list $wwidth   0]}
	DrawAxis $wnd $typ {*}$p $atg
  }
  $wnd lower $atg
}


proc ::AutoPlotM::clear { wnd } {
  variable scale
  variable wbg
  variable lastpt

  $wnd delete all
  $wnd configure -background $wbg

  unset -nocomplain lastpt
  unset -nocomplain scale

  bind $wnd <Configure> [list ::AutoPlotM::DoResize $wnd]
}

proc ::AutoPlotM::createaxis {xy {dstg set1} {fmt %g} {anchor w} } {
  variable scale
  variable pscale
  variable dset

  switch [string range $xy 0 0] {
	"x" {set aname xaxis}
	"y" {set aname yaxis}
	default return }
  set dset($dstg,$aname) $xy
  set pscale($xy,fmt) $fmt
  set pscale($xy,anchor) $anchor
  set pscale($xy,tcolor) black
  set pscale($xy,gcolor) darkgray
}

proc ::AutoPlotM::create {wnd} {
  variable scale
  variable dset
  variable wbg

  set newplot "automchart_$wnd"
  interp alias {} $newplot {} ::AutoPlotM::PlotData $wnd

  set wbg    white
  clear $wnd
  createaxis x
  createaxis y
  set dset(set1,color) black

  return $newplot
}

proc ::AutoPlotM::DoResize {wnd} {
  global redo

  if {[info exists redo]} {after cancel $redo}
  set redo [after 50 [list ::AutoPlotM::replot $wnd]]
}

# PlotData -- real data insert/draw
proc ::AutoPlotM::PlotData {wnd xcoord ycoord {dstg set1} {typ line}} {
  variable dset
  variable scale

  set xname x
  set yname y
  if {[info exist dset($dstg,xaxis)]} {set xname $dset($dstg,xaxis)}
  if {[info exist dset($dstg,yaxis)]} {set yname $dset($dstg,yaxis)}

# is any changes in data range? is scale factor changed?
  minmax $xcoord scale($xname,vmin) scale($xname,vmax) false
  set ch [minmax $xcoord scale($xname,amin) scale($xname,amax) false]
  if { $ch } { replot $wnd $xname }

  minmax $ycoord scale($yname,vmin) scale($yname,vmax) false
  set ch [minmax $ycoord scale($yname,amin) scale($yname,amax) false]
  if { $ch } { replot $wnd $yname }

  if { $typ == "line" } { plotXYline $wnd $xcoord $ycoord $dstg
  } else { plotXYdot $wnd $xcoord $ycoord $dstg }
}
