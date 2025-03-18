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

#include <aap_protobuf/service/radio/RadioMessageId.pb.h>
#include <aasdk/Channel/Radio/IRadioServiceEventHandler.hpp>
#include <aasdk/Channel/Radio/RadioService.hpp>
#include "aasdk/Common/Log.hpp"

/*
 * This is a Radio channel that could be used for integration onto another Raspberry Pi/Other Device to integrate with third party systems or head units to help control the radio if necessary.
 */

namespace aasdk::channel::radio {

  RadioService::RadioService(boost::asio::io_service::strand &strand,
                             messenger::IMessenger::Pointer messenger)
      : Channel(strand, std::move(messenger), messenger::ChannelId::RADIO) {

  }

  void RadioService::receive(IRadioServiceEventHandler::Pointer eventHandler) {

    AASDK_LOG(debug) << "[RadioService] receive()";
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&RadioService::messageHandler, this->shared_from_this(), std::placeholders::_1,
                  eventHandler),
        std::bind(&IRadioServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void RadioService::sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                                             SendPromise::Pointer promise) {
    AASDK_LOG(debug) << "[RadioService] sendChannelOpenResponse()";
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void RadioService::messageHandler(messenger::Message::Pointer message,
                                    IRadioServiceEventHandler::Pointer eventHandler) {

    AASDK_LOG(debug) << "[RadioService] messageHandler()";

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_ACTIVE_RADIO_NOTIFICATION:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_SELECT_ACTIVE_RADIO_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_STEP_CHANNEL_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_STEP_CHANNEL_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_SEEK_STATION_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_SEEK_STATION_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_SCAN_STATIONS_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_SCAN_STATIONS_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_TUNE_TO_STATION_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_TUNE_TO_STATION_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_GET_PROGRAM_LIST_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_GET_PROGRAM_LIST_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_STATION_PRESETS_NOTIFICATION:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_CANCEL_OPERATIONS_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_CANCEL_OPERATIONS_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_CONFIGURE_CHANNEL_SPACING_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_CONFIGURE_CHANNEL_SPACING_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_RADIO_STATION_INFO_NOTIFICATION:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_MUTE_RADIO_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_MUTE_RADIO_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_GET_TRAFFIC_UPDATE_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_GET_TRAFFIC_UPDATE_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_RADIO_SOURCE_REQUEST:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_RADIO_SOURCE_RESPONSE:
      case aap_protobuf::service::radio::RadioMessageId::RADIO_MESSAGE_STATE_NOTIFICATION:
      default:
        AASDK_LOG(error) << "[RadioService] Message Id not Handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void RadioService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                              IRadioServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG(debug) << "[RadioService] handleChannelOpenRequest()";
    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }
}


