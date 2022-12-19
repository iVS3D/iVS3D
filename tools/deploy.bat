@echo off
setlocal
:: build tool locations - EDIT THEM!
set QTDIR=C:\Qt\5.12.12\msvc2015_64
set JOM=C:/Qt/Tools/QtCreator/bin/jom/jom.exe
set VSDIR=C:\Program Files (x86)\Microsoft Visual Studio 14.0

:: build configuration - EDIT THEM!
set PROJ=C:\Path\to\iVS3D.pro
set APP_VERSION=1.3.3
set APP_DATE=2022-12-13
set USE_CUDA=1

:: output location (edit if you want to)
set INSTALL_PATH=%cd%\..\Releases\iVS3D-%APP_VERSION%

:: NO NEED TO TOUCH THE REST!

SET PATH=%PATH%;%QTDIR%\bin\

if %USE_CUDA%==1 (set WITH_CUDA=with_cuda) else (set WITH_CUDA= )

@echo on
mkdir %INSTALL_PATH%\build
cd %INSTALL_PATH%\build

call "%VSDIR%\VC\vcvarsall.bat" x64

call "%QTDIR%\bin\qmake.exe" %PROJ% -spec win32-msvc "CONFIG+=qtquickcompiler" "CONFIG+=%WITH_CUDA% with_dependencies" "DEFINES+=IVS3D_DAT=%APP_DATE% IVS3D_VER=%APP_VERSION%"
call "%JOM%" qmake_all
call "%JOM%"
call "%JOM%" install
call "%JOM%" clean

cd ..
mkdir install\plugins
xcopy /s "build\src\iVS3D-core\release" "install\"
xcopy /s "build\src\plugins" "install\plugins\"

cd install
call "%QTDIR%\bin\windeployqt.exe" --libdir . --plugindir plugins iVS3D-core.exe
for %%f in (plugins\*.dll) do (
  call "%QTDIR%\bin\windeployqt.exe" --libdir . --plugindir plugins %%f
)


PAUSE