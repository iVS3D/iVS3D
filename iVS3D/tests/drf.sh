#!/bin/sh
wget "https://drive.google.com/uc?export=download&id=1ZCLGsmmX6rlcTYSoQlV8LVll-kWSCAgN" -O test.zip
unzip -o test.zip -d testresources
rm test.zip
