# AASDK Package Dependencies - Fix Summary

## Issue Fixed
The AASDK package dependencies in `CMakeLists.txt` had incorrect library version specifications that didn't match the available packages in Debian 12 (bookworm).

## Changes Made

### 1. CMakeLists.txt - Fixed Package Dependencies
**Before:**
```cmake
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libusb-1.0-0 (>= 1.0.0), libboost-system1.74.0 (>= 1.74.0), libboost-log1.74.0 (>= 1.74.0), libssl3 (>= 3.0.0), libprotobuf32 (>= 3.21.0)")
```

**After:**
```cmake
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libusb-1.0-0 (>= 1.0.0), libboost-system1.81.0 | libboost-system1.74.0, libboost-log1.81.0 | libboost-log1.74.0, libssl3 (>= 3.0.0), libprotobuf32 (>= 3.21.0)")
```

### 2. PACKAGING.md - Updated Documentation
Updated the package dependencies section to reflect the new alternative dependency specifications.

## Key Improvements

1. **Boost Library Compatibility**: Added support for both Boost 1.81.0 (default in Debian 12) and 1.74.0 (fallback) using the `|` alternative dependency syntax.

2. **Removed Hard Version Constraints**: Replaced hard version requirements with alternatives that allow the package manager to choose the best available version.

3. **Current System Compatibility**: Dependencies now match what's actually available in Debian 12 (bookworm):
   - libusb-1.0-0: 2:1.0.26-1 ✓
   - libboost-system1.81.0: 1.81.0-5+deb12u1 ✓
   - libboost-log1.81.0: 1.81.0-5+deb12u1 ✓
   - libssl3: 3.0.16-1~deb12u1+rpt1 ✓
   - libprotobuf32: 3.21.12-3 ✓

## Verification

Created test scripts that confirm:
- All required runtime dependencies are satisfied on the current system
- Package dependency syntax is valid
- Alternative dependencies are properly available

## Result

The AASDK packages can now be built and installed successfully on Debian 12 systems without dependency conflicts. The alternative dependency specification provides flexibility across different Debian/Ubuntu versions while maintaining compatibility with the target system.
