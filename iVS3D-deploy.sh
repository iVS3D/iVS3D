#! /bin/sh

buildDir=$1
cd $buildDir
mkdir plugins
mv src/plugins/* ./plugins
mv src/iVS3D-core/* .
rm -rf src
