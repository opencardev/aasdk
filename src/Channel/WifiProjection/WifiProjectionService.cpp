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

#include <aap_protobuf/service/wifiprojection/WifiProjectionMessageId.pb.h>
#include <aasdk/Channel/WifiProjection/IWifiProjectionServiceEventHandler.hpp>
#include <aasdk/Channel/WifiProjection/WifiProjectionService.hpp>
#include "aasdk/Common/Log.hpp"
#include <aasdk/Common/ModernLogger.hpp>


namespace aasdk::channel::wifiprojection {

  WifiProjectionService::WifiProjectionService(boost::asio::io_service::strand &strand,
                                               messenger::IMessenger::Pointer messenger)
      : Channel(strand, std::move(messenger), messenger::ChannelId::WIFI_PROJECTION) {

  }

  void WifiProjectionService::receive(IWifiProjectionServiceEventHandler::Pointer eventHandler) {

    AASDK_LOG_CHANNEL_WIFI_PROJECTION(debug, "receive()");
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&WifiProjectionService::messageHandler, this->shared_from_this(), std::placeholders::_1,
                  eventHandler),
        std::bind(&IWifiProjectionServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void WifiProjectionService::sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                                                      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_WIFI_PROJECTION(debug, "sendChannelOpenResponse()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void WifiProjectionService::sendWifiCredentialsResponse(
      const aap_protobuf::service::wifiprojection::message::WifiCredentialsResponse &response,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_WIFI_PROJECTION(debug, "sendWifiCredentialsResponse()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(messenger::MessageId(
        aap_protobuf::service::wifiprojection::WifiProjectionMessageId::WIFI_MESSAGE_CREDENTIALS_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void WifiProjectionService::messageHandler(messenger::Message::Pointer message,
                                             IWifiProjectionServiceEventHandler::Pointer eventHandler) {

    AASDK_LOG_CHANNEL_WIFI_PROJECTION(debug, "messageHandler()");

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::wifiprojection::WifiProjectionMessageId::WIFI_MESSAGE_CREDENTIALS_REQUEST:
        this->handleWifiCredentialsRequest(payload, std::move(eventHandler));
        break;
      default:
        AASDK_LOG(error) << "[WifiProjectionService] Message Id not Handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void WifiProjectionService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                                       IWifiProjectionServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_WIFI_PROJECTION(debug, "handleChannelOpenRequest()");
    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void WifiProjectionService::handleWifiCredentialsRequest(const common::DataConstBuffer &payload,
                                                           IWifiProjectionServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_WIFI_PROJECTION(debug, "handleWifiCredentialsRequest()");

    aap_protobuf::service::wifiprojection::message::WifiCredentialsRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onWifiCredentialsRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }
}


