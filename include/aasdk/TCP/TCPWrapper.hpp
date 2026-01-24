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

#include <aasdk/TCP/ITCPWrapper.hpp>


namespace aasdk::tcp {

    class TCPWrapper : public ITCPWrapper {
    public:
      void asyncWrite(boost::asio::ip::tcp::socket &socket, common::DataConstBuffer buffer, Handler handler) override;

      void asyncRead(boost::asio::ip::tcp::socket &socket, common::DataBuffer buffer, Handler handler) override;

      void close(boost::asio::ip::tcp::socket &socket) override;

      void asyncConnect(boost::asio::ip::tcp::socket &socket, const std::string &hostname, uint16_t port,
                        ConnectHandler handler) override;

      boost::system::error_code
      connect(boost::asio::ip::tcp::socket &socket, const std::string &hostname, uint16_t port) override;
    };

}
