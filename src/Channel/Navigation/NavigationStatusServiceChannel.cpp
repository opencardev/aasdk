/*
*  This file is part of aasdk library project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

#include <aasdk_proto/ControlMessageIdsEnum.pb.h>
#include <aasdk_proto/NavigationChannelMessageIdsEnum.pb.h>
#include <aasdk_proto/NavigationStatusMessage.pb.h>
#include <aasdk_proto/NavigationTurnEventMessage.pb.h>
#include <aasdk_proto/NavigationDistanceEventMessage.pb.h>
#include <aasdk/Channel/Navigation/INavigationStatusServiceChannelEventHandler.hpp>
#include <aasdk/Channel/Navigation/NavigationStatusServiceChannel.hpp>
#include <aasdk/Common/Log.hpp>


namespace aasdk
{
namespace channel
{
namespace navigation
{

NavigationStatusServiceChannel::NavigationStatusServiceChannel(boost::asio::io_service::strand& strand, messenger::IMessenger::Pointer messenger)
    : ServiceChannel(strand, std::move(messenger), messenger::ChannelId::NAVIGATION)
{

}

void NavigationStatusServiceChannel::receive(INavigationStatusServiceChannelEventHandler::Pointer eventHandler)
{
        AASDK_LOG(info) << "[NavigationStatusServiceChannel] receive ";

    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(std::bind(&NavigationStatusServiceChannel::messageHandler, this->shared_from_this(), std::placeholders::_1, eventHandler),
                        std::bind(&INavigationStatusServiceChannelEventHandler::onChannelError, eventHandler,std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
}

messenger::ChannelId NavigationStatusServiceChannel::getId() const
{
    return channelId_;
}

void NavigationStatusServiceChannel::sendChannelOpenResponse(const proto::messages::ChannelOpenResponse& response, SendPromise::Pointer promise)
{
    AASDK_LOG(info) << "[NavigationStatusServiceChannel] channel open response ";

    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED, messenger::MessageType::CONTROL));
    message->insertPayload(messenger::MessageId(proto::ids::ControlMessage::CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
}


void NavigationStatusServiceChannel::messageHandler(messenger::Message::Pointer message, INavigationStatusServiceChannelEventHandler::Pointer eventHandler)
{
        AASDK_LOG(info) << "[NavigationStatusServiceChannel] message handler ";

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch(messageId.getId())
    {
    case proto::ids::ControlMessage::CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
    case proto::ids::NavigationChannelMessage::STATUS:
        this->handleStatusUpdate(payload, std::move(eventHandler));
        break;
    case proto::ids::NavigationChannelMessage::TURN_EVENT:
        this->handleTurnEvent(payload, std::move(eventHandler));
        break;
    case proto::ids::NavigationChannelMessage::DISTANCE_EVENT:
        this->handleDistanceEvent(payload, std::move(eventHandler));
        break;

    
    default:
        AASDK_LOG(error) << "[NavigationStatusServiceChannel] message not handled: " << messageId.getId() << " : " << dump(payload);
        this->receive(std::move(eventHandler));
        break;
    }
}

void NavigationStatusServiceChannel::handleChannelOpenRequest(const common::DataConstBuffer& payload, INavigationStatusServiceChannelEventHandler::Pointer eventHandler)
{
        AASDK_LOG(info) << "[NavigationStatusServiceChannel] channel open request ";

    proto::messages::ChannelOpenRequest request;
    if(request.ParseFromArray(payload.cdata, payload.size))
    {
        eventHandler->onChannelOpenRequest(request);
    }
    else
    {
        eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
}

void NavigationStatusServiceChannel::handleStatusUpdate(const common::DataConstBuffer& payload, INavigationStatusServiceChannelEventHandler::Pointer eventHandler)
{
    proto::messages::NavigationStatus navStatus;
    if(navStatus.ParseFromArray(payload.cdata, payload.size))
    {
        eventHandler->onStatusUpdate(navStatus);
    }
    else
    {
        eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        AASDK_LOG(error) << "[NavigationStatusServiceChannel] encountered error with message: " << dump(payload);
    }

}

void NavigationStatusServiceChannel::handleTurnEvent(const common::DataConstBuffer& payload, INavigationStatusServiceChannelEventHandler::Pointer eventHandler)
{
    proto::messages::NavigationTurnEvent turnEvent;
    if(turnEvent.ParseFromArray(payload.cdata, payload.size))
    {
        eventHandler->onTurnEvent(turnEvent);
    }
    else
    {
        eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        AASDK_LOG(error) << "[NavigationStatusServiceChannel] encountered error with message: " << dump(payload);
    }

}

void NavigationStatusServiceChannel::handleDistanceEvent(const common::DataConstBuffer& payload, INavigationStatusServiceChannelEventHandler::Pointer eventHandler)
{
    proto::messages::NavigationDistanceEvent distanceEvent;
    if(distanceEvent.ParseFromArray(payload.cdata, payload.size))
    {
        eventHandler->onDistanceEvent(distanceEvent);
    }
    else
    {
        eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        AASDK_LOG(error) << "[NavigationStatusServiceChannel] encountered error with message: " << dump(payload);
    }

}

}
}
}
