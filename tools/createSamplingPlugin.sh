#!/bin/bash

NAME=$1

if [ -z "$NAME" ]
then
    echo "Wrong use!"
    exit 1
fi

# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/bin
SCRIPTPATH=$(dirname "$SCRIPT")

MODULE_PATH=${SCRIPTPATH}/../iVS3D/src/iVS3D-${NAME}Plugin
if [ -d "${MODULE_PATH}" ]
then
    echo "A plugin with this name already exists: '$MODULE_PATH'!"
    echo "Use a unique name!"
    exit 2
fi

echo "Creating new plugin '$NAME' in submodule '$MODULE_PATH'"

NAME_UPPER=${NAME^^}
NAME_LOWER=${NAME,,}

mkdir $MODULE_PATH

echo "Creating pro-file"
TEMPLATE_FILE_PRO=$SCRIPTPATH/templates/iVS3D-Plugin.pro.template
FILE_PRO=$MODULE_PATH/iVS3D-${NAME}Plugin.pro

cp $TEMPLATE_FILE_PRO $FILE_PRO
sed -i "s/<NAME>/${NAME}/g" $FILE_PRO
sed -i "s/<NAME_LOWER>/${NAME_LOWER}/g" $FILE_PRO

echo "Creating qrc-file"
TEMPLATE_FILE_QRC=$SCRIPTPATH/templates/resources.qrc.template
FILE_QRC=$MODULE_PATH/resources.qrc

cp $TEMPLATE_FILE_QRC $FILE_QRC
sed -i "s/<NAME>/${NAME}/g" $FILE_QRC
sed -i "s/<NAME_LOWER>/${NAME_LOWER}/g" $FILE_QRC

echo "Creating translations-folder"
mkdir $MODULE_PATH/translations

echo "Creating translations-DE"
TEMPLATE_FILE_DE=$SCRIPTPATH/templates/translations_de.ts.template
FILE_DE=$MODULE_PATH/translations/${NAME_LOWER}_de.ts

cp $TEMPLATE_FILE_DE $FILE_DE
sed -i "s/<NAME>/${NAME}/g" $FILE_DE
sed -i "s/<NAME_LOWER>/${NAME_LOWER}/g" $FILE_DE

echo "Creating translations-EN"
TEMPLATE_FILE_EN=$SCRIPTPATH/templates/translations_en.ts.template
FILE_EN=$MODULE_PATH/translations/${NAME_LOWER}_en.ts

cp $TEMPLATE_FILE_EN $FILE_EN
sed -i "s/<NAME>/${NAME}/g" $FILE_EN
sed -i "s/<NAME_LOWER>/${NAME_LOWER}/g" $FILE_EN

echo "Creating h-file"
TEMPLATE_FILE_H=$SCRIPTPATH/templates/plugin.h.template
FILE_H=$MODULE_PATH/${NAME_LOWER}.h

cp $TEMPLATE_FILE_H $FILE_H
sed -i "s/<NAME>/${NAME}/g" $FILE_H
sed -i "s/<NAME_LOWER>/${NAME_LOWER}/g" $FILE_H
sed -i "s/<NAME_UPPER>/${NAME_UPPER}/g" $FILE_H

echo "Creating cpp-file"
TEMPLATE_FILE_CPP=$SCRIPTPATH/templates/plugin.cpp.template
FILE_CPP=$MODULE_PATH/${NAME_LOWER}.cpp

cp $TEMPLATE_FILE_CPP $FILE_CPP
sed -i "s/<NAME>/${NAME}/g" $FILE_CPP
sed -i "s/<NAME_LOWER>/${NAME_LOWER}/g" $FILE_CPP
sed -i "s/<NAME_UPPER>/${NAME_UPPER}/g" $FILE_CPP

echo "SUBDIRS += iVS3D-${NAME}Plugin" >> $SCRIPTPATH/../iVS3D/src/src.pro
echo "iVS3D-${NAME}Plugin.depends = iVS3D-pluginInterface" >> $SCRIPTPATH/../iVS3D/src/src.pro

exit 0