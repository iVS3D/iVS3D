#!/bin/sh
wget "https://drive.google.com/uc?export=download&id=1tR3vq25R-qqbgzOoyMUvKVnEPJm7UE6o" -O test.zip
unzip -o test.zip -d testresources
rm test.zip
