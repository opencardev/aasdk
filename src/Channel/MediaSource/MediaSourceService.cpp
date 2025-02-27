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

#include <aap_protobuf/service/media/sink/MediaMessageId.pb.h>
#include "aasdk/Messenger/Timestamp.hpp"
#include "aasdk/Channel/MediaSource/IMediaSourceServiceEventHandler.hpp"
#include "aasdk/Channel/MediaSource/MediaSourceService.hpp"
#include "aasdk/Common/Log.hpp"


namespace aasdk::channel::mediasource {

  MediaSourceService::MediaSourceService(boost::asio::io_service::strand &strand,
                                         messenger::IMessenger::Pointer messenger,
                                         messenger::ChannelId channelId)
      : Channel(strand, std::move(messenger), channelId) {

  }

  void MediaSourceService::receive(IMediaSourceServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaSourceService] receive()";
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&MediaSourceService::messageHandler, this->shared_from_this(), std::placeholders::_1, eventHandler),
        std::bind(&IMediaSourceServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void MediaSourceService::sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                                                   SendPromise::Pointer promise) {
    AASDK_LOG(debug) << "[MediaSourceService] sendChannelOpenResponse()";
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }


  void MediaSourceService::sendChannelSetupResponse(
      const aap_protobuf::service::media::shared::message::Config &response,
      SendPromise::Pointer promise) {
    AASDK_LOG(debug) << "[MediaSourceService] sendChannelSetupResponse()";
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_SETUP).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void MediaSourceService::messageHandler(messenger::Message::Pointer message,
                                          IMediaSourceServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaSourceService] messageHandler()";
    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_SETUP:
        this->handleAVChannelSetupRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_MICROPHONE_REQUEST:
        this->handleAVInputOpenRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_ACK:
        this->handleAVMediaAckIndication(payload, std::move(eventHandler));
        break;
      default:
        AASDK_LOG(error) << "[MediaSourceService] Message Id not Handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void MediaSourceService::sendMicrophoneOpenResponse(
      const aap_protobuf::service::media::source::message::MicrophoneResponse &response, SendPromise::Pointer promise) {
    AASDK_LOG(debug) << "[MediaSourceService] sendMicrophoneOpenResponse()";
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));

    message->insertPayload(messenger::MessageId(
        aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_MICROPHONE_REQUEST).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void MediaSourceService::sendMediaSourceWithTimestampIndication(messenger::Timestamp::ValueType timestamp,
                                                                  const common::Data &data,
                                                                  SendPromise::Pointer promise) {
    AASDK_LOG(debug) << "[MediaSourceService] sendMediaSourceWithTimestampIndication()";
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(messenger::MessageId(
        aap_protobuf::service::media::sink::MediaMessageId::MEDIA_MESSAGE_DATA).getData());

    auto timestampData = messenger::Timestamp(timestamp).getData();
    message->insertPayload(std::move(timestampData));
    message->insertPayload(data);

    this->send(std::move(message), std::move(promise));
  }

  void MediaSourceService::handleAVChannelSetupRequest(const common::DataConstBuffer &payload,
                                                       IMediaSourceServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaSourceService] handleAVChannelSetupRequest()";
    aap_protobuf::service::media::shared::message::Setup request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onMediaChannelSetupRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void MediaSourceService::handleAVInputOpenRequest(const common::DataConstBuffer &payload,
                                                    IMediaSourceServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaSourceService] handleAVInputOpenRequest()";
    aap_protobuf::service::media::source::message::MicrophoneRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onMediaSourceOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void MediaSourceService::handleAVMediaAckIndication(const common::DataConstBuffer &payload,
                                                      IMediaSourceServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaSourceService] handleAVMediaAckIndication()";
    aap_protobuf::service::media::source::message::Ack indication;
    if (indication.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onMediaChannelAckIndication(indication);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void MediaSourceService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                                    IMediaSourceServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[MediaSourceService] handleChannelOpenRequest()";
    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

}


