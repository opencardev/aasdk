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

}

void MessageInStream::startReceive(ReceivePromise::Pointer promise)
{
    strand_.dispatch([this, self = this->shared_from_this(), promise = std::move(promise)]() mutable {
        if(promise_ == nullptr)
        {
            promise_ = std::move(promise);

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

void MessageInStream::receiveFrameHeaderHandler(const common::DataConstBuffer& buffer)
{
    FrameHeader frameHeader(buffer);
    if (buffer.cdata[0] != 3) {
        AASDK_LOG(debug) << "Message from channel " << std::to_string(buffer.cdata[0]);
    }

    if(message_ != nullptr && message_->getChannelId() != frameHeader.getChannelId())
    {
        // we have interleaved channels, stop working on the old one and store it for later; Switch to the new one
        channel_assembly_buffers[message_->getChannelId()] = message_;
        message_ = nullptr;
        // message_.reset();
        // promise_->reject(error::Error(error::ErrorCode::MESSENGER_INTERTWINED_CHANNELS));
        // promise_.reset();
        // return;
    }
    auto prevBuffer = channel_assembly_buffers.find(frameHeader.getChannelId());
    if(prevBuffer != channel_assembly_buffers.end()){ // is there previous data in our map?
        if(frameHeader.getType()!=FrameType::FIRST) //only use the data if we're not on a new frame, otherwise disregard
            message_ = prevBuffer->second;
        else{
            message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(), frameHeader.getMessageType());
        }
        channel_assembly_buffers.erase(prevBuffer); // get rid of the previously stored data because it's now our working data.
    }
    else if(message_ == nullptr){
        message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(), frameHeader.getMessageType());
    }
    recentFrameType_ = frameHeader.getType();
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

    if(recentFrameType_ == FrameType::BULK || recentFrameType_ == FrameType::LAST)
    {
        promise_->resolve(std::move(message_));
        promise_.reset();
    }
    else
    {
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
