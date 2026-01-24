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
#include "aasdk/Messenger/IMessenger.hpp"
#include "aasdk/Channel/Promise.hpp"
#include "aasdk/Channel/IChannel.hpp"
#include <aap_protobuf/service/control/ControlMessageType.pb.h>

namespace aasdk::channel {
class Channel : public virtual IChannel {
public:
  Channel(boost::asio::io_service::strand &strand,
          messenger::IMessenger::Pointer messenger,
          messenger::ChannelId channelId);

  virtual ~Channel() = default;

  messenger::ChannelId getId() const override;

  void send(messenger::Message::Pointer message, SendPromise::Pointer promise) override;

protected:
  boost::asio::io_service::strand &strand_;
  messenger::IMessenger::Pointer messenger_;
  messenger::ChannelId channelId_;
};
}
