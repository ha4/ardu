
# -- PARAMETERS variables
set config_port "/dev/ttyUSB0"
if {$tcl_platform(platform) == "windows" } { set config_port "\\\\.\\COM3" } 

set config_logfile "dump.bin"
set config_mode "115200,n,8,1"
set config_bitrate 0.000050
set config_file ".bitscope.conf"
set config_vars {config_port config_mode config_logfile config_bitrate}

global chart
global dumpfl

