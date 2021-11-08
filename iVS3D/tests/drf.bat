@ECHO OFF
ECHO Downloading test resources...

SET "downloadurl=https://storage.googleapis.com/drive-bulk-export-anonymous/20211104T153945Z/4133399871716478688/f444b642-c9bc-46b4-9b22-a350e6308baa/1/1105dbfc-a586-413d-bf12-99d0e3027167?authuser"
SET "outpath=%~dp0"

powershell.exe -Command "If(!(test-path %outpath%)){ New-Item -ItemType Directory -Force -Path %outpath% }; $ProgressPreference = 'SilentlyContinue'; Invoke-WebRequest -Uri %downloadurl% -OutFile %outpath%/test.zip; Expand-Archive %outpath%/test.zip -DestinationPath %outpath%; Remove-Item %outpath%/test.zip; If(test-path %outpath%/testresources){ Get-ChildItem -Path %outpath%/testresources -Recurse | Remove-Item -force -recurse; Remove-Item %outpath%/testresources -Force -Recurse }; Rename-Item -Force %outpath%/iVS3D-Testdata %outpath%/testresources"
ECHO Finished downloading...