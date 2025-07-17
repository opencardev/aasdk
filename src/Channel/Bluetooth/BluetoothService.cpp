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

#include <aap_protobuf/service/bluetooth/BluetoothMessageId.pb.h>
#include "aasdk/Channel/Bluetooth/IBluetoothServiceEventHandler.hpp"
#include "aasdk/Channel/Bluetooth/BluetoothService.hpp"
#include "aasdk/Common/Log.hpp"
#include <aasdk/Common/ModernLogger.hpp>

namespace aasdk::channel::bluetooth {

  BluetoothService::BluetoothService(boost::asio::io_service::strand &strand,
                                     messenger::IMessenger::Pointer messenger)
      : Channel(strand, std::move(messenger), messenger::ChannelId::BLUETOOTH) {

  }

  void BluetoothService::receive(IBluetoothServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_BLUETOOTH(debug, "receive()");

    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&BluetoothService::messageHandler, this->shared_from_this(), std::placeholders::_1,
                  eventHandler),
        std::bind(&IBluetoothServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void BluetoothService::sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                                                 SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_BLUETOOTH(debug, "sendChannelOpenResponse()");

    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void BluetoothService::sendBluetoothPairingResponse(
      const aap_protobuf::service::bluetooth::message::BluetoothPairingResponse &response,
      SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_BLUETOOTH(debug, "sendBluetoothPairingResponse()");

    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::bluetooth::BluetoothMessageId::BLUETOOTH_MESSAGE_PAIRING_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void BluetoothService::sendBluetoothAuthenticationData(
      const aap_protobuf::service::bluetooth::message::BluetoothAuthenticationData &response,
      SendPromise::Pointer promise) {

    AASDK_LOG_CHANNEL_BLUETOOTH(debug, "sendBluetoothAuthenticationData()");

    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::SPECIFIC));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::bluetooth::BluetoothMessageId::BLUETOOTH_MESSAGE_AUTHENTICATION_DATA).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void BluetoothService::messageHandler(messenger::Message::Pointer message,
                                        IBluetoothServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_BLUETOOTH(debug, "messageHandler()");

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::bluetooth::BluetoothMessageId::BLUETOOTH_MESSAGE_PAIRING_REQUEST:
        this->handleBluetoothPairingRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::bluetooth::BluetoothMessageId::BLUETOOTH_MESSAGE_AUTHENTICATION_RESULT:
        this->handleBluetoothAuthenticationResult(payload, std::move(eventHandler));
        break;
      default:
        AASDK_LOG(error) << "[BluetoothService] Message Id not Handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void BluetoothService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                                  IBluetoothServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_BLUETOOTH(debug, "handleChannelOpenRequest()");

    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void BluetoothService::handleBluetoothAuthenticationResult(const common::DataConstBuffer &payload,
                                                                  IBluetoothServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_BLUETOOTH(debug, "handleBluetoothAuthenticationResult()");

    aap_protobuf::service::bluetooth::message::BluetoothAuthenticationResult request;

    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onBluetoothAuthenticationResult(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void BluetoothService::handleBluetoothPairingRequest(const common::DataConstBuffer &payload,
                                                       IBluetoothServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_BLUETOOTH(debug, "handleBluetoothPairingRequest()");

    aap_protobuf::service::bluetooth::message::BluetoothPairingRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onBluetoothPairingRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

}


