#!/bin/bash

# set the cuda binaries path to the default install location if not defined yet
if [ -n "$CUDA_VERSION" ] && [ -z "$CUDA_BIN" ] 
then
  export CUDA_BIN=/usr/local/cuda-$CUDA_VERSION/lib64
fi

echo "Build configuration:"
echo "--------------------"
echo "  Qt libs:          $QT_PATH/lib"
echo "  OpenCV libs:      $OCV_BIN"
if [ -n "$CUDA_VERSION" ] 
then
  echo "  CUDA Version:     $CUDA_VERSION"
  echo "  CUDA libs:        $CUDA_BIN"
fi
echo "  Project version:  $APP_VERSION"
echo "  Project date:     $APP_DATE"
echo "  Install location: $INSTALL_PATH"
echo "  Package name:     $PACKAGE_NAME"
echo "--------------------"

# set LD_LIBRARY_PATH
# this is necessary for ldd to find runtime libraries of Qt, OpenCV and cuda
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CUDA_BIN:$QT_PATH/lib:$OCV_BIN

echo "LD_LIBRARY_PATH:    "
echo "--------------------"
echo "  $LD_LIBRARY_PATH"
echo "--------------------"

addlibs () {
IFS="
"
for dep in $(ldd $1)
do
  IFS="=>"
  read -ra STR <<< "$dep"
  lib=$(echo ${STR[0]} | cut -d' ' -f1 | xargs)
  path=$(echo ${STR[2]} | cut -d' ' -f2)
  if [ ! -z "$path" ]
  then
    if [ ! -f "$2/$lib" ]
    then
      if ! grep -q "$lib" $EXCLUDED_LIBS; then
        echo "> cp $path $2/$lib"
        cp "$path" "$2/$lib"
        addlibs "lib/$lib" "$INSTALL_PATH/$PACKAGE_NAME/lib"
      fi
    fi
  fi
done
IFS=" "
}

deployapp() {
  # work from installed package location
  cd $INSTALL_PATH/$PACKAGE_NAME
  # remove subfolder bin
  mv bin/* .
  rm -rf bin
  # create folder for dependencies
  mkdir lib

  echo " "
  echo "--------------------------------"
  echo "-- adding libs for iVS3D-core --"
  echo "--------------------------------"

  addlibs iVS3D-core "$INSTALL_PATH/$PACKAGE_NAME/lib"
  patchelf --set-rpath '$ORIGIN:$ORIGIN/lib' iVS3D-core

  echo " "
  echo "--------------------------------"
  echo "--   adding libs for plugins  --"
  echo "--------------------------------"

  PLUGINS="plugins/*.so"
  for plug in $PLUGINS
  do
    echo " "
    echo "--------------------------------"
    echo " -- plugin: $plug"
    echo "--------------------------------"
    addlibs $plug "$INSTALL_PATH/$PACKAGE_NAME/lib"
    patchelf --set-rpath '$ORIGIN:$ORIGIN/..:$ORIGIN/../lib' $plug
  done

  echo " "
  echo "--------------------------------"
  echo "--     adding qt plugins      --"
  echo "--------------------------------"

  QT_PLUGIN_FILES="iconengines/libqsvgicon.so platforms/libqxcb.so"
  QT_PLUGIN_FOLDERS="imageformats platforminputcontexts xcbglintegrations geoservices"
  mkdir -p $INSTALL_PATH/$PACKAGE_NAME/plugins/iconengines
  mkdir -p $INSTALL_PATH/$PACKAGE_NAME/plugins/platforms

  for QT_PLUGIN in $QT_PLUGIN_FILES
  do
    cp $QT_PATH/plugins/$QT_PLUGIN $INSTALL_PATH/$PACKAGE_NAME/plugins/$QT_PLUGIN
    addlibs $INSTALL_PATH/$PACKAGE_NAME/plugins/$QT_PLUGIN "$INSTALL_PATH/$PACKAGE_NAME/lib"
  done

  for QT_PFOLDER in $QT_PLUGIN_FOLDERS
  do
    mkdir -p plugins/$QT_PFOLDER
    cp $QT_PATH/plugins/$QT_PFOLDER/* $INSTALL_PATH/$PACKAGE_NAME/plugins/$QT_PFOLDER
    for files in $INSTALL_PATH/$PACKAGE_NAME/plugins/$QT_PFOLDER/*
    do
        addlibs $files "$INSTALL_PATH/$PACKAGE_NAME/lib"
    done
  done
  
  echo " "
  echo "--------------------------------"
  echo "-- adding qml files and libs  --"
  echo "--------------------------------"

  cp -R $QT_PATH/qml $INSTALL_PATH/$PACKAGE_NAME

  for qmlfile in $(find $INSTALL_PATH/$PACKAGE_NAME/qml -name '*lib*.so*')
  do
    addlibs $qmlfile "$INSTALL_PATH/$PACKAGE_NAME/lib"
  done

  echo " "
  echo "--------------------------------"
  echo "--   setting rpath to ORIGIN  --"
  echo "--------------------------------"

  LIB_FILES="$INSTALL_PATH/$PACKAGE_NAME/lib/lib*"
  for OCV_FILE in $LIB_FILES
  do
    echo "adding rpath to $OCV_FILE"
    patchelf --set-rpath '$ORIGIN' $OCV_FILE
  done

  cat > $INSTALL_PATH/$PACKAGE_NAME/qt.conf << EOL
[Paths]
Prefix = ./
Plugins = plugins
Imports = qml
Qml2Imports = qml
EOL

  cd $INSTALL_PATH
  zip -r ${PACKAGE_NAME}.zip ${PACKAGE_NAME}/
}

deployapp