::%USERPROFILE%\Documents\Arduino\Tests\TestEspAT\TestEspAT\src\MakeLinks.bat
::%USERPROFILE%\Documents\Arduino\Libraries
set ProjectDir=%~dp0
set Inc=%ProjectDir%include\
set Lib=%ProjectDir%lib\

set Libraries=..\..\..\Libraries\

call :LinkIt NYG
call :LinkIt IRRemote
call :LinkIt LiquidCrystal_I2C
call :LinkIt TimeLib

pause

goto :eof
::---------------------------
:LinkIt
::---------------------------
mklink /J %Inc%%1 	                %Libraries%%1
mklink /J %Lib%%1                   %Libraries%%1
goto :eof
