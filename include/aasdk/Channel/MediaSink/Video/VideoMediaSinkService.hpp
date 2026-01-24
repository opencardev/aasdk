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

#include "aasdk/Messenger/MessageId.hpp"
#include "aasdk/Channel/Channel.hpp"
#include "IVideoMediaSinkService.hpp"
#include <aap_protobuf/service/media/video/message/VideoFocusNotification.pb.h>

namespace aasdk::channel::mediasink::video {

  class VideoMediaSinkService
      : public IVideoMediaSinkService, public Channel, public std::enable_shared_from_this<VideoMediaSinkService> {
  public:
    VideoMediaSinkService(boost::asio::io_service::strand &strand, messenger::IMessenger::Pointer messenger,
                          messenger::ChannelId channelId);

    // Senders and Receivers

    void receive(IVideoMediaSinkServiceEventHandler::Pointer eventHandler) override;

    void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) override;

    void
    sendChannelSetupResponse(const aap_protobuf::service::media::shared::message::Config &response,
                             SendPromise::Pointer promise) override;

    void
    sendMediaAckIndication(
        const aap_protobuf::service::media::source::message::Ack &indication,
        SendPromise::Pointer promise) override;

    void sendVideoFocusIndication(
        const aap_protobuf::service::media::video::message::VideoFocusNotification &indication,
        SendPromise::Pointer promise) override;

  protected:
    void registerMessageHandler(int messageId,
                                std::function<void(const common::DataConstBuffer &,
                                                   IVideoMediaSinkServiceEventHandler::Pointer)> handler);

  private:
    using std::enable_shared_from_this<VideoMediaSinkService>::shared_from_this;

    // Internal Message Handlers
    std::unordered_map<int, std::function<void(const common::DataConstBuffer &,
                                               IVideoMediaSinkServiceEventHandler::Pointer)>> messageHandlers_;

    void messageHandler(messenger::Message::Pointer message, IVideoMediaSinkServiceEventHandler::Pointer eventHandler);

    void handleChannelSetupRequest(const common::DataConstBuffer &payload,
                                   IVideoMediaSinkServiceEventHandler::Pointer eventHandler);

    void
    handleStartIndication(const common::DataConstBuffer &payload,
                          IVideoMediaSinkServiceEventHandler::Pointer eventHandler);

    void
    handleStopIndication(const common::DataConstBuffer &payload,
                         IVideoMediaSinkServiceEventHandler::Pointer eventHandler);

    void handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                  IVideoMediaSinkServiceEventHandler::Pointer eventHandler);

    void handleMediaWithTimestampIndication(const common::DataConstBuffer &payload,
                                            IVideoMediaSinkServiceEventHandler::Pointer eventHandler);

    void handleVideoFocusRequest(const common::DataConstBuffer &payload,
                                 IVideoMediaSinkServiceEventHandler::Pointer eventHandler);

  };

}


