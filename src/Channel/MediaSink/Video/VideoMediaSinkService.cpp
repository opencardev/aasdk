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

#include <aap_protobuf/service/media/sink/MediaMessageId.pb.h>
#include <aasdk/Channel//MediaSink/Video/IVideoMediaSinkServiceEventHandler.hpp>
#include <aasdk/Channel/MediaSink/Video/VideoMediaSinkService.hpp>
#include "aasdk/Common/Log.hpp"
#include <aasdk/Common/ModernLogger.hpp>

/*
 * TODO: Merge Audio and Video Sink Service - P4
 * A lot of AudioMediaSinkService and VideoMediaSinkService, share code in common with each other.
 * It would be recommended to merge these at some point for consistent MediaSink handling. This was attempted
 * previously, but caused messy code due to some of the callbacks and handlers specific to Video
 */
namespace aasdk::channel::mediasink::video {

  VideoMediaSinkService::VideoMediaSinkService(boost::asio::io_service::strand &strand,
                                               messenger::IMessenger::Pointer messenger,
                                               messenger::ChannelId channelId)
      : Channel(strand, std::move(messenger), channelId) {

  }

  void VideoMediaSinkService::receive(IVideoMediaSinkServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "receive()");
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&VideoMediaSinkService::messageHandler, this->shared_from_this(), std::placeholders::_1,
                  eventHandler),
        std::bind(&IVideoMediaSinkServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void VideoMediaSinkService::sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                                                      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "sendChannelOpenResponse()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void VideoMediaSinkService::sendChannelSetupResponse(
      const aap_protobuf::service::media::shared::message::Config &response,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "sendChannelSetupResponse()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_CONFIG).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void VideoMediaSinkService::sendMediaAckIndication(
      const aap_protobuf::service::media::source::message::Ack &indication,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "sendMediaAckIndication()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_ACK).getData());
    message->insertPayload(indication);

    this->send(std::move(message), std::move(promise));
  }

  void VideoMediaSinkService::sendVideoFocusIndication(
      const aap_protobuf::service::media::video::message::VideoFocusNotification &indication,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "sendVideoFocusIndication()");

    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED, messenger::MessageType::SPECIFIC));
    message->insertPayload(messenger::MessageId(aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_VIDEO_FOCUS_NOTIFICATION).getData());
    message->insertPayload(indication);

    this->send(std::move(message), std::move(promise));
  }

  void VideoMediaSinkService::messageHandler(messenger::Message::Pointer message,
                                             IVideoMediaSinkServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "messageHandler()");
    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_SETUP:
        this->handleChannelSetupRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_START:
        this->handleStartIndication(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_STOP:
        this->handleStopIndication(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_CODEC_CONFIG:
        eventHandler->onMediaIndication(payload);
        break;
      case aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_DATA:
        this->handleMediaWithTimestampIndication(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_VIDEO_FOCUS_REQUEST:
        this->handleVideoFocusRequest(payload, std::move(eventHandler));
        break;
      default:
        AASDK_LOG(error) << "[VideoMediaSinkService] Message Id not Handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void VideoMediaSinkService::handleChannelSetupRequest(const common::DataConstBuffer &payload,
                                                        IVideoMediaSinkServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "handleChannelSetupRequest()");
    aap_protobuf::service::media::shared::message::Setup request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onMediaChannelSetupRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void VideoMediaSinkService::handleStartIndication(const common::DataConstBuffer &payload,
                                                    IVideoMediaSinkServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "handleStartIndication()");
    aap_protobuf::service::media::shared::message::Start indication;
    if (indication.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onMediaChannelStartIndication(indication);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void VideoMediaSinkService::handleStopIndication(const common::DataConstBuffer &payload,
                                                   IVideoMediaSinkServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "handleStopIndication()");
    aap_protobuf::service::media::shared::message::Stop indication;
    if (indication.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onMediaChannelStopIndication(indication);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void VideoMediaSinkService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                                       IVideoMediaSinkServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "handleChannelOpenRequest()");
    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void VideoMediaSinkService::handleMediaWithTimestampIndication(const common::DataConstBuffer &payload,
                                                                 IVideoMediaSinkServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "handleMediaWithTimestampIndication()");
    if (payload.size >= sizeof(messenger::Timestamp::ValueType)) {
      messenger::Timestamp timestamp(payload);
      eventHandler->onMediaWithTimestampIndication(timestamp.getValue(),
                                                   common::DataConstBuffer(payload.cdata, payload.size,
                                                                           sizeof(messenger::Timestamp::ValueType)));
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void VideoMediaSinkService::handleVideoFocusRequest(const common::DataConstBuffer &payload,
                                                      IVideoMediaSinkServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_SINK(debug, "handleVideoFocusRequest()");
    aap_protobuf::service::media::video::message::VideoFocusRequestNotification request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onVideoFocusRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }
}

