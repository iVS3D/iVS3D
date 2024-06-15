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
Copy-Item -Path .\bin\* -Destination "." -Recurse
Remove-Item ".\bin" -Recurse

# add Qt dependencies for iVS3D-core
Write-Host "`nRunning windeployqt.exe"
Invoke-Expression -Command "${QT_PATH}\bin\windeployqt.exe --libdir . --plugindir plugins -concurrent iVS3D-core.exe"
If($LASTEXITCODE -gt 0){ Package-Error }

# add Qt dependencies and qml files for plugins which use qml (this requires mapping of dll name to source directory)
foreach ($entry in $QML_PLUGINS.GetEnumerator()) {
  $key = $entry.Key
  $value = $entry.Value
  
  if ([string]::IsNullOrEmpty($value)) {
      Write-Output "Skipping plugin: $key.dll because no qml source is provided"
      continue
  }
  
  Write-Output "Deploying: $key.dll with qml files from: $value directory"
  Invoke-Expression -Command "${QT_PATH}\bin\windeployqt.exe --libdir . --plugindir plugins --qmldir $PROJECT_ROOT\$value plugins\$key.dll"
  If($LASTEXITCODE -gt 0){ Package-Error }
}

# add Qt dependencies for all plugins found in plugins folder
$files = Get-ChildItem .\plugins\*.dll
foreach ($file in $files) {
  Write-Output "Deploying: $file.dll without qml files"
  Invoke-Expression -Command "${QT_PATH}\bin\windeployqt.exe --libdir . --plugindir plugins $file"
  If($LASTEXITCODE -gt 0){ Package-Error }
}

# add OpenCV dependencies
Write-Host "`nCopying OpenCV dlls"
Copy-Item -Path "${OCV_BIN}\opencv_world470.dll" -Destination "."
Copy-Item -Path "${OCV_BIN}\opencv_videoio_ffmpeg470_64.dll" -Destination "."

If($CUDA_VERSION){
  Copy-Item -Path "${OCV_BIN}\opencv_img_hash470.dll" -Destination "."
}

# add MSVC runtime
Write-Host "`nCopying MSVC ${MSVC_VERSION} runtime dlls"
Copy-Item -Path C:\Windows\System32\msvcp_win.dll -Destination "."
Copy-Item -Path C:\Windows\System32\msvcp140.dll -Destination "."
Copy-Item -Path C:\Windows\System32\vcruntime140.dll -Destination "."
Copy-Item -Path C:\Windows\System32\vcruntime140_1.dll -Destination "."

# add cuda dependencies
If($CUDA_VERSION){
  Write-Host "`nCopying CUDA ${CUDA_VERSION} runtime dlls"
  If(!($CUDA_BIN)){
    $CUDA_BIN = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v${CUDA_VERSION}\bin"
  }
  Copy-Item -Path "${CUDA_BIN}\*.dll" -Destination "."

  Write-Host "`nCopying CUDNN runtime dlls"
  If(!($CUDNN_BIN_PATH)){
    $CUDNN_BIN_PATH = "C:\Program Files\NVIDIA\CUDNN\8.8.0\bin"
  }
  Copy-Item -Path "${CUDNN_BIN_PATH}\*.dll" -Destination "."
}

Set-Content -Path qt.conf -Value "[Paths]
Prefix = ./
Plugins = plugins
Imports = plugins
Qml2Imports = plugins"

Pop-Location

# create zip archive
Compress-Archive -Path .\${PACKAGE_NAME}\* -DestinationPath "${PACKAGE_NAME}.zip"