#!/bin/bash

tag=$(git describe --abbrev=0 --tags)
tag_long=$(git describe --tags)

if [ "${tag}" = "${tag_long}" ]; then
  echo ${tag}
else
  echo ${tag}-dev
fi
