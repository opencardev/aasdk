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

#include "aasdk/IO/PromiseLink.hpp"
#include "aasdk/Channel/Channel.hpp"

namespace aasdk::channel {
  Channel::Channel(boost::asio::io_service::strand &strand,
                   messenger::IMessenger::Pointer messenger,
                   messenger::ChannelId channelId)
      : strand_(strand), messenger_(std::move(messenger)), channelId_(channelId) {

  }

  messenger::ChannelId Channel::getId() const {
    return channelId_;
  }

  void Channel::send(messenger::Message::Pointer message, SendPromise::Pointer promise) {
#if BOOST_VERSION < 106600
    auto sendPromise = messenger::SendPromise::defer(strand_.get_io_service());
#else
    auto sendPromise = messenger::SendPromise::defer(strand_.context());
#endif

    io::PromiseLink<>::forward(*sendPromise, std::move(promise));
    messenger_->enqueueSend(std::move(message), std::move(sendPromise));
  }

}

