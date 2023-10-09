#!/bin/sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [ ! -d "$SCRIPT_DIR/testresources/" ]; then
    wget "https://drive.google.com/uc?export=download&id=1tR3vq25R-qqbgzOoyMUvKVnEPJm7UE6o" -O $SCRIPT_DIR/test.zip
    unzip -o $SCRIPT_DIR/test.zip -d $SCRIPT_DIR/testresources
    rm $SCRIPT_DIR/test.zip
fi
