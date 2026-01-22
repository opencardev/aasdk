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

#include <aap_protobuf/service/sensorsource/SensorMessageId.pb.h>
#include <aasdk/Channel/SensorSource/ISensorSourceServiceEventHandler.hpp>
#include <aasdk/Channel/SensorSource/SensorSourceService.hpp>
#include "aasdk/Common/Log.hpp"
#include <aasdk/Common/ModernLogger.hpp>


namespace aasdk::channel::sensorsource {

  SensorSourceService::SensorSourceService(boost::asio::io_service::strand &strand, messenger::IMessenger::Pointer messenger)
      : Channel(strand, std::move(messenger), messenger::ChannelId::SENSOR) {

  }

  void SensorSourceService::receive(ISensorSourceServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_SENSOR_SOURCE(debug, "receive()");
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&SensorSourceService::messageHandler, this->shared_from_this(), std::placeholders::_1, eventHandler),
        std::bind(&ISensorSourceServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void SensorSourceService::sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                                              SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_SENSOR_SOURCE(debug, "sendChannelOpenResponse()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void
  SensorSourceService::messageHandler(messenger::Message::Pointer message, ISensorSourceServiceEventHandler::Pointer eventHandler) {

    AASDK_LOG_CHANNEL_SENSOR_SOURCE(debug, "messageHandler()");

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::sensorsource::SensorMessageId::SENSOR_MESSAGE_REQUEST:
        this->handleSensorStartRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
      default:
        AASDK_LOG(error) << "[SensorSourceService] Message Id not Handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void
  SensorSourceService::sendSensorEventIndication(const aap_protobuf::service::sensorsource::message::SensorBatch &indication,
                                           SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_SENSOR_SOURCE(debug, "sendSensorEventIndication()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(
        messenger::MessageId(aap_protobuf::service::sensorsource::SensorMessageId::SENSOR_MESSAGE_BATCH).getData());
    message->insertPayload(indication);

    this->send(std::move(message), std::move(promise));
  }

  void
  SensorSourceService::sendSensorStartResponse(
      const aap_protobuf::service::sensorsource::message::SensorStartResponseMessage &response,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_SENSOR_SOURCE(debug, "sendSensorStartResponse()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(
        messenger::MessageId(aap_protobuf::service::sensorsource::SensorMessageId::SENSOR_MESSAGE_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void SensorSourceService::handleSensorStartRequest(const common::DataConstBuffer &payload,
                                               ISensorSourceServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_SENSOR_SOURCE(debug, "handleSensorStartRequest()");
    aap_protobuf::service::sensorsource::message::SensorRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onSensorStartRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void SensorSourceService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                               ISensorSourceServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_SENSOR_SOURCE(debug, "handleChannelOpenRequest()");
    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

}


