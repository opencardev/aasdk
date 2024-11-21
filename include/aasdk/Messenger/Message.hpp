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

#include <boost/noncopyable.hpp>
#include <memory>
#include <google/protobuf/message.h>
#include <aasdk/Common/Data.hpp>
#include <aasdk/Messenger/ChannelId.hpp>
#include <aasdk/Messenger/EncryptionType.hpp>
#include <aasdk/Messenger/MessageType.hpp>
#include <aasdk/Messenger/MessageId.hpp>

namespace aasdk::messenger {

  class Message : boost::noncopyable {
  public:
    typedef std::shared_ptr<Message> Pointer;

    Message(ChannelId channelId, EncryptionType encryptionType, MessageType type);

    Message(Message &&other);

    Message &operator=(Message &&other);

    ChannelId getChannelId() const;

    EncryptionType getEncryptionType() const;

    MessageType getType() const;

    common::Data &getPayload();

    const common::Data &getPayload() const;

    void insertPayload(const common::Data &payload);

    void insertPayload(const google::protobuf::Message &message);

    void insertPayload(const common::DataConstBuffer &buffer);

    void insertPayload(common::DataBuffer &buffer);

  private:
    ChannelId channelId_;
    EncryptionType encryptionType_;
    MessageType type_;
    common::Data payload_;
  };

}

