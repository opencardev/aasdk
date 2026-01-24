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
 * @file TCPEndpoint.cpp
 * @brief TCP socket endpoint for wireless Android Auto communication.
 *
 * TCPEndpoint wraps a Boost ASIO TCP socket for bidirectional frame exchange
 * with Android device over WiFi network. Designed for wireless deployment where
 * USB cable is unavailable (e.g., embedded infotainment systems, aftermarket
 * head units without USB passthrough).
 *
 * Operations:
 *   - send(buffer): Async write frame data to socket; resolves when TCP sends
 *   - receive(buffer): Async read frame data from socket; resolves when data ready
 *   - stop(): Close socket cleanly
 *
 * Scenario: Wireless Android Auto over WiFi connection
 *   - T+0ms: Head unit WiFi connected to device hotspot (192.168.1.2:5037)
 *   - T+50ms: TCPEndpoint created with connected socket
 *   - T+55ms: Messenger calls receive(4) for frame header
 *   - T+55ms: asyncRead queued on socket; waiting for data
 *   - T+100ms: Android App connects from phone; sends protocol greeting
 *   - T+100ms: 4 bytes arrive over TCP; receive promise resolves
 *   - T+105ms: Messenger parses size=256; calls receive(256) for payload
 *   - T+200ms: 256 bytes payload arrives; resolve completes
 *   - T+205ms: Messenger routes message to appropriate channel service
 *
 * Network characteristics (typical WiFi):
 *   - Latency: 50-200ms depending on WiFi quality and congestion
 *   - Jitter: +/- 100ms variance in frame delivery time
 *   - Potential: Packet loss, TCP retransmission delays
 *   - Advantage: No cable needed; range up to 100ft indoors
 *
 * Error handling:
 *   - operation_aborted: Endpoint was closed during async operation
 *   - TCP_TRANSFER error: Network failure (connection lost, timeout, etc.)
 *
 * Thread Safety: All operations dispatched by TCPTransport on single strand.
 * Socket access is not thread-safe; endpoint assumes single-threaded usage.
 */

#include <aasdk/TCP/TCPEndpoint.hpp>


namespace aasdk::tcp {

  TCPEndpoint::TCPEndpoint(ITCPWrapper &tcpWrapper, SocketPointer socket)
      : tcpWrapper_(tcpWrapper), socket_(std::move(socket)) {

  }

  void TCPEndpoint::send(common::DataConstBuffer buffer, Promise::Pointer promise) {
    tcpWrapper_.asyncWrite(*socket_, std::move(buffer),
                           std::bind(&TCPEndpoint::asyncOperationHandler,
                                     this->shared_from_this(),
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     std::move(promise)));
  }

  void TCPEndpoint::receive(common::DataBuffer buffer, Promise::Pointer promise) {
    tcpWrapper_.asyncRead(*socket_, std::move(buffer),
                          std::bind(&TCPEndpoint::asyncOperationHandler,
                                    this->shared_from_this(),
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::move(promise)));
  }

  void TCPEndpoint::stop() {
    tcpWrapper_.close(*socket_);
  }

  void TCPEndpoint::asyncOperationHandler(const boost::system::error_code &ec, size_t bytesTransferred,
                                          Promise::Pointer promise) {
    if (!ec) {
      promise->resolve(bytesTransferred);
    } else {
      auto error = ec == boost::asio::error::operation_aborted ? error::Error(error::ErrorCode::OPERATION_ABORTED)
                                                               : error::Error(error::ErrorCode::TCP_TRANSFER,
                                                                              static_cast<uint32_t>(ec.value()));
      promise->reject(error);
    }
  }

}

