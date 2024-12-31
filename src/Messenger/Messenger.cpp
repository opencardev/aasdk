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
#include <aasdk/Error/Error.hpp>
#include <aasdk/Messenger/Messenger.hpp>
#include <aasdk/Common/Log.hpp>

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
    currentPromiseIndex_ = 0;
    currentMessageIndex_ = 0;
}

void Messenger::enqueueReceive(ChannelId channelId, ReceivePromise::Pointer promise)
{
    // enqueueReceive is called from the service channel.
    AASDK_LOG(debug) << "[Messenger] 1. enqueueReceived called on Channel " << channelIdToString(channelId);

    receiveStrand_.dispatch([this, self = this->shared_from_this(), channelId, promise = std::move(promise)]() mutable {
        auto interleavedPromise = ReceivePromise::defer(receiveStrand_);
        interleavedPromise->then(std::bind(&Messenger::interleavedMessageHandler, this->shared_from_this(), std::placeholders::_1),
                                 std::bind(&Messenger::rejectInterleavedMessageHandler, this->shared_from_this(), std::placeholders::_1));
        messageInStream_->setInterleavedHandler(std::move(interleavedPromise));

        //If there's any messages on the channel, resolve. The channel will call enqueueReceive again.
        if(!channelReceiveMessageQueue_.empty(channelId))
        {
            promise->resolve(std::move(channelReceiveMessageQueue_.pop(channelId)));
            AASDK_LOG(debug) << "[Messenger] 2. Message Queue not Empty. Resolving promise with Existing Message.";
        }
        else
        {
            currentPromiseIndex_++;
            AASDK_LOG(debug) << "[Messenger] 3. Message Queue is Empty. Pushing Promise on Index " << std::to_string(currentPromiseIndex_);
            channelReceivePromiseQueue_.push(channelId, std::move(promise));

            if(channelReceivePromiseQueue_.size() == 1)
            {
                currentMessageIndex_++;

                AASDK_LOG(debug) << "[Messenger] 4. Calling startReceive on MessageIndex " << std::to_string(currentMessageIndex_);

                auto inStreamPromise = ReceivePromise::defer(receiveStrand_);
                inStreamPromise->then(std::bind(&Messenger::inStreamMessageHandler, this->shared_from_this(), std::placeholders::_1),
                                     std::bind(&Messenger::rejectReceivePromiseQueue, this->shared_from_this(), std::placeholders::_1));
                messageInStream_->startReceive(std::move(inStreamPromise), channelId, currentPromiseIndex_, currentMessageIndex_);
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
    AASDK_LOG(debug) << "[Messenger] 5. inStreamMessageHandler from Channel " << channelIdToString(channelId);

    currentMessageIndex_--;

    // If there's a promise on the queue, we resolve the promise with this message....
    if(channelReceivePromiseQueue_.isPending(channelId))
    {
        channelReceivePromiseQueue_.pop(channelId)->resolve(std::move(message));
        currentPromiseIndex_--;
    }
    else
    {
        // Or we push the message to the Message Queue for when we do get a promise
        AASDK_LOG(debug) << "[Messenger] 7. Pushing Message to Queue as we have no Pending Promises." << std::to_string(currentPromiseIndex_);
        channelReceiveMessageQueue_.push(std::move(message));
    }

    if(!channelReceivePromiseQueue_.empty())
    {
        currentMessageIndex_++;
        AASDK_LOG(debug) << "[Messenger] 8. Calling startReceive on PromiseIndex " << std::to_string(currentPromiseIndex_) << " and MessageIndex " << std::to_string(currentMessageIndex_);

        auto inStreamPromise = ReceivePromise::defer(receiveStrand_);
        inStreamPromise->then(std::bind(&Messenger::inStreamMessageHandler, this->shared_from_this(), std::placeholders::_1),
                             std::bind(&Messenger::rejectReceivePromiseQueue, this->shared_from_this(), std::placeholders::_1));
        messageInStream_->startReceive(std::move(inStreamPromise), channelId, currentPromiseIndex_,currentMessageIndex_);
    }
}

void Messenger::doSend()
{
    auto queueElementIter = channelSendPromiseQueue_.begin();
    auto outStreamPromise = SendPromise::defer(sendStrand_);
    outStreamPromise->then(std::bind(&Messenger::outStreamMessageHandler, this->shared_from_this(), queueElementIter),
                           std::bind(&Messenger::rejectSendPromiseQueue, this->shared_from_this(), std::placeholders::_1));

    messageOutStream_->stream(std::move(queueElementIter->first), std::move(outStreamPromise));
}

void Messenger::interleavedMessageHandler(Message::Pointer message)
{
    auto channelId = message->getChannelId();

    AASDK_LOG(debug) << "[Messenger] 9. interleavedMessageHandler from Channel " << channelIdToString(channelId);

    channelReceiveMessageQueue_.push(std::move(message));

    auto interleavedPromise = ReceivePromise::defer(receiveStrand_);
    interleavedPromise->then(std::bind(&Messenger::interleavedMessageHandler, this->shared_from_this(), std::placeholders::_1),
                                std::bind(&Messenger::rejectInterleavedMessageHandler, this->shared_from_this(), std::placeholders::_1));
    messageInStream_->setInterleavedHandler(std::move(interleavedPromise));
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
        currentPromiseIndex_--;
    }
}

void Messenger::rejectInterleavedMessageHandler(const error::Error& e)
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
    currentPromiseIndex_ = 0;
    receiveStrand_.dispatch([this, self = this->shared_from_this()]() {
        channelReceiveMessageQueue_.clear();
    });
}

}
}
