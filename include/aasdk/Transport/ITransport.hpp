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
#include <aasdk/Common/Data.hpp>
#include <aasdk/IO/Promise.hpp>


namespace aasdk::transport {

    /**
     * @interface ITransport
     * @brief Abstract transport layer interface for Android Auto Secure Data Stream (AASD) communication.
     *
     * **Purpose:**
     * Provides a unified, asynchronous interface for bidirectional data transmission regardless of the underlying
     * physical transport (USB, TCP/IP, Bluetooth). Implements the Promise-based pattern for non-blocking operations.
     *
     * **Design Pattern:**
     * - Promise-based async operations (instead of callbacks or threads)
     * - Message-oriented streaming (not byte-oriented)
     * - Graceful stop semantics for cleanup
     *
     * **Implementations:**
     * - `USBTransport`: USB accessory mode (AOAP) for direct device connections
     * - `TCPTransport`: TCP/IP with optional SSL/TLS for wireless or remote connectivity
     *
     * **Thread Safety:**
     * - receive() and send() can be called from any thread
     * - Promise callbacks will be invoked on the io_service strand they were created with
     * - stop() must be called before destruction to ensure cleanup
     *
     * **Scenario: Device Connection Lifecycle**
     * ```
     * Time    Event                          Transport State
     * T+0ms   USBDevice plugged in          Listening for connection
     * T+50ms  ITransport created            Ready to receive
     * T+100ms receive(4096, promise1)       Waiting for handshake from phone
     * T+150ms [Phone connects via USB]      Data arrives asynchronously
     * T+200ms promise1->resolve(handshake) Handshake received
     * T+210ms send(auth_response, promise2) Sending authentication to phone
     * T+300ms promise2->resolve()          Response sent successfully
     * T+5000ms receive(4096, promise3)      Waiting for next message...
     * T+8000ms [Network degradation]       (For TCPTransport, timeout triggers)
     * T+8100ms promise3->reject(error)     Connection lost
     * T+8200ms stop()                       Transport cleaned up
     * ```
     *
     * **Example Usage (USB Scenario):**
     * ```cpp
     * auto transport = std::make_shared<USBTransport>(...);
     * auto receivePromise = std::make_shared<Promise<Data>>();
     * receivePromise->onResolve([](const Data& data) {
     *     // Handle incoming Android Auto message (4KB)
     * });
     * receivePromise->onReject([](const Error& err) {
     *     // Handle USB disconnect or timeout
     * });
     * transport->receive(4096, receivePromise);
     * ```
     *
     * **Example Usage (TCP Scenario with SSL):**
     * ```cpp
     * auto transport = std::make_shared<TCPTransport>("192.168.1.100", 5037, ssl_wrapper);
     * auto sendPromise = std::make_shared<Promise<void>>();
     * sendPromise->onResolve([]() {
     *     // Message sent to headunit
     * });
     * transport->send(auth_response, sendPromise);
     * ```
     *
     * @see USBTransport for USB-based communication
     * @see TCPTransport for wireless TCP/IP communication
     * @see io::Promise for async operation semantics
     */
    class ITransport {
    public:
      /// Shared pointer type for ITransport instances
      using Pointer = std::shared_ptr<ITransport>;

      /// Promise type for asynchronous receive operations, resolves with received Data
      using ReceivePromise = io::Promise<common::Data>;

      /// Promise type for asynchronous send operations, resolves when data is transmitted
      using SendPromise = io::Promise<void>;

      ITransport() = default;

      virtual ~ITransport() = default;

      /**
       * @brief Asynchronously receive up to @p size bytes of data.
       *
       * **Behaviour:**
       * - Non-blocking: returns immediately; completion signalled via promise
       * - Exactly one ReceivePromise should be outstanding per transport instance (though multiple
       *   calls don't error, the last one overwrites)
       * - Data is Android Auto protocol frames, not raw bytes; typical sizes are 4KB to 64KB
       *
       * **Promise Lifecycle:**
       * - resolve(Data): Data received successfully; caller owns the Data object
       * - reject(Error): I/O error (USB disconnect, TCP timeout, SSL failure, etc.)
       *
       * **Error Cases:**
       * - TRANSPORT_DISCONNECTED: Physical link lost (USB unplugged, TCP connection closed)
       * - TRANSPORT_TIMEOUT: No data arrived within configured timeout (TCP only)
       * - TRANSPORT_ERROR: SSL/TLS handshake failure or protocol violation
       *
       * **Typical Flow (USB):**
       * 1. Transport receives 4096 bytes from Android device in USB bulk transfer
       * 2. Promise resolves with those bytes
       * 3. Caller parses protocol, responds with send()
       * 4. Next receive() is queued
       *
       * @param [in] size Maximum bytes to receive (e.g., 4096 for frame receive)
       * @param [in] promise Promise to resolve with received data or error
       *
       * @see SendPromise for corresponding send operation
       */
      virtual void receive(size_t size, ReceivePromise::Pointer promise) = 0;

      /**
       * @brief Asynchronously transmit data.
       *
       * **Behaviour:**
       * - Non-blocking: returns immediately; completion signalled via promise
       * - Data is queued internally; multiple send() calls may be batched
       * - For USB: data is split into USB packets (512 bytes, 64 bytes depending on speed)
       * - For TCP: data is written to socket; Nagle's algorithm may delay send
       *
       * **Promise Lifecycle:**
       * - resolve(): All data successfully transmitted (or queued for transmission)
       * - reject(Error): Failed to queue or transmit (buffer full, connection lost, etc.)
       *
       * **Error Cases:**
       * - TRANSPORT_DISCONNECTED: Link lost before all data could be sent
       * - TRANSPORT_ERROR: USB endpoint stalled, TCP connection reset
       * - BUFFER_OVERFLOW: Send queue full (implementation-specific limit, typically 1MB)
       *
       * **Typical Flow (Authentication Message):**
       * ```
       * // Construct authentication response (binary protocol)
       * common::Data auth_response = ...;
       * auto sendPromise = std::make_shared<SendPromise>();
       * sendPromise->onResolve([]() {
       *     // Safe to reuse the memory; device has received it
       * });
       * transport->send(auth_response, sendPromise);
       * ```
       *
       * @param [in] data Payload to send (ownership transferred; do not modify after call)
       * @param [in] promise Promise to resolve when transmission is complete or error occurs
       *
       * @see ReceivePromise for corresponding receive operation
       */
      virtual void send(common::Data data, SendPromise::Pointer promise) = 0;

      /**
       * @brief Gracefully stop the transport and release resources.
       *
       * **Behaviour:**
       * - Stops accepting new receive()/send() calls
       * - Closes underlying connection (USB endpoint, TCP socket, SSL session)
       * - Any pending promises are rejected with TRANSPORT_STOPPED error
       * - Safe to call multiple times (idempotent)
       * - After stop(), the transport is no longer usable; create a new instance to reconnect
       *
       * **Side Effects:**
       * - USB: Closes bulk IN/OUT endpoints; device may attempt to reconnect via AOAP
       * - TCP: Closes socket and SSL context; any ongoing TLS handshake is aborted
       * - Any remaining queued messages are dropped (not transmitted)
       *
       * **Typical Usage:**
       * ```cpp
       * // Cleanup on shutdown or error
       * if (transport) {
       *     transport->stop();
       *     transport.reset();  // Safe to destroy after stop()
       * }
       * ```
       *
       * **Thread Safety:**
       * - Safe to call from any thread (internal synchronization provided)
       * - May be called while receive()/send() are in progress; callbacks will be rejected
       *
       * @see ITransport destructor (may implicitly call stop() if not already called)
       */
      virtual void stop() = 0;
    };

}
