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
     * @brief Concrete ITransport implementation for USB bulk transfer communication.
     *
     * USBTransport handles Android Open Accessory Protocol (AOAP) communication over
     * USB bulk endpoints. It manages the low-level USB transfers (send/receive) using
     * libusb, with configurable timeouts and error handling.
     *
     * AOAP Protocol Overview:
     * - Google vendor ID: 0x18D1
     * - Accessory mode product ID: 0x4EE1
     * - Standard accessory device provides IN/OUT bulk endpoints
     * - Messages are framed with 4-byte length header + payload
     * - Each transfer can carry multiple frames or partial frames
     *
     * Typical USB flow (single message lifecycle):
     *   T+0ms    receive(1024, promise) queued; USB IN endpoint monitored
     *   T+10ms   [Phone sends 100-byte message]
     *   T+15ms   USB driver receives data on IN endpoint
     *   T+20ms   USBTransport.receiveHandler() invoked with 100 bytes
     *   T+25ms   promise->resolve(100-byte data)
     *   T+30ms   send(50-byte response, sendPromise) called
     *   T+35ms   Data queued to USB OUT endpoint
     *   T+50ms   USB driver sends data
     *   T+55ms   sendHandler() invoked; 50 bytes confirmed sent
     *   T+60ms   sendPromise->resolve()
     *
     * Timeouts:
     * - Send timeout: 10 seconds (cSendTimeoutMs = 10000)
     * - Receive timeout: Infinite (cReceiveTimeoutMs = 0)
     *
     * Error Recovery:
     * - If USB transfer fails (cable unplugged, device reset), stop() is called
     * - All pending receive/send promises are rejected with TRANSPORT_ERROR
     * - Connection recovery is handled by application (reconnect via IUSBHub)
     *
     * Thread Safety: Inherits from Transport base class. All receive/send operations
     * are serialised; safe to call from any thread.
     */
    class USBTransport : public Transport {
    public:
      /**
       * @brief Construct a USBTransport with an AOAP device handle.
       *
       * @param ioService Boost ASIO io_service for async operations
       * @param aoapDevice AOAP device handle (from IUSBHub discovery)
       *
       * Note: ioService must outlive the USBTransport instance.
       */
      USBTransport(boost::asio::io_service &ioService, usb::IAOAPDevice::Pointer aoapDevice);

      /**
       * @brief Stop USB transport and reject pending operations.
       *
       * Closes USB endpoints and cancels any in-flight transfers. All pending
       * receive/send promises are rejected.
       */
      void stop() override;

    private:
      /// Internal: queue a receive operation on USB IN endpoint
      void enqueueReceive(common::DataBuffer buffer) override;

      /// Internal: queue send from the send queue to USB OUT endpoint
      void enqueueSend(SendQueue::iterator queueElement) override;

      /// Internal: send data (potentially in chunks) on USB OUT endpoint
      void doSend(SendQueue::iterator queueElement, common::Data::size_type offset);

      /// Internal: handle USB OUT transfer completion; check for errors, resolve promise
      void sendHandler(SendQueue::iterator queueElement, common::Data::size_type offset, size_t bytesTransferred);

      /// AOAP device handle (provides bulk IN/OUT endpoint access)
      usb::IAOAPDevice::Pointer aoapDevice_;

      /// Timeout for send operations (10 seconds)
      static constexpr uint32_t cSendTimeoutMs = 10000;

      /// Timeout for receive operations (infinite: 0)
      static constexpr uint32_t cReceiveTimeoutMs = 0;
    };

}
