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
#include <aasdk_proto/MediaInfoChannelMessageIdsEnum.pb.h>
#include <aasdk_proto/MediaInfoChannelMetadataData.pb.h>
#include <aasdk_proto/MediaInfoChannelPlaybackData.pb.h>
#include <aasdk_proto/BluetoothChannelMessageIdsEnum.pb.h>
#include <aasdk_proto/BluetoothPairingRequestMessage.pb.h>
#include <aasdk/Channel/AV/IMediaStatusServiceChannelEventHandler.hpp>
#include <aasdk/Channel/AV/MediaStatusServiceChannel.hpp>
#include <aasdk/Common/Log.hpp>


namespace aasdk
{
namespace channel
{
namespace av
{

MediaStatusServiceChannel::MediaStatusServiceChannel(boost::asio::io_service::strand& strand, messenger::IMessenger::Pointer messenger)
    : ServiceChannel(strand, std::move(messenger), messenger::ChannelId::MEDIA_STATUS)
{

}

void MediaStatusServiceChannel::receive(IMediaStatusServiceChannelEventHandler::Pointer eventHandler)
{
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(std::bind(&MediaStatusServiceChannel::messageHandler, this->shared_from_this(), std::placeholders::_1, eventHandler),
                        std::bind(&IMediaStatusServiceChannelEventHandler::onChannelError, eventHandler,std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
}

messenger::ChannelId MediaStatusServiceChannel::getId() const
{
    return channelId_;
}

void MediaStatusServiceChannel::sendChannelOpenResponse(const proto::messages::ChannelOpenResponse& response, SendPromise::Pointer promise)
{
    AASDK_LOG(info) << "[MediaStatusServiceChannel] channel open response ";
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED, messenger::MessageType::CONTROL));
    message->insertPayload(messenger::MessageId(proto::ids::ControlMessage::CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
}


void MediaStatusServiceChannel::messageHandler(messenger::Message::Pointer message, IMediaStatusServiceChannelEventHandler::Pointer eventHandler)
{
    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch(messageId.getId())
    {
    case proto::ids::ControlMessage::CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
        break;

    case proto::ids::MediaInfoChannelMessage::METADATA:
        this->handleMetadataUpdate(payload, std::move(eventHandler));
        break;

    case proto::ids::MediaInfoChannelMessage::PLAYBACK:
        this->handlePlaybackUpdate(payload, std::move(eventHandler));
        break;

    default:
        AASDK_LOG(error) << "[MediaStatusServiceChannel] message not handled: " << messageId.getId() << " : " << dump(payload);
        this->receive(std::move(eventHandler));
        break;
    }


    
}

void MediaStatusServiceChannel::handleMetadataUpdate(const common::DataConstBuffer& payload, IMediaStatusServiceChannelEventHandler::Pointer eventHandler)
{
    proto::messages::MediaInfoChannelMetadataData metadata;
    if(metadata.ParseFromArray(payload.cdata, payload.size))
    {
        eventHandler->onMetadataUpdate(metadata);
    }
    else
    {
        eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        AASDK_LOG(error) << "[MediaStatusServiceChannel] encountered error with message: " << dump(payload);
    }

}


void MediaStatusServiceChannel::handlePlaybackUpdate(const common::DataConstBuffer& payload, IMediaStatusServiceChannelEventHandler::Pointer eventHandler)
{
    proto::messages::MediaInfoChannelPlaybackData playback;
    if(playback.ParseFromArray(payload.cdata, payload.size))
    {
        eventHandler->onPlaybackUpdate(playback);
    }
    else
    {
        eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        AASDK_LOG(error) << "[MediaStatusServiceChannel] encountered error with message: " << dump(payload);
    }

}

void MediaStatusServiceChannel::handleChannelOpenRequest(const common::DataConstBuffer& payload, IMediaStatusServiceChannelEventHandler::Pointer eventHandler)
{
    AASDK_LOG(info) << "[MediaStatusServiceChannel] channel open request ";

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


}
}
}
