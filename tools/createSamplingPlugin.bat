@echo off
set NAME=%1

if [%NAME%]==[] (
    echo Wrong use!
    goto end
)

set MODULE_PATH=%~dp0\..\iVS3D\src\iVS3D-%NAME%Plugin
if exist %MODULE_PATH%\ (
    echo A plugin with this name already exists: '%MODULE_PATH%'!
    echo Use a unique name!
    goto end
)

echo Creating new plugin '%NAME%' in submodule '%MODULE_PATH%'

:: Get name as upper and lower case
set NAME_UPPER=%NAME%
call :TOUPPERCASE NAME_UPPER

type nul > "%NAME%"
dir /l /b "%NAME%" > "%NAME%"
set /p NAME_LOWER= < "%NAME%"
del "%NAME%"

mkdir %MODULE_PATH%

:: Create CMAKE-file:
echo Creating cmake-file
set TEMPLATE_FILE_CMAKE=%~dp0\templates\CMakeLists.txt.template
:: Create new empty file
type nul > "%MODULE_PATH%/CMakeLists.txt"

:: iterate template line by line
SETLOCAL DisableDelayedExpansion
FOR /F "usebackq delims=" %%a in (`"findstr /n ^^ %TEMPLATE_FILE_CMAKE%"`) do (
    set "var=%%a"
    SETLOCAL EnableDelayedExpansion
    :: replace <-NAME->, <-NAME_LOWER-> and format
    set "var=!var:<-NAME->=%NAME%!"
    set "var=!var:<-NAME_LOWER->=%NAME_LOWER%!"
    set "var=!var:*:=!"
    :: write updated line to pro file
    if [!var!]==[] (echo: >> "%MODULE_PATH%/CMakeLists.txt") else (echo !var! >> "%MODULE_PATH%/CMakeLists.txt")
    ENDLOCAL
)

:: Create QRC-file:
echo Creating resources.qrc-file
set TEMPLATE_FILE_QRC=%~dp0\templates\resources.qrc.template
:: Create new empty file
type nul > "%MODULE_PATH%/resources.qrc"

:: iterate template line by line
SETLOCAL DisableDelayedExpansion
FOR /F "usebackq delims=" %%a in (`"findstr /n ^^ %TEMPLATE_FILE_QRC%"`) do (
    set "var=%%a"
    SETLOCAL EnableDelayedExpansion
    :: replace <-NAME->, <-NAME_LOWER-> and format
    set "var=!var:<-NAME->=%NAME%!"
    set "var=!var:<-NAME_LOWER->=%NAME_LOWER%!"
    set "var=!var:*:=!"
    :: write updated line to pro file
    if [!var!]==[] (echo: >> "%MODULE_PATH%/resources.qrc") else (echo !var! >> "%MODULE_PATH%/resources.qrc")
    ENDLOCAL
)

:: Create translations-folder:
echo Creating translations-folder
mkdir %MODULE_PATH%\translations

:: Create translations-DE:
echo Creating translations-DE
set TEMPLATE_FILE_DE=%~dp0\templates\translations_de.ts.template
:: Create new empty file
type nul > "%MODULE_PATH%\translations\%NAME_LOWER%_de.ts"

:: iterate template line by line
SETLOCAL DisableDelayedExpansion
FOR /F "usebackq delims=" %%a in (`"findstr /n ^^ %TEMPLATE_FILE_DE%"`) do (
    set "var=%%a"
    SETLOCAL EnableDelayedExpansion
    :: replace <-NAME->, <-NAME_LOWER-> and format
    set "var=!var:<-NAME->=%NAME%!"
    set "var=!var:<-NAME_LOWER->=%NAME_LOWER%!"
    set "var=!var:*:=!"
    :: write updated line to pro file
    if [!var!]==[] (echo: >> "%MODULE_PATH%\translations\%NAME_LOWER%_de.ts") else (echo !var! >> "%MODULE_PATH%\translations\%NAME_LOWER%_de.ts")
    ENDLOCAL
)

:: Create translations-EN:
echo Creating translations-EN
set TEMPLATE_FILE_EN=%~dp0\templates\translations_en.ts.template
:: Create new empty file
type nul > "%MODULE_PATH%\translations\%NAME_LOWER%_en.ts"

:: iterate template line by line
SETLOCAL DisableDelayedExpansion
FOR /F "usebackq delims=" %%a in (`"findstr /n ^^ %TEMPLATE_FILE_EN%"`) do (
    set "var=%%a"
    SETLOCAL EnableDelayedExpansion
    :: replace <-NAME->, <-NAME_LOWER-> and format
    set "var=!var:<-NAME->=%NAME%!"
    set "var=!var:<-NAME_LOWER->=%NAME_LOWER%!"
    set "var=!var:*:=!"
    :: write updated line to pro file
    if [!var!]==[] (echo: >> "%MODULE_PATH%\translations\%NAME_LOWER%_en.ts") else (echo !var! >> "%MODULE_PATH%\translations\%NAME_LOWER%_en.ts")
    ENDLOCAL
)

:: Create Header-files:
echo Creating .h-file

set TEMPLATE_FILE_H=%~dp0\templates\plugin.h.template
:: Create new empty file
type nul > "%MODULE_PATH%/%NAME_LOWER%.h"
:: iterate template line by line
SETLOCAL DisableDelayedExpansion
FOR /F "usebackq delims=" %%a in (`"findstr /n ^^ %TEMPLATE_FILE_H%"`) do (
    set "var=%%a"
    SETLOCAL EnableDelayedExpansion
    :: replace <-NAME->, <-NAME_LOWER-> and format
    set "var=!var:<-NAME->=%NAME%!"
    set "var=!var:<-NAME_LOWER->=%NAME_LOWER%!"
    set "var=!var:<-NAME_UPPER->=%NAME_UPPER%!"
    set "var=!var:*:=!"
    :: write updated line to pro file
    if [!var!]==[] (echo: >> "%MODULE_PATH%/%NAME_LOWER%.h") else (echo !var! >> "%MODULE_PATH%/%NAME_LOWER%.h")
    ENDLOCAL
)

:: Create Source-files:
echo Creating .cpp-file

set TEMPLATE_FILE_CPP=%~dp0\templates\plugin.cpp.template
:: Create new empty file
type nul > "%MODULE_PATH%/%NAME_LOWER%.cpp"
:: iterate template line by line
SETLOCAL DisableDelayedExpansion
FOR /F "usebackq delims=" %%a in (`"findstr /n ^^ %TEMPLATE_FILE_CPP%"`) do (
    set "var=%%a"
    SETLOCAL EnableDelayedExpansion
    :: replace <-NAME->, <-NAME_LOWER-> and format
    set "var=!var:<-NAME->=%NAME%!"
    set "var=!var:<-NAME_LOWER->=%NAME_LOWER%!"
    set "var=!var:<-NAME_UPPER->=%NAME_UPPER%!"
    set "var=!var:*:=!"
    :: write updated line to pro file
    if [!var!]==[] (echo: >> "%MODULE_PATH%/%NAME_LOWER%.cpp") else (echo !var! >> "%MODULE_PATH%/%NAME_LOWER%.cpp")
    ENDLOCAL
)

:: add as subdir project to iVS3D/src/CMakeLists.txt
echo if(Build_Plugins) >> %~dp0\..\iVS3D\src\CMakeLists.txt
echo   add_subdirectory(iVS3D-%NAME%Plugin) >> %~dp0\..\iVS3D\src\CMakeLists.txt
echo endif() >> %~dp0\..\iVS3D\src\CMakeLists.txt

:end
pause
goto :eof

:TOUPPERCASE
if not defined %~1 exit /b
for %%a in ("a=A" "b=B" "c=C" "d=D" "e=E" "f=F" "g=G" "h=H" "i=I" "j=J" "k=K" "l=L" "m=M" "n=N" "o=O" "p=P" "q=Q" "r=R" "s=S" "t=T" "u=U" "v=V" "w=W" "x=X" "y=Y" "z=Z" "ä=Ä" "ö=Ö" "ü=Ü") do (
call set %~1=%%%~1:%%~a%%
)