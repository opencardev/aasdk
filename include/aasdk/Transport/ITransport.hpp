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
     * @class ITransport
     * @brief Abstract interface for bidirectional data transport (USB or TCP).
     *
     * ITransport provides the low-level abstraction for sending and receiving raw
     * protocol data over a physical or virtual transport (USB, TCP). Implementations
     * handle the platform-specific details of hardware communication while providing
     * a consistent async Promise-based interface.
     *
     * Typical async lifecycle (USB example, ~60ms):
     *   T+0ms    device.connect() -> USB layer establishes endpoint
     *   T+10ms   Transport created with USB endpoints
     *   T+20ms   receive(1024, promise1) called
     *   T+50ms   [phone sends auth frame]
     *   T+60ms   promise1->resolve(authData) - callback invoked
     *   T+70ms   send(responseData, promise2) queued
     *   T+80ms   [USB transfer completes]
     *   T+85ms   promise2->resolve() - callback invoked
     *
     * Thread Safety: All methods are async and callback-based. Internal synchronization
     * ensures that callbacks are safe to call from any thread. Multiple concurrent
     * receive/send operations are queued and serialised.
     *
     * Error Handling: If transport error occurs (cable disconnected, socket closed,
     * timeout), any outstanding promises are rejected with an Error. After stop()
     * is called, no further operations are accepted.
     */
    class ITransport {
    public:
      using Pointer = std::shared_ptr<ITransport>;

      /// Promise type for receive operations: resolves with received data
      using ReceivePromise = io::Promise<common::Data>;

      /// Promise type for send operations: resolves with void (success/failure only)
      using SendPromise = io::Promise<void>;

      ITransport() = default;

      virtual ~ITransport() = default;

      /**
       * @brief Asynchronously receive up to size bytes from the transport.
       *
       * Returns immediately; promise resolves when data arrives or error occurs.
       * Multiple receive() calls queue promises; they resolve in FIFO order.
       *
       * @param size Maximum number of bytes to receive (driver may receive less)
       * @param promise Promise to resolve with received data (common::Data)
       *
       * Promise lifecycle:
       * - resolve(common::Data): Data received from transport
       * - reject(error::Error): Transport error, timeout, or stop() called
       */
      virtual void receive(size_t size, ReceivePromise::Pointer promise) = 0;

      /**
       * @brief Asynchronously send data to the transport.
       *
       * Returns immediately; promise resolves when data sent or error occurs.
       * Data is buffered internally; caller should not modify data after calling.
       * Multiple send() calls queue; they are serialised and sent in FIFO order.
       *
       * @param data Payload to transmit (std::vector<uint8_t>)
       * @param promise Promise to resolve when send completes
       *
       * Promise lifecycle:
       * - resolve(): Data successfully transmitted
       * - reject(error::Error): Transport error, send queue overflow, or stop() called
       */
      virtual void send(common::Data data, SendPromise::Pointer promise) = 0;

      /**
       * @brief Stop the transport and reject all pending operations.
       *
       * Gracefully shuts down the transport. All outstanding receive/send promises
       * are rejected with TRANSPORT_STOPPED. Subsequent receive/send calls are rejected.
       * Safe to call multiple times (idempotent).
       */
      virtual void stop() = 0;
    };

}
