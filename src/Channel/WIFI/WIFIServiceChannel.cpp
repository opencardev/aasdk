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

#include "aasdk/Channel/WIFI/WIFIServiceChannel.hpp"
#include <aasdk_proto/ControlMessageIdsEnum.pb.h>
#include <aasdk_proto/WifiChannelMessageIdsEnum.pb.h>
#include <aasdk_proto/WifiSecurityResponseMessage.pb.h>
#include <aasdk/Channel/WIFI/IWIFIServiceChannelEventHandler.hpp>
#include <aasdk/Channel/WIFI/WIFIServiceChannel.hpp>
#include <aasdk/Common/Log.hpp>


namespace aasdk
{
namespace channel
{
namespace wifi
{

WIFIServiceChannel::WIFIServiceChannel(boost::asio::io_service::strand& strand, messenger::IMessenger::Pointer messenger)
    : ServiceChannel(strand, std::move(messenger), messenger::ChannelId::WIFI)
{

}

void WIFIServiceChannel::receive(IWIFIServiceChannelEventHandler::Pointer eventHandler)
{
    AASDK_LOG(info) << "[WIFIServiceChannel] receive ";

    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(std::bind(&WIFIServiceChannel::messageHandler, this->shared_from_this(), std::placeholders::_1, eventHandler),
                        std::bind(&IWIFIServiceChannelEventHandler::onChannelError, eventHandler,std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
}

messenger::ChannelId WIFIServiceChannel::getId() const
{
    return channelId_;
}

void WIFIServiceChannel::sendChannelOpenResponse(const proto::messages::ChannelOpenResponse& response, SendPromise::Pointer promise)
{
    AASDK_LOG(info) << "[WIFIServiceChannel] send channel open response ";

    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED, messenger::MessageType::CONTROL));
    message->insertPayload(messenger::MessageId(proto::ids::ControlMessage::CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
}

void WIFIServiceChannel::sendWifiSecurityResponse(const proto::messages::WifiSecurityResponse &response, SendPromise::Pointer promise) {
    AASDK_LOG(info) << "[WIFIServiceChannel] send wifi security response ";

    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED, messenger::MessageType::SPECIFIC));
    message->insertPayload(messenger::MessageId(proto::ids::WifiChannelMessage_Enum::WifiChannelMessage_Enum_CREDENTIALS_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
}

void WIFIServiceChannel::messageHandler(messenger::Message::Pointer message, IWIFIServiceChannelEventHandler::Pointer eventHandler)
{
    AASDK_LOG(info) << "[WIFIServiceChannel] message handler ";

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch(messageId.getId())
    {
    case proto::ids::ControlMessage::CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;
    case proto::ids::WifiChannelMessage::CREDENTIALS_REQUEST:
        this->handleWifiSecurityRequest(payload, std::move(eventHandler));
        break;
    default:
        AASDK_LOG(error) << "[WIFIServiceChannel] message not handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
}

void WIFIServiceChannel::handleChannelOpenRequest(const common::DataConstBuffer& payload, IWIFIServiceChannelEventHandler::Pointer eventHandler)
{
    AASDK_LOG(info) << "[WIFIServiceChannel] channel open request ";

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

void WIFIServiceChannel::handleWifiSecurityRequest(const common::DataConstBuffer &payload, IWIFIServiceChannelEventHandler::Pointer eventHandler) {
    AASDK_LOG(info) << "[WIFIServiceChannel] wifi security request ";
    eventHandler->onWifiSecurityRequest();
}
}
}
}
