#!/bin/bash

echo "Testing aap_protobuf inclusion in AASDK packages"
echo "================================================"

# Build release version
echo "Building release version..."
mkdir -p build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ../
make -j$(nproc) aap_protobuf aasdk

echo
echo "Checking built libraries:"
echo "========================"
find . -name "*.so*" -type f | sort

echo
echo "Creating package..."
cpack --config CPackConfig.cmake

echo
echo "Checking package contents:"
echo "=========================="
for deb in *.deb; do
    if [ -f "$deb" ]; then
        echo
        echo "Contents of $deb:"
        dpkg-deb -c "$deb" | grep -E "\.(so|a)($|\\.)"
    fi
done

echo
echo "Checking install manifest:"
find . -name "install_manifest_*.txt" -exec echo "Contents of {}:" \; -exec cat {} \; 2>/dev/null
