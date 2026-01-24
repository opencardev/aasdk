// This file is part of aasdk library project.
// Copyright (C) 2018 f1x.studio (Michal Szwaj)
// Copyright (C) 2024 CubeOne (Simon Dean - simon.dean@cubeone.co.uk)
// Copyright (C) 2026 OpenCarDev (Matthew Hilton - matthilton2005@gmail.com)
//
// aasdk is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// aasdk is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with aasdk. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <memory>
#include <functional>
#include <aasdk/USB/IUSBWrapper.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/IO/Promise.hpp>


namespace aasdk::usb {

    /**
     * @class IUSBHub
     * @brief Abstract interface for USB device discovery and hotplug handling.
     *
     * IUSBHub manages USB device enumeration and hotplug detection. It continuously
     * monitors connected devices and invokes callbacks when Android Auto compatible
     * devices are attached or detached. Implementations interact with libusb to scan
     * the USB bus and handle device lifecycle events.
     *
     * Android Open Accessory Protocol (AOAP) devices:
     * - Vendor ID: 0x18D1 (Google)
     * - Product ID: 0x4EE1 (Accessory mode)
     * - After handshake, device may reboot into accessory mode
     *
     * Typical flow (phone connects to accessory):
     *   T+0ms    [USB device plugged in]
     *   T+50ms   IUSBHub detects vendor/product ID match
     *   T+60ms   IUSBHub.start(promise) called by application
     *   T+100ms  [libusb enumerates device]
     *   T+150ms  DeviceHandle acquired, promise->resolve(handle) invoked
     *   T+200ms  Application creates USBTransport with handle
     *   T+300ms  [Android handshake begins]
     *
     * Thread Safety: Callbacks (device discovered, detached) may be invoked from
     * internal hotplug monitor thread. Safe to call cancel() from any thread.
     *
     * Error Handling: If USB subsystem error occurs (no device found, timeout),
     * promise is rejected with USB_HUB_ERROR or similar. Multiple start() calls
     * can be queued; each resolves independently as devices are found.
     */
    class IUSBHub {
    public:
      using Pointer = std::shared_ptr<IUSBHub>;

      /// Promise type for device discovery: resolves with DeviceHandle when device found
      using Promise = io::Promise<DeviceHandle>;

      IUSBHub() = default;

      virtual ~IUSBHub() = default;

      /**
       * @brief Start monitoring for Android Auto compatible USB devices.
       *
       * Asynchronously scans the USB bus for AOAP devices (Google vendor ID 0x18D1,
       * product ID 0x4EE1) or devices that support accessory mode. Returns immediately;
       * promise resolves when a compatible device is found or error occurs.
       *
       * Multiple start() calls can be queued. Each resolves independently as devices
       * are discovered. If a device is already connected, promise resolves quickly.
       *
       * @param promise Promise to resolve with DeviceHandle when compatible device found
       *
       * Promise lifecycle:
       * - resolve(DeviceHandle): Device acquired and ready for transport layer
       * - reject(error::Error): USB enumeration failed, timeout, or cancel() called
       */
      virtual void start(Promise::Pointer promise) = 0;

      /**
       * @brief Cancel pending device discovery and reject outstanding promises.
       *
       * Stops monitoring for new devices and rejects all pending start() promises
       * with USB_HUB_CANCELLED. Safe to call multiple times (idempotent).
       * After cancel(), new start() calls can begin new discovery.
       */
      virtual void cancel() = 0;
    };

}
