### RPI Cross Compile
The docker image in this folder is intended to be used to cross compile `aasdk` without having to configure your
host pc with multiarch or installing a toolchain.

#### Build
```bash
cd buildenv
sudo docker compose up --build
```
`aasdk*_armhf.deb` and `libaasdk.so*` will then be available in `buildenv/release`.