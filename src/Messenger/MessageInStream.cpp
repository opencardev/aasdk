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

#include <aasdk/Messenger/MessageInStream.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/Common/Log.hpp>
#include <iostream>

namespace aasdk
{
namespace messenger
{

MessageInStream::MessageInStream(boost::asio::io_service& ioService, transport::ITransport::Pointer transport, ICryptor::Pointer cryptor)
    : strand_(ioService)
    , transport_(std::move(transport))
    , cryptor_(std::move(cryptor))
{
    currentMessageIndex_ = 0;
}

void MessageInStream::startReceive(ReceivePromise::Pointer promise, ChannelId channelId, int promiseIndex, int messageIndex)
{
    AASDK_LOG(debug) << "[MessageInStream] 1. Start Receive called with channel " << channelIdToString(channelId) << " PI " << std::to_string(promiseIndex) << " MI " << std::to_string(messageIndex);

    strand_.dispatch([this, self = this->shared_from_this(), promise = std::move(promise)]() mutable {
        if (promise_ == nullptr) {
            promise_ = std::move(promise);
            auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
            transportPromise->then(
                    [this, self = this->shared_from_this()](common::Data data) mutable {
                        this->receiveFrameHeaderHandler(common::DataConstBuffer(data));
                    },
                    [this, self = this->shared_from_this()](const error::Error &e) mutable {
                        AASDK_LOG(debug) << "[MessageInStream] 2. Error Here?";
                        promise_->reject(e);
                        promise_.reset();
                    });

            transport_->receive(FrameHeader::getSizeOf(), std::move(transportPromise));
        } else {
            AASDK_LOG(debug) << "[MessageInStream] 3. Operation in Progress.";
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

    AASDK_LOG(debug) << "[MessageInStream] 5. Processing Frame Header: Ch " << channelIdToString(frameHeader.getChannelId()) << " Fr " << frameTypeToString(frameHeader.getType());

    isValidFrame_ = true;
    isInterleaved_ = false;

    // New Promise or Interleaved
    if(message_ != nullptr && message_->getChannelId() != frameHeader.getChannelId()) {
        // We have an existing message but the channels don't match...
        AASDK_LOG(debug) << "[MessageInStream] 6. Interleaved ChannelId MisMatch - F: " << channelIdToString(frameHeader.getChannelId()) << " M: " << channelIdToString(message_->getChannelId());

        isInterleaved_ = true;

        // Store message in buffer;
        messageBuffer_[message_->getChannelId()] = message_;
        message_.reset();
    }

    // Look for Buffered Message
    if ((message_ == nullptr) && (frameHeader.getType() == FrameType::MIDDLE || frameHeader.getType() == FrameType::LAST)) {
        AASDK_LOG(debug) << "[MessageInStream] 7. Null Message but Middle or Last Frame.";
        auto bufferedMessage = messageBuffer_.find(frameHeader.getChannelId());
        if (bufferedMessage != messageBuffer_.end()) {
            AASDK_LOG(debug) << "[MessageInStream] 8. Found Existing Message on Channel.";

            message_ = bufferedMessage->second;
            messageBuffer_.erase(bufferedMessage);

            isInterleaved_ = false;
        }
    }

    if (message_ == nullptr) {
        if (frameHeader.getType() == FrameType::FIRST || frameHeader.getType() == FrameType::BULK) {
            AASDK_LOG(debug) << "[MessageInStream] 11. New message created with Index " << std::to_string(currentMessageIndex_);
            currentMessageIndex_++;
        } else {
            // This will be an invalid message, but we still need to read from the buffer.
            isValidFrame_ = false;
        }
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
    frameSize_ = (int) frameSize.getFrameSize();
    transport_->receive(frameSize.getFrameSize(), std::move(transportPromise));
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
    if((thisFrameType_ == FrameType::BULK || thisFrameType_ == FrameType::LAST) && isValidFrame_)
    {
        if (!isInterleaved_) {
            AASDK_LOG(debug) << "[MessageInStream] 12. Resolving Normal message. " << std::to_string(currentMessageIndex_);
            promise_->resolve(std::move(message_));
            promise_.reset();
            isResolved = true;
        } else {
            AASDK_LOG(debug) << "[MessageInStream] 13. Resolving Interleaved Message. " << std::to_string(currentMessageIndex_);
            interleavedPromise_->resolve(std::move(message_));
        }
        currentMessageIndex_--;
        message_.reset();
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
