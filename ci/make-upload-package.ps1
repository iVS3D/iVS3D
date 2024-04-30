# cleanup if an error occurs and exit
function Package-Error {
  $RETVAL = $LASTEXITCODE
  Pop-Location
  If (Test-Path ".\${PACKAGE_NAME}" -PathType Container) { Remove-Item ".\${PACKAGE_NAME}" -Recurse }
  Write-Host "`nERROR $RETVAL! Failed to deploy iVS3D!" -ForegroundColor Red
  exit $RETVAL
}

# exit whenever something fails
$ErrorActionPreference = "Stop"

# assumtion: The folder to package is located here and named $PACKAGE_NAME
Push-Location ${PACKAGE_NAME}

# move the files from the bin directory one level up
Write-Host "`nCreating strucktured package with dependencies"
Move-Item -Path .\bin\* -Destination "." -Recurse
Remove-Item ".\bin" -Recurse

Write-Host "`nRunning windeployqt.exe"
Invoke-Expression -Command "${QT_PATH}\bin\windeployqt.exe --libdir . --plugindir plugins -concurrent iVS3D-core.exe"
If($LASTEXITCODE -gt 0){ Package-Error }

$files = Get-ChildItem .\plugins\*.dll
foreach ($file in $files) {
  Invoke-Expression -Command "${QT_PATH}\bin\windeployqt.exe --libdir . --plugindir plugins $file"
  If($LASTEXITCODE -gt 0){ Package-Error }
}

Write-Host "`nCopying OpenCV dlls"
Copy-Item -Path "${OCV_BIN}\opencv_world470.dll" -Destination "."
Copy-Item -Path "${OCV_BIN}\opencv_videoio_ffmpeg470_64.dll" -Destination "."

If($CUDA_VERSION){
  Copy-Item -Path "${OCV_BIN}\opencv_img_hash470.dll" -Destination "."
}

Write-Host "`nCopying MSVC ${MSVC_VERSION} runtime dlls"
Copy-Item -Path C:\Windows\System32\msvcp_win.dll -Destination "."
Copy-Item -Path C:\Windows\System32\msvcp140.dll -Destination "."
Copy-Item -Path C:\Windows\System32\vcruntime140.dll -Destination "."
Copy-Item -Path C:\Windows\System32\vcruntime140_1.dll -Destination "."

If($CUDA_VERSION){
  Write-Host "`nCopying CUDA ${CUDA_VERSION} runtime dlls"
  If(!($CUDA_BIN)){
    $CUDA_BIN = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v${CUDA_VERSION}\bin"
  }
  Copy-Item -Path "${CUDA_BIN}\*.dll" -Destination "."
}

Pop-Location

# create zip archive
Compress-Archive -Path .\${PACKAGE_NAME}\* -DestinationPath "${PACKAGE_NAME}.zip"