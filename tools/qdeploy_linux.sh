#!/bin/bash

echo "Build configuration:"
echo "--------------------"
echo "  Project:          $PRO_PATH"
echo "  Install location: $INSTALL_PATH"
echo "  Qt location:      $QT_PATH"
echo "  OpenCV location:  $OCV_PRI"
echo "  Use CUDA:         $USE_CUDA"
if $USE_CUDA
then
  echo "  CUDA location:    $CUDA_PRI"
fi
echo "  Project version:  $APP_VERSION"
echo "  Project date:     $APP_DATE"
echo "--------------------"

echo "LD_LIBRARY_PATH:    "
echo "--------------------"
echo "  $LD_LIBRARY_PATH"
echo "--------------------"

# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/bin
SCRIPTPATH=$(dirname "$SCRIPT")

buildapp() {
  mkdir -p $INSTALL_PATH/build
  cd $INSTALL_PATH/build
  echo
  echo "qmake:"
  echo "--------------------"
  if $USE_CUDA
  then
    $QT_PATH/bin/qmake $PRO_PATH -spec linux-g++ CONFIG+=qtquickcompiler CONFIG+=with_cuda DEFINES+=HIDE_INPUT_WIDGET DEFINES+=IVS3D_VER=$APP_VERSION DEFINES+=IVS3D_DAT=$APP_DATE && /usr/bin/make qmake_all
  else
    $QT_PATH/bin/qmake $PRO_PATH -spec linux-g++ CONFIG+=qtquickcompiler DEFINES+=HIDE_INPUT_WIDGET DEFINES+=IVS3D_VER=$APP_VERSION DEFINES+=IVS3D_DAT=$APP_DATE && /usr/bin/make qmake_all
  fi
  echo
  echo "make:"
  echo "--------------------"
  /usr/bin/make -j$(nproc)
  echo
  echo "make install:"
  echo "--------------------"
  /usr/bin/make install -j$(nproc)
  echo
  echo "make:"
  echo "--------------------"
  /usr/bin/make clean -j$(nproc)
}

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
      if ! grep -q "$lib" $SCRIPTPATH/excluded-libs.txt; then
        echo "> cp $path $2/$lib"
        cp "$path" "$2/$lib"
        addlibs "lib/$lib" "$INSTALL_PATH/install/lib"
      fi
    fi
  fi
done
IFS=" "
}

deployapp() {

  mkdir -p $INSTALL_PATH/install/plugins
  cd $INSTALL_PATH
  cp -R build/${IVS3D_PREFIX}src/iVS3D-core/* install
  cp build/${IVS3D_PREFIX}src/plugins/* install/plugins
  rm -f install/Makefile
  
  cd install
  
  mkdir lib

  echo " "
  echo "--------------------------------"
  echo "-- adding libs for iVS3D-core --"
  echo "--------------------------------"

  addlibs iVS3D-core "$INSTALL_PATH/install/lib"
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
    addlibs $plug "$INSTALL_PATH/install/lib"
    patchelf --set-rpath '$ORIGIN:$ORIGIN/..:$ORIGIN/../lib' $plug
  done

  echo " "
  echo "--------------------------------"
  echo "--     adding qt plugins      --"
  echo "--------------------------------"

  QT_PLUGIN_FILES="iconengines/libqsvgicon.so platforms/libqxcb.so"
  QT_PLUGIN_FOLDERS="imageformats platforminputcontexts xcbglintegrations geoservices"
  mkdir -p $INSTALL_PATH/install/plugins/iconengines
  mkdir -p $INSTALL_PATH/install/plugins/platforms

  for QT_PLUGIN in $QT_PLUGIN_FILES
  do
    cp $QT_PATH/plugins/$QT_PLUGIN $INSTALL_PATH/install/plugins/$QT_PLUGIN
    addlibs $INSTALL_PATH/install/plugins/$QT_PLUGIN "$INSTALL_PATH/install/lib"
  done

  for QT_PFOLDER in $QT_PLUGIN_FOLDERS
  do
    mkdir -p $PWD/plugins/$QT_PFOLDER
    cp $QT_PATH/plugins/$QT_PFOLDER/* $INSTALL_PATH/install/plugins/$QT_PFOLDER
    for files in $INSTALL_PATH/install/plugins/$QT_PFOLDER/*
    do
        addlibs $files "$INSTALL_PATH/install/lib"
    done
  done
  
  echo " "
  echo "--------------------------------"
  echo "-- adding qml files and libs  --"
  echo "--------------------------------"

  cp -R $QT_PATH/qml $INSTALL_PATH/install

  for qmlfile in $(find $INSTALL_PATH/install/qml -name '*lib*.so*')
  do
    addlibs $qmlfile "$INSTALL_PATH/install/lib"
  done

  echo " "
  echo "--------------------------------"
  echo "--   setting rpath to ORIGIN  --"
  echo "--------------------------------"

  for OCV_FILE in "lib/*.so*"
  do
    patchelf --set-rpath '$ORIGIN' $OCV_FILE
  done

  cat > $INSTALL_PATH/install/qt.conf << EOL
[Paths]
Prefix = ./
Plugins = plugins
Imports = qml
Qml2Imports = qml
EOL
}

create3rdpartypri() {
  if $USE_CUDA
  then
    {
      echo "include($OCV_PRI)"
      echo "include($CUDA_PRI)"
    } > $PRI_FILE
  else
    echo "include($OCV_PRI)" > $PRI_FILE
  fi
}

check3rdpartypri() {
  if [ -f "$PRI_FILE" ] 
  then
    while true; do
      read -p "A 3rdparty.pri file already exists! Do you wish to override it? [Y/N]" yn
      case $yn in
        [Yy]* ) create3rdpartypri; break;;
        [Nn]* ) break;;
        * ) echo "Please answer yes or no.";;
      esac
    done
  else
    create3rdpartypri;
  fi
}

executeAll() {
  check3rdpartypri
  buildapp
  deployapp
}

while true; do
    read -p "Do you wish to deploy this program? [Y/N]" yn
    case $yn in
        [Yy]* ) executeAll; break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done
