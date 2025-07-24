#!/bin/bash

echo "Testing AASDK Package Dependencies"
echo "=================================="
echo

# List of dependencies we require
dependencies=(
    "libusb-1.0-0"
    "libboost-system1.81.0"
    "libboost-log1.81.0"
    "libssl3"
    "libprotobuf32"
)

# Alternative dependencies (fallback versions)
alternatives=(
    "libboost-system1.74.0"
    "libboost-log1.74.0"
)

echo "Checking primary dependencies:"
all_satisfied=true

for dep in "${dependencies[@]}"; do
    if dpkg -l | grep -q "^ii  $dep"; then
        version=$(dpkg -l | grep "^ii  $dep" | awk '{print $3}' | head -1)
        echo "✓ $dep ($version) - INSTALLED"
    else
        echo "✗ $dep - NOT INSTALLED"
        all_satisfied=false
    fi
done

echo
echo "Checking alternative dependencies:"
for dep in "${alternatives[@]}"; do
    if dpkg -l | grep -q "^ii  $dep"; then
        version=$(dpkg -l | grep "^ii  $dep" | awk '{print $3}' | head -1)
        echo "✓ $dep ($version) - AVAILABLE AS ALTERNATIVE"
    else
        echo "- $dep - NOT AVAILABLE"
    fi
done

echo
if [ "$all_satisfied" = true ]; then
    echo "✅ All primary dependencies are satisfied!"
    echo "📦 Package can be installed successfully"
else
    echo "❌ Some dependencies are missing"
    echo "🔧 Run: sudo apt install libusb-1.0-0-dev libboost-all-dev libssl-dev libprotobuf-dev"
fi

echo
echo "System Information:"
echo "OS: $(cat /etc/os-release | grep PRETTY_NAME | cut -d'"' -f2)"
echo "Architecture: $(dpkg --print-architecture)"
