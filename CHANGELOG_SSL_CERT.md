# SSL Certificate Externalization - Change Log

**Date:** 2025-01-XX  
**Component:** aasdk  
**Type:** Enhancement  

## Summary

Externalized SSL/TLS certificate and private key from hardcoded C++ constants to separate files, enabling easier certificate updates without recompilation.

## Files Modified

### aasdk/src/Messenger/Cryptor.cpp
- Added `#include <fstream>` and `#include <sstream>` for file I/O
- Added `readFileContents()` static helper function
- Added `loadCertificate()` function with 4-path fallback mechanism:
  1. `/etc/openauto/headunit.crt` (installed location)
  2. `/usr/share/aasdk/cert/headunit.crt` (alternative system location)
  3. `./cert/headunit.crt` (development directory)
  4. `../cert/headunit.crt` (alternative development directory)
- Added `loadPrivateKey()` function with identical fallback mechanism
- Modified `init()` to use file-based loading with fallback to embedded constants
- Added logging to show which certificate path was loaded

### aasdk/CMakeLists.txt
- Added install rules for certificate files:
  ```cmake
  install(FILES 
          "${CMAKE_CURRENT_SOURCE_DIR}/cert/headunit.crt"
          "${CMAKE_CURRENT_SOURCE_DIR}/cert/headunit.key"
          DESTINATION /etc/openauto
          PERMISSIONS OWNER_READ GROUP_READ
          COMPONENT runtime
  )
  ```

### aasdk/cert/ (New Directory)
- **headunit.crt**: Extracted Google Automotive Link certificate
- **headunit.key**: Extracted 2048-bit RSA private key
- **.gitignore**: Protects custom certificates (`custom_*.crt`, `*.pem`)
- **README.md**: Comprehensive documentation on certificate management

## Benefits

1. **Easier Updates**: Certificates can be updated by replacing files, no recompilation needed
2. **Custom Certificates**: Vendors can deploy custom certificates without modifying source code
3. **Development Flexibility**: Different certificate sets for testing/production
4. **Backward Compatibility**: Fallback to embedded constants if files not found
5. **Security**: Certificate files can be managed via package management system

## Installation Behavior

When the aasdk debian package is installed:
- Certificate files are copied to `/etc/openauto/`
- Files are set with `OWNER_READ GROUP_READ` permissions (0644)
- Files are part of the `runtime` component package

## Runtime Behavior

On application startup:
1. `Cryptor::init()` attempts to load certificate from file paths (in order)
2. First successful read is used
3. If all file reads fail, embedded constant is used
4. Log message indicates which path was loaded
5. No application error occurs if files are missing (fallback works)

## Testing Checklist

- [ ] Build aasdk package
- [ ] Verify certificate files installed to `/etc/openauto/`
- [ ] Verify permissions (should be 0644)
- [ ] Test application startup logs certificate path
- [ ] Test custom certificate replacement
- [ ] Test missing certificate files (should fallback)
- [ ] Test Android Auto connection still works

## Migration Notes

**For Existing Systems:**
- No immediate action required
- Application continues using embedded certificates if files not present
- Installing updated aasdk package will deploy certificate files to `/etc/openauto/`

**For New Installations:**
- Certificate files automatically installed via debian package
- No manual configuration needed

## Security Considerations

- Default certificate is publicly available (from JVC Kenwood head units)
- Default certificate is **not confidential** and suitable for standard Android Auto
- Custom certificates can be deployed for vendor-specific requirements
- Certificate files installed to system directory require root access to modify

## Related Issues

- Resolves difficulty updating SSL certificates
- Enables vendor-specific certificate deployment
- Improves development workflow (no rebuild for certificate testing)

## Code Review Notes

- File I/O uses standard C++ streams (fstream, sstream)
- Error handling: empty string returned if file cannot be read
- Logging added for debugging certificate loading
- Maintains backward compatibility via embedded constant fallback
- CMake install rules follow debian packaging best practices

## Future Enhancements

Possible improvements:
- Add validation of certificate format before using
- Add certificate expiration warnings
- Support for certificate chains
- Environment variable override for certificate path
