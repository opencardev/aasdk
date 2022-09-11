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

}

void MessageInStream::startReceive(ReceivePromise::Pointer promise)
{
    strand_.dispatch([this, self = this->shared_from_this(), promise = std::move(promise)]() mutable {
        if (promise_ == nullptr) {
            promise_ = std::move(promise);
            auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
            transportPromise->then(
                    [this, self = this->shared_from_this()](common::Data data) mutable {
                        this->receiveFrameHeaderHandler(common::DataConstBuffer(data));
                    },
                    [this, self = this->shared_from_this()](const error::Error &e) mutable {
                        promise_->reject(e);
                        promise_.reset();
                    });

            transport_->receive(FrameHeader::getSizeOf(), std::move(transportPromise));
        } else {
            promise->reject(error::Error(error::ErrorCode::OPERATION_IN_PROGRESS));
        }
    });
}

void MessageInStream::receiveFrameHeaderHandler(const common::DataConstBuffer& buffer)
{
    FrameHeader frameHeader(buffer);

    AASDK_LOG(debug) << "[MessageInStream] Processing Frame Header: Ch " << channelIdToString(frameHeader.getChannelId()) << " Fr " << frameTypeToString(frameHeader.getType());

    isValidFrame_ = true;

    auto bufferedMessage = messageBuffer_.find(frameHeader.getChannelId());
    if (bufferedMessage != messageBuffer_.end()) {
        // We have found a message...
        message_ = std::move(bufferedMessage->second);
        messageBuffer_.erase(bufferedMessage);

        AASDK_LOG(debug) << "[MessageInStream] Found existing message.";

        if (frameHeader.getType() == FrameType::FIRST || frameHeader.getType() == FrameType::BULK) {
            // If it's first or bulk, we need to override the message anyhow, so we will start again.
            // Need to start a new message anyhow
            message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(), frameHeader.getMessageType());
        }
    } else {
        AASDK_LOG(debug) << "[MessageInStream] Could not find existing message.";
        // No Message Found in Buffers and this is a middle or last frame, this an error.
        // Still need to process the frame, but we will not resolve at the end.
        message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(), frameHeader.getMessageType());
        if (frameHeader.getType() == FrameType::MIDDLE || frameHeader.getType() == FrameType::LAST) {
            // This is an error
            isValidFrame_ = false;
        }
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
        AASDK_LOG(debug) << "[MessageInStream] Resolving message.";
        promise_->resolve(std::move(message_));
        promise_.reset();
        isResolved = true;

        //currentMessageIndex_--;
    } else {
        // First or Middle message, we'll store in our buffer...
        messageBuffer_[message_->getChannelId()] = std::move(message_);
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
