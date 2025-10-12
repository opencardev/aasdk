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
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=$ARCH ../
make -j4

# Package with CPack
cpack

# Move packages to release directory
rm -f /release/*.deb
rm -f /release/*.so*

# Find and move .deb files (they may be in current directory or subdirectories)
find . -name "*.deb" -exec mv {} /release/ \;

# Move shared libraries from lib directory (if they exist)
cd ..
if [ -d "lib" ] && [ "$(ls -A lib/*.so* 2>/dev/null)" ]; then
    mv lib/*.so* /release/
fi
