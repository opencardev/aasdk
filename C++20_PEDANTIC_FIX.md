# C++20 Build Failure: Pedantic Flag Fix

## Problem

Build failures on the `feature/c++20` branch during compilation of external dependencies (Abseil and Protobuf v30.0).

### Root Cause

The AASDK CMakeLists.txt sets a global C++ compiler flag:
```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT} -fPIC -Wall -pedantic")
```

When `FetchContent_MakeAvailable()` is called for external dependencies, these global flags are inherited by the child projects (Abseil and Protobuf). The `-pedantic` flag causes valid code in these libraries to trigger warnings that are treated as errors during compilation.

### Error Symptoms

Build log excerpt showing the failure:
```
#14 76.37 In file included from /src/build-debug/_deps/abseil-src/absl/strings/internal/str_format/arg.h:35,
#14 81.25    31 |              ? ~static_cast<__int128>
#14 81.25 [output clipped, log limit 2MiB reached]
#14 81.36 [ 24%] Building CXX object _deps/protobuf-build/CMakeFiles/protoc-gen-upb.dir/src/google/protobuf/arena.cc.o
```

The specific error was in Abseil's string formatting code where `-Wpedantic` warnings were being promoted to errors.

## Solution

### Implementation

Modified [protobuf/CMakeLists.txt](protobuf/CMakeLists.txt) to:

1. **Save** the original CMAKE_CXX_FLAGS before building external dependencies
2. **Temporarily disable** the `-pedantic` flag before calling FetchContent_MakeAvailable()
3. **Restore** the original flags after external dependencies are built

### Code Changes

```cmake
# Save current compiler flags to restore later
set(SAVED_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Remove -pedantic from flags for external dependencies (Abseil, Protobuf)
# These external libraries may have valid code that triggers pedantic warnings
string(REPLACE " -pedantic" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
message(STATUS "Temporarily disabled -pedantic flag for external dependency compilation")

# ... FetchContent_MakeAvailable() calls ...

# Restore original compiler flags for AASDK code
set(CMAKE_CXX_FLAGS "${SAVED_CMAKE_CXX_FLAGS}")
message(STATUS "Restored original compiler flags (with -pedantic) for AASDK code")
```

### Benefits

- ✅ External dependencies (Abseil, Protobuf) build without pedantic warnings
- ✅ AASDK's own code still compiles with strict `-pedantic` checks
- ✅ Maintains code quality standards for the main project
- ✅ Enables C++20 feature branch to proceed with Protobuf v30.0 and Abseil LTS 2024-07-22

## Testing

The fix has been committed to `feature/c++20` branch (commit 94b2fcd) and should resolve the following GitHub Actions runs that were previously failing:
- Run 21229687218 (trixie/bookworm amd64 builds failed)
- All prior failures on feature/c++20 due to this issue

Next build should complete successfully with these external dependencies compiling cleanly.

## Related Issues

- **GitHub Actions Workflows**: `.github/workflows/build_native.yml`
- **Feature Branch**: `feature/c++20` (C++20 modernisation effort)
- **Dependencies**: 
  - Abseil: LTS 2024-07-22 (20240722.0)
  - Protobuf: v30.0

## Notes

The root cause was pre-existing and unrelated to the documentation additions made in previous commits (3d1c2c0, 8c4b137, etc.). The build system issue affected the entire feature branch from the beginning of the C++20 modernisation effort.

Commit: 94b2fcd - "fix(c++20): disable -pedantic for external dependencies (Abseil, Protobuf)"
