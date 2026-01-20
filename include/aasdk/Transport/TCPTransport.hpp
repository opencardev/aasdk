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
     * @brief TCP/IP transport layer for wireless or remote Android Auto communication.
     *
     * **Purpose:**
     * Implements network-based communication with Android devices using TCP sockets, typically
     * over Wi-Fi. This enables:
     * - Wireless headunits (no physical USB cable)
     * - Remote connection scenarios (phone in parking lot, headunit in garage)
     * - Development/testing without USB hardware
     * - Fallback for devices without AOAP support
     *
     * **Network Topology:**
     * ```
     *   Android Device (TCP Client)
     *        │
     *        ├─ Wi-Fi Network (192.168.1.100:5037)
     *        │
     *   Headunit (TCP Server, listening on 0.0.0.0:5037)
     *        │
     *   TCPTransport
     *        │
     *   [Socket]
     *   ├─ Receive (read) stream
     *   └─ Send (write) stream
     *
     *   Optional: [SSL/TLS Layer] (ISSLWrapper)
     *   ├─ Certificate validation
     *   └─ Encryption (AES, etc.)
     * ```
     *
     * **Key Characteristics:**
     * - **Network dependency**: Works over Wi-Fi, 4G/LTE, or Ethernet
     * - **Higher latency**: Typical RTT 50–500ms (vs. 5–20ms for USB)
     * - **SSL/TLS capable**: Optional encryption; see ISSLWrapper
     * - **Timeouts needed**: Network can be unreliable; protocols must implement heartbeats
     * - **Concurrent sessions**: Multiple phones can connect (headunit acts as server)
     * - **Scalability**: Suitable for cloud/remote scenarios (headunit in data centre)
     *
     * **TCP Connection Flow:**
     * ```
     * 1. Headunit TCP server listening on 0.0.0.0:5037
     * 2. Phone connects (initiates three-way handshake)
     * 3. TCP connection established
     * 4. Optional: TLS handshake for encryption
     * 5. Phone sends Android Auto handshake message
     * 6. TCPTransport receive() promise resolves
     * 7. Headunit sends auth response
     * 8. TCPTransport send() promise resolves
     * 9. Bi-directional communication continues
     * 10. Either side closes → connection ends
     * ```
     *
     * **Wireless Reliability Considerations:**
     * ```
     * Risk                          Mitigation
     * Wi-Fi signal drops            TCP reconnection logic; heartbeat messages
     * Network congestion            Adaptive backpressure; send queue limits
     * Firewall blocking port 5037   Fallback to USB; UPnP port mapping
     * Latency jitter                Buffering; higher timeouts than USB
     * Packet loss                   TCP layer retransmits; application timeout
     * Out-of-order delivery         TCP guarantees order; no app concern
     * ```
     *
     * **Scenario: Phone Loses Wi-Fi Mid-Transfer**
     * ```
     * Time    Event                                TCPTransport State
     * T+0ms   Phone connected via Wi-Fi           Sending media metadata
     * T+100ms send(mediaMsg, promise)              In flight to headunit
     * T+500ms [Wi-Fi signal drops]                 TCP retransmitting
     * T+1500ms TCP retransmit timeout             Connection lost
     * T+1600ms promise->reject(CONNECTION_LOST)   Error reported
     * T+2000ms [User plugs in USB cable]          Fallback to USB transport
     * T+2500ms USBTransport takes over            Resume messaging
     * ```
     *
     * **Timeout Strategy:**
     * - TCP connection: 30 seconds (initial handshake)
     * - Send operation: 60 seconds per frame
     * - Receive operation: Infinite (wait for phone data or close)
     * - Keep-alive (heartbeat): Application-level, e.g., 30 seconds
     *
     * **Thread Safety:**
     * - receive()/send()/stop() can be called from any thread
     * - Socket operations run on boost::asio strands
     * - Promise callbacks invoked on the strand
     *
     * **Comparison: USB vs. TCP/IP**
     * ```
     * Feature             USB            TCP/IP
     * Latency            5–20ms         50–500ms
     * Reliability        Very high      Medium (network-dependent)
     * Setup complexity   Complex        Simple
     * Encryption         Not needed     Optional (SSL/TLS)
     * Throughput         1–10 MB/s      10–100 MB/s
     * Concurrent sessions 1 (per device) N (server-based)
     * Power (device)     Charges        Wi-Fi power draw
     * Range              Direct cable   ~30m (Wi-Fi), unlimited (cloud)
     * Failure mode       Unplugged      Network drops
     * ```
     *
     * **Usage Example (Wireless Headunit):**
     * ```cpp
     * // Create TCP endpoint (handles socket & optional SSL)
     * auto tcpEndpoint = std::make_shared<TCPEndpoint>(
     *     io_service,
     *     "192.168.1.100",  // Phone's Wi-Fi IP
     *     5037,             // Android Auto standard port
     *     ssl_wrapper       // optional
     * );
     *
     * // Create transport
     * auto transport = std::make_shared<TCPTransport>(io_service, tcpEndpoint);
     *
     * // Receive Android Auto frames
     * auto receivePromise = std::make_shared<ReceivePromise>();
     * receivePromise->onResolve([](const Data& frame) {
     *     LOG(INFO) << "Received " << frame.size() << " bytes via TCP";
     *     // Process frame...
     * });
     * receivePromise->onReject([](const Error& err) {
     *     LOG(ERROR) << "TCP receive failed: " << err.message();
     *     // Fallback to USB or notify user
     * });
     * transport->receive(8192, receivePromise);
     * ```
     *
     * **Deployment Scenarios:**
     * ```
     * Scenario 1: Wireless Headunit in Car
     *   - Headunit and phone on same Wi-Fi network
     *   - Range: ~30m indoors, ~50m outdoors
     *   - Typical latency: 100–200ms
     *   - Reliability: Good if network is stable
     *
     * Scenario 2: Aftermarket Headunit with Cloud Sync
     *   - Headunit in car; phone at home
     *   - Connection via 4G/LTE or home Wi-Fi
     *   - Typical latency: 500ms–2s (network-dependent)
     *   - Reliability: Needs robust reconnection logic
     *
     * Scenario 3: Development/Testing
     *   - Emulator or VM as "headunit"
     *   - Phone on same LAN or VPN
     *   - Typical latency: 10–50ms
     *   - Reliability: Excellent for testing
     * ```
     *
     * @see ITransport for the interface contract
     * @see Transport for base class
     * @see ITCPEndpoint for socket abstraction
     * @see ISSLWrapper for optional TLS encryption
     */
    class TCPTransport : public Transport {
    public:
      /**
       * @brief Construct a TCPTransport connected to an endpoint.
       *
       * **Parameters:**
       * - ioService: Boost ASIO io_service for async socket operations
       * - tcpEndpoint: ITCPEndpoint with an open and connected socket
       *
       * **Preconditions:**
       * - tcpEndpoint must have an established TCP connection
       * - Optionally, TLS handshake has completed (if using SSL/TLS)
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
       * auto transport = std::make_shared<TCPTransport>(io_service, tcpEndpoint);
       * ```
       */
      TCPTransport(boost::asio::io_service &ioService, tcp::ITCPEndpoint::Pointer tcpEndpoint);

      /**
       * @brief Stop TCP communication and close the socket.
       *
       * **Behaviour:**
       * - Cancels all pending read/write operations
       * - Closes the TCP socket gracefully (FIN handshake if possible)
       * - Rejects all outstanding receive/send promises
       * - Safe to call multiple times
       *
       * **Side Effects:**
       * - Phone sees connection closed (RST or FIN)
       * - Undelivered messages are discarded
       * - After stop(), the transport is unusable; must create new TCPTransport for reconnection
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
       * @brief Internal: Queue an asynchronous socket read.
       *
       * Posts a read operation on the TCP socket to receive up to @p buffer.size() bytes.
       * When data arrives, the promise is resolved with the received data.
       * If an error occurs (connection closed, timeout, etc.), the promise is rejected.
       *
       * @param [in] buffer Output buffer for received data
       */
      void enqueueReceive(common::DataBuffer buffer) override;

      /**
       * @brief Internal: Initiate transmission of a queued message.
       *
       * Pops a (Data, Promise) pair from the send queue and starts a socket write operation.
       * Unlike USB (which may fragment), TCP typically sends the entire frame in one write,
       * though multiple writes may be needed for very large messages.
       *
       * @param [in] queueElement Iterator to send queue element being transmitted
       */
      void enqueueSend(SendQueue::iterator queueElement) override;

      /**
       * @brief Callback: Handle completion of a socket write operation.
       *
       * Called by boost::asio when a socket write completes or fails. Resolves or rejects
       * the associated send promise.
       *
       * @param [in] queueElement Iterator to send queue element
       * @param [in] e boost::system::error_code (OK if successful, error otherwise)
       */
      void sendHandler(SendQueue::iterator queueElement, const error::Error &e);

      /// TCP endpoint with connected socket
      tcp::ITCPEndpoint::Pointer tcpEndpoint_;
    };

}
