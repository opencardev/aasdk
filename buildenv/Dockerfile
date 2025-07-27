ARG ARCH="armhf"
FROM --platform=${PLATFORM} debian:bookworm AS aasdk
RUN apt-get update
RUN apt-get -y install cmake build-essential git
RUN apt-get -y install gcc-arm-linux-gnueabihf cpp-arm-linux-gnueabihf g++-arm-linux-gnueabihf protobuf-compiler
RUN apt-get -y install gcc-11-base libc6 libgcc1 libicu72 libidn2-0 libstdc++6 libunistring2

# These are all the libboost requirements. It would be shorter if libboost-log-dev wasn't a disaster that's being dealt with manually.
RUN apt-get install -y libprotobuf-dev:$ARCH libusb-1.0.0-dev:$ARCH libssl-dev:$ARCH libboost-dev:$ARCH libboost-system-dev:$ARCH libboost-atomic1.81.0:$ARCH libboost-chrono1.81.0:$ARCH libboost-date-time1.81.0:$ARCH libboost-filesystem1.81.0:$ARCH libboost-regex1.81.0:$ARCH libboost-system1.81.0:$ARCH libboost-thread1.81.0:$ARCH libboost-filesystem1.81-dev:$ARCH libboost-thread1.81-dev:$ARCH libboost-date-time1.81-dev:$ARCH

COPY / /src

WORKDIR /src

# Import resources
COPY ./buildenv/patch-libboost-log-deb.sh /src/resources/patch-libboost-log-deb.sh
COPY ./buildenv/entrypoint.sh /entrypoint.sh

# Patch libboost-log-dev and install
RUN chmod +x ./resources/patch-libboost-log-deb.sh
RUN ./resources/patch-libboost-log-deb.sh $ARCH

# Make Executable
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]