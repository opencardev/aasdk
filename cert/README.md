# SSL/TLS Certificate Configuration

## Overview

This directory contains the SSL/TLS certificate and private key used by AASDK for Android Auto communication. These files are used during the SSL handshake between the head unit and the Android device.

## Files

- **headunit.crt**: X.509 certificate (Google Automotive Link issued to JVC Kenwood)
- **headunit.key**: RSA private key (2048-bit)
- **.gitignore**: Protects custom certificates from being committed

## Default Certificate

The default certificate included (`headunit.crt`) is a publicly available certificate used in Android Auto head units. It is **not a secret** and is suitable for standard Android Auto functionality.

## Custom Certificates

You can replace the default certificate with your own:

1. Create custom certificate files:
   - `custom_headunit.crt`
   - `custom_headunit.key`

2. These will be automatically ignored by git (see `.gitignore`)

3. The application will try loading certificates from these paths in order:
   - `/etc/openauto/headunit.crt` (installed location)
   - `/usr/share/aasdk/cert/headunit.crt` (alternative system location)
   - `./cert/headunit.crt` (development directory)
   - `../cert/headunit.crt` (alternative development directory)

4. If no certificate file is found, the application falls back to the embedded certificate in the code

## Installation

When building the aasdk package, the certificate files are automatically installed to `/etc/openauto/` with read-only permissions:

```bash
# Installed by debian package:
/etc/openauto/headunit.crt  (owner: root, permissions: 0644)
/etc/openauto/headunit.key  (owner: root, permissions: 0644)
```

## Security Notes

- The default certificate is public and **not confidential**
- Custom certificates can be used for specific vendor requirements
- The private key should be protected if using a custom certificate
- Certificate files are loaded at runtime, allowing updates without recompilation

## Usage

The `Cryptor` class in AASDK automatically loads these certificates during initialization:

```cpp
// Certificate loading happens in Cryptor::init()
std::string certContent = loadCertificate();  // Tries file paths, falls back to embedded
std::string keyContent = loadPrivateKey();    // Tries file paths, falls back to embedded
```

Logging is enabled to show which certificate path was successfully loaded:
```
[INFO] [Cryptor] Loaded certificate from: /etc/openauto/headunit.crt
```

## Development

During development, place your certificate files in the `cert/` directory:
- `cert/headunit.crt`
- `cert/headunit.key`

The application will automatically find them using the relative path fallback mechanism.

## Updating Certificates

To update the certificate on an installed system:

1. Copy new certificate files to `/etc/openauto/`:
   ```bash
   sudo cp new_headunit.crt /etc/openauto/headunit.crt
   sudo cp new_headunit.key /etc/openauto/headunit.key
   sudo chmod 644 /etc/openauto/headunit.*
   ```

2. Restart the service:
   ```bash
   sudo systemctl restart openauto
   ```

No recompilation required!

## Troubleshooting

If the certificate fails to load:

1. Check file permissions:
   ```bash
   ls -l /etc/openauto/headunit.*
   ```

2. Verify certificate format (should be PEM):
   ```bash
   openssl x509 -in /etc/openauto/headunit.crt -text -noout
   ```

3. Check application logs for certificate loading messages

4. The application will fall back to the embedded certificate if files cannot be read
