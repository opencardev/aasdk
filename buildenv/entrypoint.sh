#!/bin/bash
ARCH=$1
MAJORVER=0

if [ -z "$ARCH" ]
then
    echo "Please supply a target architecture to build"
    echo "Choose from 'amd64', 'armhf', 'arm64'"
    exit
else
    if [ "$ARCH" != "amd64" ] && [ "$ARCH" != "armhf" ] && [ "$ARCH" != "arm64" ]
    then
        echo "Invalid architecture requested"
        exit
    fi
fi

echo "Now building within docker for $ARCH"

# Make protobuf
mkdir protobuf/build
cd protobuf/build
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=$ARCH ..
make
make install

# Clear out the /build directory
rm -f bin/*
rm -f lib/*
mkdir build;
cd build;
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=$ARCH ../
make -j4
cpack --config CPackConfig.cmake

# Move it to release
rm -f /release/*.deb
rm -f /release/*.so*
mv *.deb /release/
cd ..
mv lib/*.so* /release
