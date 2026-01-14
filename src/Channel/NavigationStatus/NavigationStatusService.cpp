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

#include <aap_protobuf/service/navigationstatus//NavigationStatusMessageId.pb.h>
#include "aasdk/Channel/NavigationStatus/INavigationStatusServiceEventHandler.hpp"
#include "aasdk/Channel/NavigationStatus/NavigationStatusService.hpp"
#include "aasdk/Common/Log.hpp"
#include <aasdk/Common/ModernLogger.hpp>

/*
 * This is a Navigation Status channel that could be used for integration onto another Raspberry Pi/Other Device to add an additional screen for notification and control purposes - such as updating the LCD screen on older Vauxhall/Opel/GM Cars
 */

namespace aasdk::channel::navigationstatus {

  NavigationStatusService::NavigationStatusService(boost::asio::io_service::strand &strand,
                                                   messenger::IMessenger::Pointer messenger)
      : Channel(strand, std::move(messenger), messenger::ChannelId::NAVIGATION_STATUS) {

  }

  void NavigationStatusService::receive(INavigationStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_NAVIGATION(debug, "receive()");

    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&NavigationStatusService::messageHandler, this->shared_from_this(), std::placeholders::_1,
                  eventHandler),
        std::bind(&INavigationStatusServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void NavigationStatusService::sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                                                        SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_NAVIGATION(debug, "sendChannelOpenResponse()");

    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }


  void NavigationStatusService::messageHandler(messenger::Message::Pointer message,
                                               INavigationStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_NAVIGATION(debug, "messageHandler()");

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::navigationstatus::NavigationStatusMessageId::INSTRUMENT_CLUSTER_NAVIGATION_STATUS:
        this->handleStatusUpdate(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::navigationstatus::NavigationStatusMessageId::INSTRUMENT_CLUSTER_NAVIGATION_TURN_EVENT:
        this->handleTurnEvent(payload, std::move(eventHandler));
        break;
      case aap_protobuf::service::navigationstatus::NavigationStatusMessageId::INSTRUMENT_CLUSTER_NAVIGATION_DISTANCE_EVENT:
        this->handleDistanceEvent(payload, std::move(eventHandler));
        break;
      default:
        AASDK_LOG(error) << "[NavigationStatusService] Message Id not Handled: " << messageId.getId() << " : "
                         << dump(payload);
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void NavigationStatusService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                                         INavigationStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_NAVIGATION(debug, "handleChannelOpenRequest()");

    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }

  void NavigationStatusService::handleStatusUpdate(const common::DataConstBuffer &payload,
                                                   INavigationStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_NAVIGATION(debug, "handleStatusUpdate()");
    aap_protobuf::service::navigationstatus::message::NavigationStatus navStatus;
    if (navStatus.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onStatusUpdate(navStatus);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
      AASDK_LOG(error) << "[NavigationStatusService] encountered error with message: " << dump(payload);
    }

  }

  void NavigationStatusService::handleTurnEvent(const common::DataConstBuffer &payload,
                                                INavigationStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_NAVIGATION(debug, "handleTurnEvent()");
    aap_protobuf::service::navigationstatus::message::NavigationNextTurnEvent turnEvent;
    if (turnEvent.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onTurnEvent(turnEvent);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
      AASDK_LOG(error) << "[NavigationStatusService] encountered error with message: " << dump(payload);
    }

  }

  void NavigationStatusService::handleDistanceEvent(const common::DataConstBuffer &payload,
                                                    INavigationStatusServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_NAVIGATION(debug, "handleDistanceEvent()");
    aap_protobuf::service::navigationstatus::message::NavigationNextTurnDistanceEvent distanceEvent;
    if (distanceEvent.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onDistanceEvent(distanceEvent);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
      AASDK_LOG(error) << "[NavigationStatusService] encountered error with message: " << dump(payload);
    }

  }

}


