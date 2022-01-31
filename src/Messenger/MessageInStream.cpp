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

#include <f1x/aasdk/Messenger/MessageInStream.hpp>
#include <f1x/aasdk/Error/Error.hpp>
#include <f1x/aasdk/Common/Log.hpp>

#include <iostream>
namespace f1x
{
namespace aasdk
{
namespace messenger
{

MessageInStream::MessageInStream(boost::asio::io_service& ioService, transport::ITransport::Pointer transport, ICryptor::Pointer cryptor)
    : strand_(ioService)
    , transport_(std::move(transport))
    , cryptor_(std::move(cryptor))
{
    isNewMessage_ = true;
}

void MessageInStream::startReceive(ReceivePromise::Pointer promise)
{
    strand_.dispatch([this, self = this->shared_from_this(), promise = std::move(promise)]() mutable {
        if(promise_ == nullptr)
        {
            promise_ = std::move(promise);
            isNewMessage_ = true;
            auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
            transportPromise->then(
                [this, self = this->shared_from_this()](common::Data data) mutable {
                    this->receiveFrameHeaderHandler(common::DataConstBuffer(data));
                },
                [this, self = this->shared_from_this()](const error::Error& e) mutable {
                    promise_->reject(e);
                    promise_.reset();
                });

            transport_->receive(FrameHeader::getSizeOf(), std::move(transportPromise));
        }
        else
        {
            promise->reject(error::Error(error::ErrorCode::OPERATION_IN_PROGRESS));
        }
    });
}

void MessageInStream::setInterleavedHandler(ReceivePromise::Pointer promise)
{
    interleavedPromise_ = std::move(promise);
}

void MessageInStream::receiveFrameHeaderHandler(const common::DataConstBuffer& buffer)
{
    FrameHeader frameHeader(buffer);

    isInterleaved_ = false;

    // Store the ChannelId if this is a new message.
    if (isNewMessage_) {
        originalMessageChannelId_ = frameHeader.getChannelId();
        isNewMessage_ = false;
    }

    // If Frame Channel does not match Message Channel, store Existing Message in Buffer.
    if(message_ != nullptr && message_->getChannelId() != frameHeader.getChannelId())
    {
        AASDK_LOG(debug) << "[MessageInStream] ChannelId mismatch -- Frame " << channelIdToString(frameHeader_.getChannelId()) << " -- Message -- " << channelIdToString(message_.getChannelId());
        isInterleaved_ = true;

        messageBuffer_[message_->getChannelId()] = message_;
        message_ = nullptr;
    }

    if (frameHeader.getType() == FrameType::FIRST || frameHeader.getType() == FrameType::BULK) {
        // If it's a First or Bulk Frame, we need to start a new message.
        message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(), frameHeader.getMessageType());
    } else {
        // This is a Middle or Last Frame. We must find an existing message.
        auto bufferedMessage = messageBuffer_.find(frameHeader.getChannelId());
        if (bufferedMessage != messageBuffer_.end()) {
            /*
             * If the original channel does not match, then this is an interleaved frame.
             * It is no good just to match the channelid on the message we recovered from the queue, we need
             * to go back to the original message channel as even this frame may be ANOTHER interleaved
             * message frame within an existing interleaved message.
             * Our promise must resolve only the channel id we've been tasked to work on.
             * Everything else is incidental.
             */

            // We can restore the original message, and and if the current frame matches the chnnale id
            // then we're not interleaved anymore.
            if (originalMessageChannelId_ == frameHeader.getChannelId()) {
                isInterleaved_ = false;
                AASDK_LOG(debug) << "[MessageInStream] Restored Message from Buffer";
            }

            message_ = bufferedMessage->second;
            messageBuffer_.erase(bufferedMessage);
        }
    }

    // If we have nothing at this point, start a new message.
    if(message_ == nullptr)
    {
        message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(), frameHeader.getMessageType());
    }

    thisFrameType_ = frameHeader.getType();
    const size_t frameSize = FrameSize::getSizeOf(frameHeader.getType() == FrameType::FIRST ? FrameSizeType::EXTENDED : FrameSizeType::SHORT);

    auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
    transportPromise->then(
        [this, self = this->shared_from_this()](common::Data data) mutable {
            this->receiveFrameSizeHandler(common::DataConstBuffer(data));
        },
        [this, self = this->shared_from_this()](const error::Error& e) mutable {
            message_.reset();
            promise_->reject(e);
            promise_.reset();
        });

    transport_->receive(frameSize, std::move(transportPromise));
}

void MessageInStream::receiveFrameSizeHandler(const common::DataConstBuffer& buffer)
{
    auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
    transportPromise->then(
        [this, self = this->shared_from_this()](common::Data data) mutable {
            this->receiveFramePayloadHandler(common::DataConstBuffer(data));
        },
        [this, self = this->shared_from_this()](const error::Error& e) mutable {
            message_.reset();
            promise_->reject(e);
            promise_.reset();
        });

    FrameSize frameSize(buffer);
    frameSize_ = (int) frameSize.getSize();
    transport_->receive(frameSize.getSize(), std::move(transportPromise));
}

void MessageInStream::receiveFramePayloadHandler(const common::DataConstBuffer& buffer)
{   
    if(message_->getEncryptionType() == EncryptionType::ENCRYPTED)
    {
        try
        {
            cryptor_->decrypt(message_->getPayload(), buffer, frameSize_);
        }
        catch(const error::Error& e)
        {
            message_.reset();
            promise_->reject(e);
            promise_.reset();
            return;
        }
    }
    else
    {
        message_->insertPayload(buffer);
    }

    bool isResolved = false;

    // If this is the LAST frame or a BULK frame...
    if(thisFrameType_ == FrameType::BULK || thisFrameType_ == FrameType::LAST)
    {
        // If this isn't an interleaved frame, then we can resolve the promise
        if (!isInterleaved_) {
            isResolved = true;
            promise_->resolve(std::move(message_));
            promise_.reset();
        } else {
            // Otherwise resolve through our random promise
            interleavedPromise_->resolve(std::move(message_));
        }
    }

    // If the main promise isn't resolved, then carry on retrieving frame headers.
    if (!isResolved) {
        auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
        transportPromise->then(
                [this, self = this->shared_from_this()](common::Data data) mutable {
                    this->receiveFrameHeaderHandler(common::DataConstBuffer(data));
                },
                [this, self = this->shared_from_this()](const error::Error& e) mutable {
                    message_.reset();
                    promise_->reject(e);
                    promise_.reset();
                });

        transport_->receive(FrameHeader::getSizeOf(), std::move(transportPromise));
    }
}

}
}
}
