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

#include "aasdk/Channel/Channel.hpp"
#include "IMediaPlaybackStatusService.hpp"


namespace aasdk::channel::mediaplaybackstatus {


  class MediaPlaybackStatusService
      : public IMediaPlaybackStatusService,
        public Channel,
        public std::enable_shared_from_this<MediaPlaybackStatusService> {
  public:
    MediaPlaybackStatusService(boost::asio::io_service::strand &strand, messenger::IMessenger::Pointer messenger);

    // Senders and Receivers

    void receive(IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler) override;

    void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) override;

  private:
    using std::enable_shared_from_this<MediaPlaybackStatusService>::shared_from_this;

    // Internal Message Handlers

    void
    messageHandler(messenger::Message::Pointer message, IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler);

    void handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                  IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler);

    void handleMetadataUpdate(const common::DataConstBuffer &payload,
                              IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler);

    void handlePlaybackUpdate(const common::DataConstBuffer &payload,
                              IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler);

  };

}

