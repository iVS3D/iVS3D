# cleanup if an error occurs and exit
function Compile-Error {
  $RETVAL = $LASTEXITCODE
  Pop-Location
  If (Test-Path ".\${buildfolder}" -PathType Container) { Remove-Item ".\${buildfolder}" -Recurse }
  Write-Host "`nERROR $RETVAL! Failed to compile iVS3D!" -ForegroundColor Red
  exit $RETVAL
}

function Package-Error {
  $RETVAL = $LASTEXITCODE
  Pop-Location
  If (Test-Path ".\${buildfolder}" -PathType Container) { Remove-Item ".\${buildfolder}" -Recurse }
  If (Test-Path ".\${releasefolder}" -PathType Container) { Remove-Item ".\${releasefolder}" -Recurse }
  Write-Host "`nERROR $RETVAL! Failed to deploy iVS3D!" -ForegroundColor Red
  exit $RETVAL
}

# exit whenever something fails
$ErrorActionPreference = "Stop"

# Setup Visual Studio 14.0 Command Prompt for powershell
# see https://stackoverflow.com/questions/2124753/how-can-i-use-powershell-with-the-visual-studio-command-prompt 
Push-Location "${VCVARS_PATH}"
cmd /c "vcvarsall.bat x64&set" |
ForEach-Object {
  if ($_ -match "=") {
    $v = $_.split("="); set-item -force -path "ENV:\$($v[0])"  -value "$($v[1])"
  }
}
Pop-Location
Write-Host "`nVisual Studio ${MSVC_VERSION} Command Prompt variables set." -ForegroundColor Yellow

Write-Host "`nRunning lrelease-pro.exe" -ForegroundColor Yellow
Invoke-Expression -Command "${QT_PATH}\bin\lrelease-pro.exe .\iVS3D\iVS3D.pro"
If($LASTEXITCODE -gt 0){
  Write-Host "`nERROR $LASTEXITCODE! Failed to create translations!" -ForegroundColor Red
  exit $LASTEXITCODE
}

$buildfolder = "build"
If($CUDA_VERSION){
  $releasefolder = "iVS3D-${APP_VERSION}-cuda${CUDA_VERSION}-msvc${MSVC_VERSION}-x64"
} Else {
  $releasefolder = "iVS3D-${APP_VERSION}-msvc${MSVC_VERSION}-x64"
}

New-Item -ItemType Directory -Path ".\$buildfolder"
Push-Location ".\$buildfolder"

If($CUDA_VERSION){
  Write-Host "`nRunning qmake.exe WITH CUDA" -ForegroundColor Yellow
  Invoke-Expression -Command "${QT_PATH}\bin\qmake.exe ..\iVS3D\iVS3D.pro -spec win32-msvc `"CONFIG+=qtquickcompiler`" `"CONFIG+=with_cuda`" `"DEFINES+=IVS3D_DAT=$APP_DATE IVS3D_VER=$APP_VERSION`""
} Else {
  Write-Host "`nRunning qmake.exe" -ForegroundColor Yellow
  Invoke-Expression -Command "${QT_PATH}\bin\qmake.exe ..\iVS3D\iVS3D.pro -spec win32-msvc `"CONFIG+=qtquickcompiler`" `"DEFINES+=IVS3D_DAT=$APP_DATE IVS3D_VER=$APP_VERSION`""
}
If($LASTEXITCODE -gt 0){ Compile-Error }

Write-Host "`nRunning jom.exe qmake_all" -ForegroundColor Yellow
Invoke-Expression -Command "${JOM_PATH}\jom.exe qmake_all"
If($LASTEXITCODE -gt 0){ Compile-Error }

Write-Host "`nRunning jom.exe" -ForegroundColor Yellow
Invoke-Expression -Command "${JOM_PATH}\jom.exe"
If($LASTEXITCODE -gt 0){ Compile-Error }

Write-Host "`nRunning jom.exe install" -ForegroundColor Yellow
Invoke-Expression -Command "${JOM_PATH}\jom.exe install"
If($LASTEXITCODE -gt 0){ Compile-Error }

Write-Host "`nRunning jom.exe clean" -ForegroundColor Yellow
Invoke-Expression -Command "${JOM_PATH}\jom.exe clean"
If($LASTEXITCODE -gt 0){ Compile-Error }

Pop-Location

Write-Host "`nCreating strucktured package"
New-Item -ItemType Directory -Path ".\$releasefolder"
Copy-Item -Path .\$buildfolder\src\iVS3D-core\release\* -Destination ".\$releasefolder" -Recurse
New-Item -ItemType Directory -Path ".\$releasefolder\plugins"
Copy-Item -Path .\$buildfolder\src\plugins\* -Destination ".\$releasefolder\plugins"

Push-Location ".\$releasefolder"

Write-Host "`nRunning windeployqt.exe"
Invoke-Expression -Command "${QT_PATH}\bin\windeployqt.exe --libdir . --plugindir plugins -concurrent iVS3D-core.exe"
If($LASTEXITCODE -gt 0){ Package-Error }

$files = Get-ChildItem .\plugins\*.dll
foreach ($file in $files) {
  Invoke-Expression -Command "${QT_PATH}\bin\windeployqt.exe --libdir . --plugindir plugins $file"
  If($LASTEXITCODE -gt 0){ Package-Error }
}

Pop-Location

Write-Host "`nCopying OpenCV dlls"
Copy-Item -Path "${OCV_BIN}\opencv_world470.dll" -Destination ".\$releasefolder"
Copy-Item -Path "${OCV_BIN}\opencv_videoio_ffmpeg470_64.dll" -Destination ".\$releasefolder"

If($CUDA_VERSION){
  Copy-Item -Path "${OCV_BIN}\opencv_img_hash470.dll" -Destination ".\$releasefolder"
}

Write-Host "`nCopying MSVC ${MSVC_VERSION} runtime dlls"
Copy-Item -Path C:\Windows\System32\msvcp_win.dll -Destination ".\$releasefolder"
Copy-Item -Path C:\Windows\System32\msvcp140.dll -Destination ".\$releasefolder"
Copy-Item -Path C:\Windows\System32\vcruntime140.dll -Destination ".\$releasefolder"
Copy-Item -Path C:\Windows\System32\vcruntime140_1.dll -Destination ".\$releasefolder"

If($CUDA_VERSION){
  Write-Host "`nCopying CUDA ${CUDA_VERSION} runtime dlls"
  If(!($CUDA_BIN)){
    $CUDA_BIN = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v${CUDA_VERSION}\bin"
  }
  Copy-Item -Path "${CUDA_BIN}\*.dll" -Destination ".\$releasefolder"
}

If($INSTALL_PATH){
  Write-Host "`nInstalling to ${INSTALL_PATH}"
  If (!(Test-Path "${INSTALL_PATH}" -PathType Container)) {
    Write-Host "`nCreating install location"
    New-Item -ItemType Directory -Force -Path "${INSTALL_PATH}"
  }
  Move-Item -Path ".\${releasefolder}" -Destination "${INSTALL_PATH}"
}
Write-Host "`nCleaning temporary build folders"
Remove-Item ".\${buildfolder}" -Recurse