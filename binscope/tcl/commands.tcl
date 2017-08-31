# --------------------
# -- PROGRAM COMMANDS

proc cmd_about {} {
	tk_messageBox -message "Binary Oscilloscope\n\
	"date 24.07.2017" -type ok -title "BinScope"
}

proc cmd_cons {} {
	catch {console show}
}

proc cmd_exit {} {
	global config_file config_vars

	set retc [tk_messageBox -message "Really exit?" \
		-type yesno -icon warning -title "BinScope Exit"]

	if {$retc == yes} {
		config_save $config_file $config_vars
		chb close
		exit
	}
}

proc cmd_clear {} {
	global Ntime
	global chart

	::AutoPlotM::clear .c
	unset -nocomplain Ntime
	$chart 0 -1.0
	$chart 0 2.0
}

proc cmd_close {} {
	global  dumpfl

	catch {set _ $dumpfl
	unset -nocomplain dumpfl
	close $_}

}

proc cmd_open {} {
	global  dumpfl
	global  config_logfile

	if [info exist dumpfl] {
		cmd_close
		return
	}

	if [file exist $config_logfile] {
		set retc [tk_messageBox -message "File EXIST.\n Overwrite??" \
			-type yesno -icon warning -title "Data File Overwrite"]
	if {$retc ne "yes"} return
	}

	set dumpfl [open $config_logfile w+]
}

proc cmd_conn {} {
	global config_port
	global config_mode

	chb port $config_port $config_mode
	chb restart
}

proc cmd_dconn {} {
	chb close
}

proc cmd_fsel {} {
	global config_logfile
	set types {{"Data Files" .bin} {"All Files" *}}

	set filename [tk_getSaveFile -filetypes $types \
		-defaultextension {.bin}]

	if {$filename ne ""} {set config_logfile $filename}
}

proc cmd_fread {} {
	global config_tplot
	set types {{"Data Files" .bin} {"All Files" *}}

	set filename [tk_getOpenFile -filetypes $types \
		-defaultextension {.bin}]

	if {$filename eq ""} return

	cmd_clear
}

proc cmd_setup {} {
	global config_port config_mode config_bitrate

	set par [list $config_port $config_mode $config_bitrate]
	set names [list \
		{Serial port}\
		{Serial mode}\
		{birate,sec}\
	]

	set res [tk_inputer .setdia "BinScope Setup" $names $par]
	foreach {config_port config_mode config_bitrate} $res break
}


