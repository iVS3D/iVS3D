#! /bin/sh

# Arguments for deployment:
#
# iVS3D-deploy PATH/TO/iVS3D-core [PATH/TO/linuxdeployqt [PATH/TO/linux-dependencies.txt]]
#
# The path to iVS3D-core executable has to be supplied!
#
# linuxdeploy can be executed if the path to the executable is given.
#
# To add dependencies create a textfile and list the dependencies there.
# Pass as optional 3rd argument.
#--------------------------------------------------------------------------------

# navigate in the build directory where iVS3D-core is located
buildDir=$(dirname $1)
cd $buildDir

# move plugins folder and iVS3D-core files to build dir
mkdir plugins
mv src/plugins/* ./plugins
mv src/iVS3D-core/* .

# remove leftover folders and temporary build files
rm -rf src

if [ ! -z "$3" ]
then
    # if linuxdeployqt executable is provided add dependencies to lib folder
    mkdir -p $buildDir/lib

    echo "Added following extra dependencies:"
    while IFS= read -r line; do
    	DEPS=`ldd $1 | grep $line`
    	PP=$(echo $DEPS | cut -d' ' -f3)
    	echo $PP
    	cp $PP $buildDir/lib
    done < $3
fi

if [ ! -z "$2" ]
then
    echo "Starting linuxdeployqt..."
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$buildDir/lib
    $2 $buildDir/iVS3D-core -verbose=2 -no-translations -extra-plugins=$buildDir/plugins
fi
