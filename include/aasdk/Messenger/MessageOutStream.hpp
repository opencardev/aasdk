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

#include <aasdk/Common/Data.hpp>
#include <aasdk/Transport/ITransport.hpp>
#include <aasdk/Messenger/ICryptor.hpp>
#include <aasdk/Messenger/IMessageOutStream.hpp>
#include <aasdk/Messenger/FrameHeader.hpp>
#include <aasdk/Messenger/FrameSize.hpp>


namespace aasdk::messenger {

    class MessageOutStream
        : public IMessageOutStream, public std::enable_shared_from_this<MessageOutStream> {
    public:
      MessageOutStream(boost::asio::io_service &ioService, transport::ITransport::Pointer transport,
                       ICryptor::Pointer cryptor);

      // Deleted copy operations
      MessageOutStream(const MessageOutStream &) = delete;
      MessageOutStream &operator=(const MessageOutStream &) = delete;

      void stream(Message::Pointer message, SendPromise::Pointer promise) override;

    private:
      using std::enable_shared_from_this<MessageOutStream>::shared_from_this;

      void streamSplittedMessage();

      common::Data compoundFrame(FrameType frameType, const common::DataConstBuffer &payloadBuffer);

      void streamEncryptedFrame(FrameType frameType, const common::DataConstBuffer &payloadBuffer);

      void streamPlainFrame(FrameType frameType, const common::DataConstBuffer &payloadBuffer);

      void setFrameSize(common::Data &data, FrameType frameType, size_t payloadSize, size_t totalSize);

      void reset();

      boost::asio::io_service::strand strand_;
      transport::ITransport::Pointer transport_;
      ICryptor::Pointer cryptor_;
      Message::Pointer message_;
      size_t offset_;
      size_t remainingSize_;
      SendPromise::Pointer promise_;

      static constexpr size_t cMaxFramePayloadSize = 0x4000;
    };

}
