@echo off
setlocal
:: build tool configuration - EDIT THIS!
:: -------------------------------------
:: Visual Studio Version (2019;2022)
set VS_VERSION=2022
:: Visual Studio Edition (Community;Enterprise)
set VS_EDITION=Community
:: (ADVANCED) Path to vcvarsall.bat of VS Developer Console
set VSVARS_PATH=C:\Program Files\Microsoft Visual Studio\%VS_VERSION%\%VS_EDITION%\VC\Auxiliary\Build
:: Path to Qt installation: bin\qmake.exe and bin\lrelease-pro.exe
set QT_PATH=C:\Qt\5.15.2\msvc2019_64
:: Path to Qt Jom installation, running jom.exe
set JOM_PATH=C:\Qt\Tools\QtCreator\bin\jom


:: build dependencies - EDIT THIS!
:: -------------------------------
:: Path to OpenCV bin folder (make sure to add the opencv.pri file to your 3rdparty.pri as well!)
set OCV_BIN=C:\OpenCV\opencv_4.7.0_msvc2019_win_x64\x64\vc16\bin
:: (OPTIONAL) CUDA Version for GPU acceleration
::set CUDA_VERSION=12.0
:: (ADVANCED) Only uncomment if CUDA is not installed at default location! point this variable to CUDA bin folder
::set CUDA_BIN=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v%CUDA_VERSION%\bin

pushd %~pd0
pushd ..

:: output location (edit if you want to)
set INSTALL_PATH=%cd%\Releases

for /f %%i in ('powershell.exe -executionpolicy bypass -file .\ci\find-version.ps1') do set APP_VERSION=%%i
for /f %%i in ('powershell.exe -executionpolicy bypass -command Get-Date -Format "yyyy-MM-dd"') do set APP_DATE=%%i

echo Building iVS3D-%APP_VERSION% at %APP_DATE%
echo ------------------------------------------

powershell.exe -executionpolicy bypass -command $VCVARS_PATH=$env:VSVARS_PATH; $QT_PATH=$env:QT_PATH; $JOM_PATH=$env:JOM_PATH; $OCV_BIN=$env:OCV_BIN; $MSVC_VERSION=$env:VS_VERSION; $APP_VERSION=$env:APP_VERSION; $APP_DATE=$env:APP_DATE; $INSTALL_PATH=$env:INSTALL_PATH; $CUDA_VERSION=$env:CUDA_VERSION; $CUDA_BIN=$env:CUDA_BIN; .\ci\make-upload-package.ps1

popd
popd

pause