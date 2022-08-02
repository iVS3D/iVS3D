TEMPLATE = subdirs

!test{
    SUBDIRS += src
}
test {
    SUBDIRS += tests
}

OTHER_FILES += \
    3rdparty.pri \
    3rdparty.txt \
    3rdparty_debian10-x64-cuda.json \
    setrpath.pri
