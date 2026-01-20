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
     * @interface IUSBHub
     * @brief Device discovery and connection manager for Android Auto over USB.
     *
     * **Purpose:**
     * Monitors for Android devices connecting over USB, negotiates the Android Open Accessory
     * Protocol (AOAP), and provides a DeviceHandle for communication once a device is ready.
     *
     * **AOAP Handshake Overview:**
     * The AOAP protocol is a multi-step negotiation that allows an Android device to enter a
     * special "accessory mode" where it can communicate with a headunit (car infotainment system).
     * This involves:
     * 1. Device detection (standard USB VID/PID)
     * 2. Protocol version query
     * 3. Accessory identification (manufacturer, model, version strings)
     * 4. Accessory mode request
     * 5. Device re-enumeration in accessory mode
     * 6. Bulk endpoint establishment
     *
     * **Typical Device Lifecycle:**
     * ```
     * Time    Event                             Hub State
     * T+0ms   [USB Device plugged in]           Scanning connected devices
     * T+50ms  Device detected (Samsung Galaxy) Checking AOAP compatibility
     * T+100ms start(promise) called            Negotiating protocol version
     * T+150ms IUSBHub sends AOAP query        Waiting for device response
     * T+200ms Device responds (protocol v2)   Version confirmed
     * T+250ms send accessory strings          Identifying headunit
     * T+300ms send "Crankshaft" manufacturer  Accessory info sent
     * T+350ms request device enter AOA mode   Waiting for re-enum
     * T+500ms [Device re-enumerates]          New USB descriptors
     * T+550ms New device appears (AOA mode)   Opening bulk endpoints
     * T+600ms Bulk endpoints ready            Creating DeviceHandle
     * T+650ms promise->resolve(handle)        Connection ready for auth
     * ```
     *
     * **Promise Contract:**
     * - resolve(DeviceHandle): AOAP negotiation complete, device ready; caller takes ownership of handle
     * - reject(Error):
     *   - DEVICE_NOT_FOUND: No compatible device detected after timeout
     *   - AOAP_PROTOCOL_ERROR: Device doesn't support AOAP (old Android version)
     *   - USB_PERMISSION_DENIED: Linux only—libusb permission issue (udev rule not set)
     *   - USB_ERROR: Bus error, device unplugged during negotiation
     *
     * **Implementation Notes:**
     * - Must handle multiple devices (only one promise at a time, but may queue)
     * - Must retry on protocol errors (some devices are slow to respond)
     * - Timeout typically 10 seconds (configurable)
     *
     * **Usage Example:**
     * ```cpp
     * auto hub = std::make_shared<USBHub>(usb_wrapper, io_service);
     * auto promise = std::make_shared<Promise<DeviceHandle>>();
     * promise->onResolve([](const DeviceHandle& handle) {
     *     // Device ready; create USBTransport for communication
     *     auto transport = std::make_shared<USBTransport>(handle);
     *     // Proceed with authentication and message exchange
     * });
     * promise->onReject([](const Error& err) {
     *     LOG(ERROR) << "Device not compatible: " << err.message();
     * });
     * hub->start(promise);
     * // Later, to cancel:
     * hub->cancel();
     * ```
     *
     * @see DeviceHandle for the resolved value type
     * @see USBTransport for communication after AOAP negotiation
     * @see IAccessoryModeQueryChain for protocol negotiation details
     */
    class IUSBHub {
    public:
      /// Shared pointer to IUSBHub
      using Pointer = std::shared_ptr<IUSBHub>;

      /// Promise type: resolves with a DeviceHandle or Error
      using Promise = io::Promise<DeviceHandle>;

      IUSBHub() = default;

      virtual ~IUSBHub() = default;

      /**
       * @brief Start listening for Android devices and negotiate AOAP.
       *
       * **Behaviour:**
       * - Non-blocking: initiates device scan asynchronously
       * - Completes when a device successfully enters accessory mode (bulk endpoints open)
       * - Only one start() should be active at a time; calling twice queues the second request
       * - Timeout typically 10 seconds per device attempt
       *
       * **AOAP Negotiation Steps:**
       * 1. Enumerate connected USB devices
       * 2. For each device matching vendor/product ID:
       *    a. Query protocol version (supports v1 or v2)
       *    b. Send accessory metadata (manufacturer="Crankshaft", model="Headunit", etc.)
       *    c. Request device enter accessory mode (control transfer)
       *    d. Wait for device re-enumeration (typically 500–2000ms)
       *    e. Open bulk IN/OUT endpoints
       * 3. Return DeviceHandle
       *
       * **Error Scenarios:**
       * - Device unplugged during negotiation → reject(DEVICE_DISCONNECTED)
       * - Device too old (Android < 4.1, no AOAP support) → reject(AOAP_NOT_SUPPORTED)
       * - Bulk endpoint not found after re-enum → reject(ENDPOINT_NOT_FOUND)
       * - Bus error (power loss, cable fault) → reject(USB_ERROR)
       *
       * **Thread Safety:**
       * - Safe to call from any thread
       * - Promise callbacks are invoked on the io_service strand
       * - Calling cancel() from another thread is safe
       *
       * **Example: Wait for Device with Timeout**
       * ```cpp
       * // Start listening
       * auto promise = std::make_shared<Promise>();
       * hub->start(promise);
       * 
       * // Set 15-second timeout
       * auto timer = std::make_shared<asio::deadline_timer>(io_service);
       * timer->expires_from_now(boost::posix_time::seconds(15));
       * timer->async_wait([hub](const boost::system::error_code&) {
       *     if (!promise->isResolved()) {
       *         hub->cancel();
       *         promise->reject(Error(ErrorCode::TIMEOUT));
       *     }
       * });
       * ```
       *
       * @param [in] promise Promise to resolve with DeviceHandle or reject with Error
       *
       * @see cancel() to stop waiting
       * @see DeviceHandle for the resulting handle type
       */
      virtual void start(Promise::Pointer promise) = 0;

      /**
       * @brief Cancel an active or pending device search.
       *
       * **Behaviour:**
       * - Stops monitoring for devices
       * - Any active promise is rejected with CANCELLED error
       * - Can be called multiple times (idempotent)
       * - Returns immediately; cleanup is asynchronous
       *
       * **When to Use:**
       * - User cancels connection attempt in UI
       * - Application shutting down
       * - Timeout expires (see example in start())
       * - Another device takes priority (e.g., multiple phones connected)
       *
       * **Side Effects:**
       * - Ongoing USB transactions are aborted (may generate USB errors on device)
       * - Any pending bulk transfers are cancelled
       * - Device is not affected (remains in accessory mode if negotiation was complete)
       *
       * **Thread Safety:**
       * - Safe to call from any thread at any time
       * - Safe to call even if start() has not been called
       * - Safe to call while start()'s promise callback is being invoked
       *
       * **Example: Timeout-based Cancellation**
       * ```cpp
       * hub->cancel();  // Will reject outstanding promise with CANCELLED
       * ```
       *
       * @see start() for initiating the search
       */
      virtual void cancel() = 0;
    };

}
