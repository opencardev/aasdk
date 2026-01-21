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

/**
 * @file Transport.cpp
 * @brief Base transport layer for frame-level communication with Android device.
 *
 * This implementation provides the low-level frame receive/send buffering:
 * - Manages receive buffer and distributes available data to waiting requests
 * - Queues send operations for transmission by subclass implementations
 * - Handles strand-based serialisation for thread-safe buffer access
 * - Provides size-based flow control (wait until N bytes available)
 *
 * Subclasses (USBTransport, TCPTransport) implement enqueueReceive/enqueueSend
 * to handle physical transmission (USB transfers, TCP sockets).
 *
 * Design: CircularBuffer holds received frames; receive() waits for N bytes
 * before resolving. Multiple receive operations can be pending with different
 * size requirements (e.g., one waiting for 4-byte header, another for 1024-byte
 * payload). sendQueue_ ensures sends complete before starting next one.
 *
 * Typical scenario (frame reception):
 *   1. Messenger calls receive(4) to read frame header
 *   2. Queued on receiveStrand_; check buffer - needs more data
 *   3. Call enqueueReceive(1500) to read from USB/TCP
 *   4. 1500 bytes arrive in receivedDataSink_
 *   5. receiveHandler() distributes: 4 bytes to header request, 1496 to next
 *   6. Header promise resolves; Messenger parses size field
 *   7. Messenger calls receive(size) for payload
 *   8. Payload data already in buffer; resolve immediately
 */

#include <aasdk/Common/Log.hpp>
#include <aasdk/Common/ModernLogger.hpp>
#include <aasdk/Transport/Transport.hpp>


namespace aasdk {
  namespace transport {

    Transport::Transport(boost::asio::io_service &ioService)
        : receiveStrand_(ioService), sendStrand_(ioService) {}

    /**
     * @brief Queue a receive request for N bytes from the transport.
     *
     * This method asynchronously waits for the specified number of bytes to
     * become available in the receive buffer. When sufficient data is available,
     * the promise is resolved with that data. This enables size-based flow control
     * where the caller reads fixed-size headers first, then payload frames.
     *
     * Scenario: Receiving a frame from USB transport:
     *   - T+0ms: Messenger calls receive(4) for header size
     *   - T+0ms: Buffer empty; enqueueReceive(1500) dispatches to USB layer
     *   - T+0ms: USB bulk transfer scheduled to endpoint
     *   - T+15ms: 1500 bytes arrive; receiveHandler(1500) called
     *   - T+15ms: distributeReceivedData() checks: 4 bytes available for header
     *   - T+15ms: Promise resolved with 4-byte header
     *   - Messenger parses header, determines 512-byte payload follows
     *   - Messenger calls receive(512) for payload
     *   - T+15ms: distributeReceivedData() finds 1496 bytes in buffer
     *   - T+15ms: 512 bytes consumed, remaining 984 stay buffered
     *   - Next frame can be partially available before payload request arrives
     *
     * Thread Safety: All buffer operations dispatched to receiveStrand_.
     *
     * @param size Number of bytes to wait for before resolving promise
     * @param promise Promise that resolves when N bytes available, or rejects on error
     */
    void Transport::receive(size_t size, ReceivePromise::Pointer promise) {
      AASDK_LOG_TRANSPORT(debug, "receive()");
      receiveStrand_.dispatch([this, self = this->shared_from_this(), size, promise = std::move(promise)]() mutable {
        receiveQueue_.emplace_back(size, std::move(promise));

        if (receiveQueue_.size() == 1) {
          try {
            AASDK_LOG_TRANSPORT(debug, "Distribute received data.");
            this->distributeReceivedData();
          }
          catch (const error::Error &e) {
            // Due to the design of the messaging system, we don't really need to raise an error - debug it is
            AASDK_LOG_TRANSPORT(debug, "Reject receive promise.");
            this->rejectReceivePromises(e);
          }
        }
      });
    }

    void Transport::receiveHandler(size_t bytesTransferred) {
      try {
        AASDK_LOG_TRANSPORT(debug, "receiveHandler()");
        receivedDataSink_.commit(bytesTransferred);
        this->distributeReceivedData();
      }
      catch (const error::Error &e) {
        // Due to the design of the messaging system, we don't really need to raise an error - debug it is
        AASDK_LOG_TRANSPORT(debug, "Rejecting promise.");
        this->rejectReceivePromises(e);
      }
    }

    void Transport::distributeReceivedData() {
      AASDK_LOG_TRANSPORT(debug, "distributeReceivedData()");
      for (auto queueElement = receiveQueue_.begin(); queueElement != receiveQueue_.end();) {
        if (receivedDataSink_.getAvailableSize() < queueElement->first) {
          AASDK_LOG_TRANSPORT(debug, "Receiving from buffer.");
          auto buffer = receivedDataSink_.fill();
          this->enqueueReceive(std::move(buffer));

          break;
        } else {
          auto data(receivedDataSink_.consume(queueElement->first));
          AASDK_LOG_TRANSPORT(debug, "Resolve and clear message.");
          queueElement->second->resolve(std::move(data));
          queueElement = receiveQueue_.erase(queueElement);
        }
      }
    }

    void Transport::rejectReceivePromises(const error::Error &e) {
      for (auto &queueElement: receiveQueue_) {
        queueElement.second->reject(e);
      }

      receiveQueue_.clear();
    }

    void Transport::send(common::Data data, SendPromise::Pointer promise) {
      sendStrand_.dispatch(
          [this, self = this->shared_from_this(), data = std::move(data), promise = std::move(promise)]() mutable {
            sendQueue_.emplace_back(std::move(data), std::move(promise));

            if (sendQueue_.size() == 1) {
              this->enqueueSend(sendQueue_.begin());
            }
          });
    }

  }
}
