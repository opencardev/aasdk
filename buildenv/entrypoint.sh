#!/bin/bash
ARCH=$1
MAJORVER=0

if [ -z "$ARCH" ]
then
    echo "Please supply a target architecture to build"
    echo "Choose from 'amd64', 'armhf'"
    exit
else
    if [ "$ARCH" != "amd64" ] && [ "$ARCH" != "armhf" ]
    then
        echo "Invalid architecture requested"
        exit
    fi
fi

echo "Now building within docker for $ARCH"

# Clear out the /build directory
mkdir build;
cd build;
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=$ARCH ../
make -j4
cpack --config CPackConfig.cmake

# Move it to release
mv *.deb /release/