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

#include <aasdk/TCP/ITCPEndpoint.hpp>
#include <aasdk/Transport/Transport.hpp>


namespace aasdk::transport {

    /**
     * @class TCPTransport
     * @brief Concrete ITransport implementation for network-based communication via TCP.
     *
     * TCPTransport provides Android Auto protocol communication over TCP sockets,
     * enabling wireless deployment and remote testing. Useful for:
     * - Wireless head units (Wi-Fi connected to phone)
     * - Simulator/emulator testing (localhost connections)
     * - Remote diagnostics and development
     *
     * Network Topology:
     * - Phone connects to accessory over Wi-Fi or cellular IP network
     * - TCP server listens on accessory (port configurable, typically 5037)
     * - Phone initiates connection; establishes secure tunnel
     * - Android Auto protocol frames flow over TCP (same as USB)
     *
     * Typical TCP connection flow (~500ms-2s latency due to network):
     *   T+0ms    [Phone joins Wi-Fi network, obtains accessory IP]
     *   T+100ms  Phone initiates TCP connect to accessory:5037
     *   T+200ms  Accessory accepts TCP connection; socket established
     *   T+300ms  TCPTransport created with socket handle
     *   T+400ms  receive(1024, promise) queued; TCP socket monitored
     *   T+600ms  [Phone sends auth message over TCP]
     *   T+800ms  TCPTransport.receiveHandler() invoked with message
     *   T+900ms  promise->resolve(authData)
     *   T+1000ms send(response, sendPromise) queued to TCP socket
     *   T+1200ms TCP driver transmits data
     *   T+1400ms sendHandler() confirms; sendPromise->resolve()
     *
     * Reliability: TCP provides retransmission and ordering guarantees;
     * packet loss is handled transparently by the kernel. Timeout on
     * receive/send indicates network disconnection or severe congestion.
     *
     * Wireless Considerations:
     * - Latency: 50-200ms typical (vs USB ~10-60ms)
     * - Bandwidth: Adequate for Android Auto (audio/video/nav streams)
     * - Stability: Depends on Wi-Fi signal strength and congestion
     * - Handoff: May disconnect if phone moves between networks
     *
     * Error Handling:
     * - If TCP connection drops (phone leaves Wi-Fi, router resets), stop() is called
     * - All pending promises are rejected with TRANSPORT_ERROR
     * - Application must restart discovery via network connection layer
     *
     * Thread Safety: Inherits from Transport base class. All operations
     * are async and serialised; safe to call from any thread.
     */
    class TCPTransport : public Transport {
    public:
      /**
       * @brief Construct a TCPTransport with a connected TCP socket.
       *
       * @param ioService Boost ASIO io_service for async socket operations
       * @param tcpEndpoint Connected TCP socket endpoint (phone or test client)
       *
       * Note: ioService and tcpEndpoint must remain valid until TCPTransport is destroyed.
       */
      TCPTransport(boost::asio::io_service &ioService, tcp::ITCPEndpoint::Pointer tcpEndpoint);

      /**
       * @brief Stop TCP transport and close socket.
       *
       * Closes the TCP socket and cancels any in-flight send/receive operations.
       * All pending promises are rejected with TRANSPORT_STOPPED.
       */
      void stop() override;

    private:
      /// Internal: queue a receive operation on TCP socket
      void enqueueReceive(common::DataBuffer buffer) override;

      /// Internal: queue send from send queue to TCP socket
      void enqueueSend(SendQueue::iterator queueElement) override;

      /// Internal: handle TCP send completion; check for errors, resolve promise
      void sendHandler(SendQueue::iterator queueElement, const error::Error &e);

      /// TCP socket endpoint (provides async read/write operations)
      tcp::ITCPEndpoint::Pointer tcpEndpoint_;
    };

}
