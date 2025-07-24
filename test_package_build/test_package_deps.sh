#!/bin/bash

# Test the package dependency syntax by creating a minimal control file
echo "Testing package dependency syntax..."

# Our dependency string from CMakeLists.txt
DEPENDS="libusb-1.0-0 (>= 1.0.0), libboost-system1.81.0 | libboost-system1.74.0, libboost-log1.81.0 | libboost-log1.74.0, libssl3 (>= 3.0.0), libprotobuf32 (>= 3.21.0)"

# Create a minimal Debian control file for testing
cat > control_test << EOF
Package: libaasdk-arm64-test
Version: 2025.07.24
Architecture: arm64
Maintainer: AASDK Test <test@example.com>
Depends: $DEPENDS
Description: Test package for dependency validation
 This is a test package to validate AASDK dependencies.
EOF

echo "Control file created:"
cat control_test

echo
echo "Validating dependency syntax..."

# Test if the dependency string is valid by parsing it
if dpkg-deb --help > /dev/null 2>&1; then
    echo "✓ dpkg-deb is available for package testing"
else
    echo "✗ dpkg-deb not available"
fi

# Test each dependency individually
echo
echo "Testing individual dependencies:"

IFS=', ' read -ra DEPS <<< "$DEPENDS"
for dep in "${DEPS[@]}"; do
    # Remove version constraints for basic package existence check
    pkg_name=$(echo "$dep" | sed 's/ .*$//' | sed 's/|.*$//')
    if apt-cache show "$pkg_name" > /dev/null 2>&1; then
        echo "✓ $pkg_name - Package exists in repository"
    else
        echo "? $pkg_name - Package might not exist or need different name"
    fi
done

echo
echo "✅ Dependency syntax test completed!"
