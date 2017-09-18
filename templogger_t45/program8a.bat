rem DEFAULT: lfuse:62 hfuse:DF efuse:FF {CKDIV8=0 CKOUT=1 SUT=10 CKSEL=0010} {RSTDIS=1 DWEN=1 SPIEN=0 WDT=1 EESAV=1 BODLV=111}
rem DESIRED: lfuse:E2 hfuse:DF efuse:FF {CDKIV8=1 CKOUT=1 SUT=10 CKSEL=0010}

SET dudePath=C:\Program Files (x86)\Arduino\hardware\tools\avr\bin
set AVRDUDE="%dudePath%\avrdude.exe" -C "%dudePath%\..\etc\avrdude.conf"

rem %AVRDUDE% -c usbtiny -p t45 -u -U lfuse:r:-:i -U hfuse:r:-:i -U efuse:r:-:i
rem %AVRDUDE% -c usbtiny -p t85 -u -U lfuse:r:-:i -U hfuse:r:-:i -U efuse:r:-:i
rem %AVRDUDE% -c usbtiny -p t45 -u -U lfuse:w:0xE2:m
rem %AVRDUDE% -c usbtiny -p t85 -u -U lfuse:w:0xE2:m
rem %AVRDUDE% -c usbtiny -p t45 -e -u -U flash:w:tlog.hex:i

