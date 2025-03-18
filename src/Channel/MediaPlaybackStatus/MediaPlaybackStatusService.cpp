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

#include <aap_protobuf/service/mediaplayback/MediaPlaybackStatusMessageId.pb.h>
#include "aasdk/Channel/MediaPlaybackStatus/MediaPlaybackStatusService.hpp"
#include "aasdk/Channel/MediaPlaybackStatus/IMediaPlaybackStatusServiceEventHandler.hpp"
#include "aasdk/Common/Log.hpp"

/*
 * This is a Media Playback Status channel that could be used for integration onto another Raspberry Pi/Other Device to add an additional screen for notification and control purposes - such as updating the LCD screen on older Vauxhall/Opel/GM Cars
 */

namespace aasdk::channel::mediaplaybackstatus {

  MediaPlaybackStatusService::MediaPlaybackStatusService(boost::asio::io_service::strand &strand,
                                                         messenger::IMessenger::Pointer messenger)
      : Channel(strand, std::move(messenger), messenger::ChannelId::MEDIA_PLAYBACK_STATUS) {

  }

  void MediaPlaybackStatusService::receive(IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaPlaybackStatusService] receive()";
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&MediaPlaybackStatusService::messageHandler, this->shared_from_this(), std::placeholders::_1,
                  eventHandler),
        std::bind(&IMediaPlaybackStatusServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void MediaPlaybackStatusService::sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                                                           SendPromise::Pointer promise) {
    AASDK_LOG(debug) << "[MediaPlaybackStatusService] sendChannelOpenResponse()";
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }


  void MediaPlaybackStatusService::messageHandler(messenger::Message::Pointer message,
                                                  IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaPlaybackStatusService] messageHandler()";

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;

      case aap_protobuf::service::mediaplayback::MediaPlaybackStatusMessageId::MEDIA_PLAYBACK_METADATA:
        this->handleMetadataUpdate(payload, std::move(eventHandler));
        break;

      case aap_protobuf::service::mediaplayback::MediaPlaybackStatusMessageId::MEDIA_PLAYBACK_STATUS:
        this->handlePlaybackUpdate(payload, std::move(eventHandler));
        break;

      default:
        AASDK_LOG(error) << "[MediaPlaybackStatusService] Message Id not Handled: " << messageId.getId() << " : "
                         << dump(payload);
        this->receive(std::move(eventHandler));
        break;
    }


  }

  void MediaPlaybackStatusService::handleMetadataUpdate(const common::DataConstBuffer &payload,
                                                        IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaPlaybackStatusService] handleMetadataUpdate()";
    aap_protobuf::service::mediaplayback::message::MediaPlaybackMetadata metadata;
    if (metadata.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onMetadataUpdate(metadata);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
      AASDK_LOG(error) << "[MediaPlaybackStatusService] encountered error with message: " << dump(payload);
    }

  }


  void MediaPlaybackStatusService::handlePlaybackUpdate(const common::DataConstBuffer &payload,
                                                        IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaPlaybackStatusService] handlePlaybackUpdate()";
    aap_protobuf::service::mediaplayback::message::MediaPlaybackStatus playback;
    if (playback.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onPlaybackUpdate(playback);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
      AASDK_LOG(error) << "[MediaPlaybackStatusService] encountered error with message: " << dump(payload);
    }

  }

  void MediaPlaybackStatusService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                                            IMediaPlaybackStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaPlaybackStatusService] handleChannelOpenRequest()";

    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }


}


