@echo off

: .tab=4
set path=%path%;%~dp0\

: 引数のファイルをマージするときに merge に jmp
goto merge

for %%A in ( %* ) do (
	set SrcFile="%%~dpnxA"
	set SrcFileOrg="%%~nxA"
	set NMEFile="%%~dpnA.nme"
	set SrcFileMod="%%~dpnA.org_nme"
	set SrcFileMod2="%%~nA.org_nme"
	set SrcFileExt="%%~xA"
	set DstFile="%%~dpnA.kml"
	set DstFileZ="%%~dpnA.kmz"
	
	call :conv
)

goto exit

: ***************************************************************************

:merge

if exist %1\ (
	cd %1
	dir /a/b/s/ogn *.dat *.nme>%temp%\gpslog.lst
) else (
	dir /a/b/s/ogn %*>%temp%\gpslog.lst
)

if exist %temp%\gpslog.nme del %temp%\gpslog.nme
copy /y nul %temp%\gpslog.nme>nul

: dat を nme に変換→cat
for /f "delims=" %%A in ( %temp%\gpslog.lst ) do (
	set SrcFile="%%~dpnxA"
	set SrcFileOrg="%%~nxA"
	set NMEFile="%%~dpnA.nme"
	set SrcFileMod="%%~dpnA.org_nme"
	set SrcFileMod2="%%~nA.org_nme"
	set SrcFileExt="%%~xA"
	set SrcFileExtOrg="%%~xA"
	set DstFile="%%~dpnA.kml"
	set DstFileZ="%%~dpnA.kmz"
	
	call :conv_dat2nme_cat
)

for %%A in ( "%temp%\gpslog.nme" ) do (
	set SrcFile="%%~dpnxA"
	set SrcFileOrg="%%~nxA"
	set NMEFile="%%~dpnA.nme"
	set SrcFileMod="%%~dpnA.org_nme"
	set SrcFileMod2="%%~nA.org_nme"
	set SrcFileExt="%%~xA"
	set DstFile="%%~dpnA.kml"
	set DstFileZ="%%~dpnA.kmz"
	
	if %SrcFileExtOrg%==".dat" (
		call :conv_nme2kmz
	) else (
		call :conv
	)
)

for /f "delims=" %%A in ( %temp%\gpslog.lst ) do (
	set DstFileFin="%%~dpnA.kml"
	goto move
)

:move
move /y %temp%\gpslog.kml %DstFileFin%

del %temp%\gpslog.lst

goto exit

: ***************************************************************************
:conv_dat2nme_cat
if %SrcFileExt%==".dat" gpslogcv -IPSP -oR %SrcFile%>nul 2>&1

copy /y/b %temp%\gpslog.nme+%NMEFile% %temp%\gpslog2.nme>nul
move /y %temp%\gpslog2.nme %temp%\gpslog.nme

if %SrcFileExt%==".dat" del %NMEFile%
goto exit

:conv
: .ext が .nme なら，リネーム
if %SrcFileExt%==".nme" (
	ren %SrcFile% %SrcFileMod2%
	set SrcFile=%SrcFileMod%
)

if %SrcFileExt%==".dat" (
	gpslogcv -IPSP -oR %SrcFile%>nul 2>&1
) else (
	gpslogcv -ITOKYO -INME -TNME -DW -oR %SrcFile%>nul 2>&1
)

:conv_nme2kmz
gpsbabel -p "" -w -i nmea -f %NMEFile% -o kml,labels=0,line_width=6 -F %DstFile%
:gpsbabel -p "" -w -i nmea -f %NMEFile% -o kml,line_width=6 -F %DstFile%
del %NMEFile%

: .ext が .nme なら，リネーム
if %SrcFileExt%==".nme" (
	if exist %SrcFileMod% ren %SrcFileMod% %SrcFileOrg%
)

: zip -q9 %DstFileZ% %DstFile%
: del %DstFile%

:exit
