#---------------------------------------------------------------------------------------------------
# translations

message("PRE-BUILD: building translation files")
system(lrelease $$PWD/translations/*.ts)

#---------------------------------------------------------------------------------------------------
# pyinstaller

!exists( $$PWD/ots/colmapwrapper/ColmapWorker ) {
  PY_COLMAP_WRAPPER_SOURCE += \     
     $$PWD/ots/colmapwrapper/GpsEntry.py \
     $$PWD/ots/colmapwrapper/ColmapWorker.py

  message("PRE-BUILD: running pyinstaller for ColmapWorker")
  system(python3 -m PyInstaller --distpath $$PWD/ots/colmapwrapper/ --specpath $$PWD/ots/colmapwrapper/ \
        --hidden-import=pyproj.datadir \
        --copy-metadata pyproj \
        --onefile --name ColmapWorker $$PY_COLMAP_WRAPPER_SOURCE)
}

!exists( $$PWD/ots/colmapwrapper/ExctractGpsReferenceData ) {
  PY_EXTRACT_GPS_SOURCE += \     
     $$PWD/ots/colmapwrapper/GpsEntry.py \
     $$PWD/ots/colmapwrapper/ExctractGpsReferenceData.py

  message("PRE-BUILD: running pyinstaller for ExctractGpsReferenceData")
  system(python3 -m PyInstaller --distpath $$PWD/ots/colmapwrapper/ --specpath $$PWD/ots/colmapwrapper/ \
        --hidden-import=pyproj.datadir \
        --copy-metadata pyproj \
        --onefile --name ExctractGpsReferenceData $$PY_EXTRACT_GPS_SOURCE)
}
