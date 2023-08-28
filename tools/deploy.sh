#!/bin/bash

###################### PRESETS - DONT TOUCH THEM! #################################

# Absolute path to this script, e.g. /home/user/iVS3D/tools/deploy.sh
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/iVS3D/tools
SCRIPTPATH=$(dirname "$SCRIPT")
# Absolute path to this project, e.g. /home/user/iVS3D
export PROJECT_ROOT=$SCRIPTPATH/..
# change directory to project root, so paths can be realtive within the project
cd $PROJECT_ROOT


###################### BUILD CONFIGURATION - EDIT THIS! ###########################

# Qt Version and Location
export QT_VERSION=5.15.2
export QT_PATH=$HOME/Qt/$QT_VERSION/gcc_64

# OpenCV binary location, this should be the version used in the 3rdparty.pri!
export OCV_BIN=/path/to/opencv/lib

# gcc version
export GCC_VERSION=10.2.1
export BUILD_ENVIRONMENT=debian11

# (OPTIONAL) cuda version and location of runtime binaries
#export CUDA_VERSION=12.0
#export CUDA_BIN=/usr/local/cuda-$CUDA_VERSION/lib64

# feel free to change the output path
export INSTALL_PATH=$PROJECT_ROOT/Releases


###################### DEPLOY STEPS - DONT TOUCH THEM! ###########################

# build version and date displayed in the app
export APP_VERSION="$(ci/find-version.sh)"
export APP_DATE="$(date '+%Y-%m-%d')"

# path to iVS3D.pro, no need to touch this
export PRO_PATH=$PROJECT_ROOT/iVS3D/iVS3D.pro
#export PATH=$QT_PATH/bin:$PATH

mkdir -p build
cd build
echo
echo "qmake:"
echo "--------------------"
if [ -n "$CUDA_VERSION" ]
then
  $QT_PATH/bin/qmake $PRO_PATH -spec linux-g++ CONFIG+=qtquickcompiler CONFIG+=with_cuda DEFINES+=IVS3D_VER=$APP_VERSION DEFINES+=IVS3D_DAT=$APP_DATE && /usr/bin/make qmake_all
else
  $QT_PATH/bin/qmake $PRO_PATH -spec linux-g++ CONFIG+=qtquickcompiler DEFINES+=IVS3D_VER=$APP_VERSION DEFINES+=IVS3D_DAT=$APP_DATE && /usr/bin/make qmake_all
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

cd $PROJECT_ROOT
export EXCLUDED_LIBS="$PROJECT_ROOT/ci/excluded-libs.txt"
source ci/make-upload-package.sh

rm -r $PROJECT_ROOT/build