set AVR=C:\MHVAVRTools
rem set AVR=C:\Program Files\Arduino\hardware\tools\avr

PATH %AVR%\utils\bin;%AVR%\bin;%PATH%

cmd /C make %*

