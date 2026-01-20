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

#include <utility>
#include <boost/asio.hpp>
#include <aasdk/Transport/Transport.hpp>
#include <aasdk/USB/IAOAPDevice.hpp>


namespace aasdk::transport {

    /**
     * @class USBTransport
     * @brief USB Accessory Mode transport layer for Android Auto protocol communication.
     *
     * **Purpose:**
     * Implements direct USB communication with Android devices that support Android Open Accessory
     * Protocol (AOAP). This is the primary transport for hardwired car infotainment systems with
     * direct USB connections to devices in the vehicle.
     *
     * **USB Topology:**
     * ```
     *   Android Device (USB Client)
     *             │
     *             ├─ [Micro USB / USB-C]
     *             │
     *   Headunit (USB Host / AOAP Accessory)
     *             │
     *        USBTransport
     *             │
     *        Bulk Endpoints
     *   IN (device→headunit)   [4KB/transfer]
     *   OUT (headunit→device)  [4KB/transfer]
     * ```
     *
     * **Key Characteristics:**
     * - **Direct connection**: No router or switch; dedicated cable
     * - **High speed**: USB 2.0 (480 Mbps) or USB 3.0+ (5+ Gbps)
     * - **Low latency**: Typical message round-trip < 10ms
     * - **Reliable**: Automatic retry on CRC error; no message loss
     * - **Tethering friendly**: Can charge device while communicating
     *
     * **AOAP Background:**
     * Android Open Accessory Protocol (AOAP) is Google's specification for USB accessories
     * (headunits, car dashboards, etc.) to communicate with Android devices. Key points:
     * - Device starts in normal USB mode (VID:PID = Google's standard)
     * - Headunit (host) queries device protocol version (v1 or v2)
     * - Headunit sends accessory identification strings (manufacturer, model, version)
     * - Headunit issues a control transfer requesting accessory mode
     * - Device re-enumerates with new VID:PID (assigned by manufacturer, e.g., Crankshaft)
     * - After re-enum, bulk IN/OUT endpoints are available for frame exchange
     * - No SSL/TLS needed (assumed to be a trusted physical connection)
     *
     * **Lifecycle: Boot-up to Active Communication**
     * ```
     * Time    Event                                   USBTransport State
     * T+0ms   [Device plugged in]                     Not instantiated
     * T+100ms IUSBHub detects device                 Hub negotiating AOAP
     * T+200ms Device enters AOAP mode, re-enums      Re-enumeration
     * T+500ms Device appears with new VID:PID        Device ready
     * T+550ms USBTransport created with IAOAPDevice Connected to device
     * T+600ms receive(4096, promise) called          Bulk IN queued
     * T+650ms [Device sends first frame]             Data arrives
     * T+700ms promise->resolve(frameData)            Handshake received
     * T+750ms send(authResponse, sendPromise)        Bulk OUT queued
     * T+850ms [Frame transmitted to device]          Data sent
     * T+900ms sendPromise->resolve()                 Ready for next exchange
     * ```
     *
     * **Bulk Transfer Mechanics:**
     * - Bulk IN: Receives 4–64KB frame from device (typical 4KB)
     * - Bulk OUT: Sends 4–64KB frame to device (typical 4KB)
     * - USB scheduler may batch multiple frames or introduce latency
     * - No guaranteed delivery interval; depends on device and host load
     *
     * **Timeout Strategy:**
     * - Send timeout: 10 seconds per frame (if device stalls or crashes)
     * - Receive timeout: Infinite (wait forever for device to send)
     * - Rationale: Device may be idle for long periods; headunit waits for wake-up
     *
     * **Error Scenarios & Recovery:**
     * ```
     * Scenario 1: User unplugs device
     *   → Bulk endpoint error (DEVICE_DISCONNECTED)
     *   → All pending promises rejected
     *   → stop() called; transport cleanup
     *   → New IUSBHub::start() to wait for next device
     *
     * Scenario 2: Device crashes mid-transfer
     *   → Send timeout (10s) expires
     *   → Error reported: TRANSPORT_ERROR
     *   → Connection considered lost; stop() required
     *
     * Scenario 3: Temporary USB bus congestion
     *   → Frame sent successfully, but with added latency
     *   → No error (transparent to application)
     * ```
     *
     * **Thread Safety:**
     * - receive()/send()/stop() can be called from any thread
     * - Bulk operations run on boost::asio strands
     * - Promise callbacks invoked on the strand, so must be reentrant or lightweight
     *
     * **Performance Characteristics:**
     * ```
     * Metric                     Typical Value
     * Message latency (full RTT) 5–20ms
     * Throughput                 1–10 MB/s (depends on frame size & device load)
     * CPU usage (idle)           <1%
     * CPU usage (active transfer) 2–5%
     * Power draw                 0.5–1W (from USB port)
     * ```
     *
     * **Usage Example (Headunit Integration):**
     * ```cpp
     * // After device successfully enters AOAP mode
     * auto aoapDevice = std::make_shared<AOAPDevice>(deviceHandle);
     * auto transport = std::make_shared<USBTransport>(io_service, aoapDevice);
     *
     * // Start receiving frames
     * auto receivePromise = std::make_shared<ReceivePromise>();
     * receivePromise->onResolve([](const Data& frame) {
     *     LOG(INFO) << "Received " << frame.size() << " bytes from device";
     *     // Process Android Auto protocol frame...
     * });
     * receivePromise->onReject([](const Error& err) {
     *     LOG(ERROR) << "USB receive failed: " << err.message();
     *     // Device disconnected or error; restart connection
     * });
     * transport->receive(4096, receivePromise);
     *
     * // Send response (asynchronously)
     * auto responseData = constructAndroidAutoFrame(...);
     * auto sendPromise = std::make_shared<SendPromise>();
     * sendPromise->onResolve([]() {
     *     LOG(INFO) << "Frame sent to device";
     * });
     * transport->send(responseData, sendPromise);
     * ```
     *
     * **Connection Lifespan (Typical Car Scenario):**
     * ```
     * 1. Driver plugs phone into USB port
     * 2. Headunit detects → AOAP negotiation → USBTransport created
     * 3. Authentication → permission prompts on phone
     * 4. Active communication: navigation, media, messages, etc.
     * 5. User unplugs → USB error → connection ends → restart listening
     * 6. Or: User drives out of range (unlikely in car) → timeout
     * ```
     *
     * @see ITransport for the interface contract
     * @see Transport for base class (common logic)
     * @see IAOAPDevice for USB endpoint abstraction
     * @see IUSBHub for AOAP negotiation
     */
    class USBTransport : public Transport {
    public:
      /**
       * @brief Construct a USBTransport connected to an AOAP device.
       *
       * **Parameters:**
       * - ioService: Boost ASIO io_service for async bulk transfers
       * - aoapDevice: IAOAPDevice with open bulk IN/OUT endpoints
       *
       * **Preconditions:**
       * - aoapDevice must have completed AOAP negotiation
       * - Bulk endpoints (IN/OUT) must be open and ready
       * - ioService must outlive this transport
       *
       * **Postconditions:**
       * After construction, the transport is ready to receive()/send(). No initial I/O
       * is performed; the caller must invoke receive() to begin.
       *
       * **Thread Safety:**
       * Constructor may be called from any thread.
       *
       * **Example:**
       * ```cpp
       * auto transport = std::make_shared<USBTransport>(io_service, aoapDevice);
       * ```
       */
      USBTransport(boost::asio::io_service &ioService, usb::IAOAPDevice::Pointer aoapDevice);

      /**
       * @brief Stop USB communication and release resources.
       *
       * **Behaviour:**
       * - Cancels all pending bulk transfers (IN and OUT)
       * - Rejects all outstanding receive/send promises
       * - Closes bulk endpoints gracefully
       * - Safe to call multiple times
       *
       * **Side Effects:**
       * - Device may see USB connection drop or stall
       * - Undelivered messages are discarded
       * - After stop(), the transport is unusable; must create new USBTransport for reconnection
       *
       * **Typical Usage:**
       * ```cpp
       * transport->stop();  // Cleanup before destruction
       * ```
       *
       * @see ITransport::stop() for interface contract
       */
      void stop() override;

    private:
      /**
       * @brief Internal: Queue an asynchronous bulk IN transfer.
       *
       * Posts a read operation on the bulk IN endpoint to receive up to @p buffer.size()
       * bytes. When data arrives, the promise is resolved with the received data.
       * If an error occurs (disconnection, timeout, etc.), the promise is rejected.
       *
       * @param [in] buffer Output buffer for received data
       */
      void enqueueReceive(common::DataBuffer buffer) override;

      /**
       * @brief Internal: Initiate transmission of a queued message.
       *
       * Pops a (Data, Promise) pair from the send queue and starts a bulk OUT transfer.
       * Handles multi-frame sends (if data > max USB frame size) and promise resolution.
       *
       * @param [in] queueElement Iterator to send queue element being transmitted
       */
      void enqueueSend(SendQueue::iterator queueElement) override;

      /**
       * @brief Internal: Send a fragment of data via USB bulk OUT.
       *
       * Continues sending a (possibly large) message by breaking it into USB frames
       * and issuing sequential bulk transfers. Called recursively until all data sent.
       *
       * @param [in] queueElement Iterator to send queue element
       * @param [in] offset Byte offset within the data to send (for multi-frame sends)
       */
      void doSend(SendQueue::iterator queueElement, common::Data::size_type offset);

      /**
       * @brief Callback: Handle completion of a bulk OUT transfer.
       *
       * Called by boost::asio when a bulk OUT frame completes. Updates offset and
       * either sends the next frame or resolves the promise if complete.
       *
       * @param [in] queueElement Iterator to send queue element
       * @param [in] offset Current byte offset
       * @param [in] bytesTransferred Bytes sent in this transfer
       */
      void sendHandler(SendQueue::iterator queueElement, common::Data::size_type offset, size_t bytesTransferred);

      /// AOAP device with open bulk endpoints
      usb::IAOAPDevice::Pointer aoapDevice_;

      /// Timeout for bulk OUT (send) operations: 10 seconds
      static constexpr uint32_t cSendTimeoutMs = 10000;

      /// Timeout for bulk IN (receive) operations: infinite (wait forever)
      static constexpr uint32_t cReceiveTimeoutMs = 0;
    };

}
