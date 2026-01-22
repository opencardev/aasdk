#!/bin/bash
set -e

echo "=== Testing CMake Configuration with Protobuf v30.0 ===" 
cd /mnt/c/Users/matth/install/repos/opencardev/aasdk

echo "Cleaning build directories..."
rm -rf build-debug protobuf/build

echo "Creating protobuf build directory..."
mkdir -p protobuf/build
cd protobuf/build

echo "Running CMake configuration..."
cmake -DCMAKE_BUILD_TYPE=Debug .. 2>&1 | tee cmake-output.log

echo ""
echo "=== CMake Configuration Summary ==="
grep -E "(Protobuf|protobuf|C\+\+ standard|AASDK)" cmake-output.log | head -20

echo ""
echo "=== Checking for errors ==="
if grep -qi "error" cmake-output.log; then
    echo "ERRORS FOUND:"
    grep -i "error" cmake-output.log
    exit 1
else
    echo "No errors found in CMake configuration!"
fi

echo ""
echo "Configuration successful! You can now run: make -j2"
