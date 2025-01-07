// This file is part of aasdk library project.
// Copyright (C) 2018 f1x.studio (Michal Szwaj)
// Copyright (C) 2024 CubeOne (Simon Dean - simon.dean@cubeone.co.uk)
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
#include <aasdk/Messenger/ChannelId.hpp>
#include <aasdk/Messenger/FrameType.hpp>
#include <aasdk/Messenger/EncryptionType.hpp>
#include <aasdk/Messenger/MessageType.hpp>


namespace aasdk {
  namespace messenger {

    class FrameHeader {
    public:
      FrameHeader(const common::DataConstBuffer &buffer);

      FrameHeader(ChannelId channelId, FrameType frameType, EncryptionType encryptionType, MessageType messageType);

      ChannelId getChannelId() const;

      FrameType getType() const;

      EncryptionType getEncryptionType() const;

      MessageType getMessageType() const;

      common::Data getData() const;

      static constexpr size_t getSizeOf() { return 2; }

    private:
      ChannelId channelId_;
      FrameType frameType_;
      EncryptionType encryptionType_;
      MessageType messageType_;
    };

  }
}
