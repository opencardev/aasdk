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

#include <boost/endian/conversion.hpp>
#include <f1x/aasdk/Error/Error.hpp>
#include <f1x/aasdk/Messenger/Messenger.hpp>
#include <f1x/aasdk/Common/Log.hpp>

namespace f1x
{
namespace aasdk
{
namespace messenger
{

Messenger::Messenger(boost::asio::io_service& ioService, IMessageInStream::Pointer messageInStream, IMessageOutStream::Pointer messageOutStream)
    : receiveStrand_(ioService)
    , sendStrand_(ioService)
    , messageInStream_(std::move(messageInStream))
    , messageOutStream_(std::move(messageOutStream))
{

}

void Messenger::enqueueReceive(ChannelId channelId, ReceivePromise::Pointer promise)
{
    receiveStrand_.dispatch([this, self = this->shared_from_this(), channelId, promise = std::move(promise)]() mutable {
        if(!channelReceiveMessageQueue_.empty(channelId))
        {
            //TODO: Use this to check on the Frame/Channel Id?
            //this->parseMessage(channelReceiveMessageQueue_.pop(channelId), promise);
            promise->resolve(std::move(channelReceiveMessageQueue_.pop(channelId)));
            //TODO: Problem with resolving like this, is that if we resolve an interleave frame, our resolve goes to the wrong channel - eg Audio on VideoServiceChannel
        }
        else
        {
            channelReceivePromiseQueue_.push(channelId, std::move(promise));

            if(channelReceivePromiseQueue_.size() == 1)
            {
                auto inStreamPromise = ReceivePromise::defer(receiveStrand_);
                inStreamPromise->then(std::bind(&Messenger::inStreamMessageHandler, this->shared_from_this(), std::placeholders::_1),
                                     std::bind(&Messenger::rejectReceivePromiseQueue, this->shared_from_this(), std::placeholders::_1));
                messageInStream_->startReceive(std::move(inStreamPromise));

                auto randomInStreamPromise = ReceivePromise::defer(receiveStrand_);
                randomInStreamPromise->then(std::bind(&Messenger::randomInStreamMessageHandler, this->shared_from_this(), std::placeholders::_1),
                                            std::bind(&Messenger::randomRejectReceivePromiseQueue, this->shared_from_this(), std::placeholders::_1));
                messageInStream_->setInterleavedHandler(std::move(randomInStreamPromise));
            }
        }
    });
}

void Messenger::enqueueSend(Message::Pointer message, SendPromise::Pointer promise)
{
    sendStrand_.dispatch([this, self = this->shared_from_this(), message = std::move(message), promise = std::move(promise)]() mutable {
        channelSendPromiseQueue_.emplace_back(std::make_pair(std::move(message), std::move(promise)));

        if(channelSendPromiseQueue_.size() == 1)
        {
            this->doSend();
        }
    });
}

void Messenger::inStreamMessageHandler(Message::Pointer message)
{
    auto channelId = message->getChannelId();
    if (message->getChannelId() != ChannelId::VIDEO) {
        //AASDK_LOG(debug) << channelIdToString(message->getChannelId()) << ": " << common::dump(message->getPayload());
    }

    if(channelReceivePromiseQueue_.isPending(channelId))
    {
        //this->parseMessage(message, channelReceivePromiseQueue_.pop(channelId));
        channelReceivePromiseQueue_.pop(channelId)->resolve(std::move(message));
    }
    else
    {
        channelReceiveMessageQueue_.push(std::move(message));
    }

    if(!channelReceivePromiseQueue_.empty())
    {
        auto inStreamPromise = ReceivePromise::defer(receiveStrand_);
        inStreamPromise->then(std::bind(&Messenger::inStreamMessageHandler, this->shared_from_this(), std::placeholders::_1),
                             std::bind(&Messenger::rejectReceivePromiseQueue, this->shared_from_this(), std::placeholders::_1));
        messageInStream_->startReceive(std::move(inStreamPromise));
    }
}

void Messenger::randomInStreamMessageHandler(Message::Pointer message)
{
    //AASDK_LOG(debug) << "Interleaved Message Pushed to Queue";;
    channelReceiveMessageQueue_.push(std::move(message));

    auto randomInStreamPromise = ReceivePromise::defer(receiveStrand_);
    randomInStreamPromise->then(std::bind(&Messenger::randomInStreamMessageHandler, this->shared_from_this(), std::placeholders::_1),
                          std::bind(&Messenger::randomRejectReceivePromiseQueue, this->shared_from_this(), std::placeholders::_1));
    messageInStream_->setInterleavedHandler(std::move(randomInStreamPromise));
}

void Messenger::parseMessage(Message::Pointer message, ReceivePromise::Pointer promise) {
    if (message->getChannelId() != ChannelId::VIDEO) {
        //AASDK_LOG(debug) << channelIdToString(message->getChannelId()) << " " << MessageId(message->getPayload());
    }
    promise->resolve(message);
}

void Messenger::doSend()
{
    auto queueElementIter = channelSendPromiseQueue_.begin();
    auto outStreamPromise = SendPromise::defer(sendStrand_);
    outStreamPromise->then(std::bind(&Messenger::outStreamMessageHandler, this->shared_from_this(), queueElementIter),
                           std::bind(&Messenger::rejectSendPromiseQueue, this->shared_from_this(), std::placeholders::_1));

    messageOutStream_->stream(std::move(queueElementIter->first), std::move(outStreamPromise));
}

void Messenger::outStreamMessageHandler(ChannelSendQueue::iterator queueElement)
{
    queueElement->second->resolve();
    channelSendPromiseQueue_.erase(queueElement);

    if(!channelSendPromiseQueue_.empty())
    {
        this->doSend();
    }
}

void Messenger::rejectReceivePromiseQueue(const error::Error& e)
{
    while(!channelReceivePromiseQueue_.empty())
    {
        channelReceivePromiseQueue_.pop()->reject(e);
    }
}

void Messenger::randomRejectReceivePromiseQueue(const error::Error& e)
{
    // Dummy
}

void Messenger::rejectSendPromiseQueue(const error::Error& e)
{
    while(!channelSendPromiseQueue_.empty())
    {
        auto queueElement(std::move(channelSendPromiseQueue_.front()));
        channelSendPromiseQueue_.pop_front();
        queueElement.second->reject(e);
    }
}

void Messenger::stop()
{
    receiveStrand_.dispatch([this, self = this->shared_from_this()]() {
        channelReceiveMessageQueue_.clear();
    });
}

}
}
}
