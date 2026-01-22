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
 * @file TCPWrapper.cpp
 * @brief TCP socket operations wrapper for Boost ASIO compatibility layer.
 *
 * TCPWrapper provides a clean abstraction over Boost ASIO TCP operations,
 * enabling mock/stub implementations for testing and supporting both
 * synchronous (blocking) and asynchronous connection/communication.
 *
 * Operations:
 *   - asyncWrite: Non-blocking send; completes when data written to kernel buffer
 *   - asyncRead: Non-blocking receive; completes when data available
 *   - connect: Synchronous (blocking) TCP connection to remote host
 *   - asyncConnect: Non-blocking connection; typically used during init
 *   - close: Graceful socket shutdown (both directions) and cleanup
 *
 * Scenario: Wireless Android Auto connection over WiFi
 *   - T+0ms: App calls connect("192.168.1.45", 5037) - blocking call
 *   - T+5ms: TCP three-way handshake (SYN, SYN-ACK, ACK) completes
 *   - T+10ms: connect() returns; socket ready for communication
 *   - T+15ms: TCPTransport created; calls asyncRead(4) for frame header
 *   - T+20ms: asyncRead queued; waiting for data from Android device
 *   - T+50ms: User launches Android Auto app on phone
 *   - T+100ms: App connects; sends greeting frame (4 bytes header)
 *   - T+100ms: asyncRead completes; calls handler with header data
 *   - T+105ms: TCPTransport parses header; requests payload with asyncRead(256)
 *   - T+200ms: Payload arrives; asyncRead completes
 *   - T+300ms: Messaging established; services begin negotiation
 *
 * Optimisations:
 *   - TCP_NODELAY enabled: Disables Nagle algorithm for low-latency messaging
 *     (important for interactive navigation turns and media control)
 *   - async_write: Guarantees all N bytes written (loops internally if needed)
 *   - async_receive: Returns immediately when any data available (max buffer size)
 *
 * Error handling:
 *   - connection_refused: Remote device not listening
 *   - timed_out: Network unreachable or device offline
 *   - shutdown errors ignored (best-effort close)
 *
 * Thread Safety: Wrapper delegates to ASIO; caller responsible for strand usage.
 */

#include <utility>
#include <boost/asio.hpp>
#include <aasdk/TCP/TCPWrapper.hpp>


namespace aasdk {
  namespace tcp {

    void TCPWrapper::asyncWrite(boost::asio::ip::tcp::socket &socket, common::DataConstBuffer buffer, Handler handler) {
      boost::asio::async_write(socket, boost::asio::buffer(buffer.cdata, buffer.size), std::move(handler));
    }

    void TCPWrapper::asyncRead(boost::asio::ip::tcp::socket &socket, common::DataBuffer buffer, Handler handler) {
      socket.async_receive(boost::asio::buffer(buffer.data, buffer.size), std::move(handler));
    }

    void TCPWrapper::close(boost::asio::ip::tcp::socket &socket) {
      boost::system::error_code ec;
      socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
      socket.close(ec);
    }

    void TCPWrapper::asyncConnect(boost::asio::ip::tcp::socket &socket, const std::string &hostname, uint16_t port,
                                  ConnectHandler handler) {
      socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(hostname), port),
                           std::move(handler));
    }

    boost::system::error_code
    TCPWrapper::connect(boost::asio::ip::tcp::socket &socket, const std::string &hostname, uint16_t port) {
      boost::system::error_code ec;
      socket.set_option(boost::asio::ip::tcp::no_delay(true), ec);
      socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(hostname), port), ec);
      return ec;
    }

  }
}
