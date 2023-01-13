#!/bin/bash

# set LD_LIBRARY_PATH
# this is necessary for ldd to find libraries
export LD_LIBRARY_PATH=/path/to/cuda/lib64:/path/to/Qt/5.15.2/gcc_64/lib:/path/to/opencv/lib

# set variables for finding dependencies
export QT_PATH=/path/to/Qt/5.15.2/gcc_64
export USE_CUDA=true
export CUDA_PRI=/path/to/cuda.pri
export OCV_PRI=/path/to/opencv.pri

# build version and date displayed in the app
export APP_VERSION=1.3.3
export APP_DATE=2022-12-03

# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/bin
SCRIPTPATH=$(dirname "$SCRIPT")

# feel free to change the output path
export INSTALL_PATH=$SCRIPTPATH/../Releases/iVS3D-${APP_VERSION}

# path to iVS3D.pro and 3rdparty.pri, no need to touch this
export PRO_PATH=$SCRIPTPATH/../iVS3D/iVS3D.pro
export PRI_FILE="$(dirname "$PRO_PATH")/3rdparty.pri"
export PATH=$QT_PATH/bin:$PATH

$SCRIPTPATH/qdeploy_linux.sh
