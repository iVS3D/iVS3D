echo off
set buildDir=%1
cd %buildDir%\src
for /d %%G in (*) do (
	set "dontDel="
	if "%%G" equ "plugins" set "dontDel=True"
	if "%%G" equ "iVS3D-core" set "dontDel=True"
	if not defined dontDel rmdir /s /q %%~fG 
)
for /r %%F in ("iVS3D-core\release\*.*") do move %%F %%~pF../../..
for /d %%H in ("iVS3D-core\release\*") do move %%H %%~pH../../..
move %buildDir%/src/plugins %buildDir%/plugins
cd ..
rmdir /s /q src
del Makefile
del .qmake.stash