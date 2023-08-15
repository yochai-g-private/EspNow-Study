::%USERPROFILE%\Documents\Arduino\Tests\TestEspAT\TestEspAT\src\MakeLinks.bat
::%USERPROFILE%\Documents\Arduino\Libraries
set ProjectDir=%~dp0
set Inc=%ProjectDir%include\
set Lib=%ProjectDir%lib\

set Libraries=..\..\..\Libraries\

call :LinkIt NYG
call :LinkIt TimeLib
call :LinkIt DS3231

pause

goto :eof
::---------------------------
:LinkIt
::---------------------------
mklink /J %Inc%%1 	                %Libraries%%1
mklink /J %Lib%%1                   %Libraries%%1
goto :eof
