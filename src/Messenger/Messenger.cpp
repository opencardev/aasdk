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
// along with aasdk. If not, see <http://www.gnu.org/licenses/>.#include <boost/endian/conversion.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/Messenger/Messenger.hpp>
#include <aasdk/Common/Log.hpp>
#include <aasdk/Common/ModernLogger.hpp>

namespace aasdk::messenger {

  Messenger::Messenger(boost::asio::io_service &ioService, IMessageInStream::Pointer messageInStream,
                       IMessageOutStream::Pointer messageOutStream)
      : receiveStrand_(ioService), sendStrand_(ioService), messageInStream_(std::move(messageInStream)),
        messageOutStream_(std::move(messageOutStream)) {

  }

  void Messenger::enqueueReceive(ChannelId channelId, ReceivePromise::Pointer promise) {
    AASDK_LOG(debug) << "[Messenger::enqueueReceive] Called on channel " << channelIdToString(channelId);

    // enqueueReceive is called from the service channel.
    receiveStrand_.dispatch([this, self = this->shared_from_this(), channelId, promise = std::move(promise)]() mutable {
      //If there's any messages on the service, resolve. The service will call enqueueReceive again.
      if (!channelReceiveMessageQueue_.empty(channelId)) {
        AASDK_LOG_MESSENGER(debug, "Message queue not empty, resolving message first.");
        promise->resolve(std::move(channelReceiveMessageQueue_.pop(channelId)));
      } else {
        AASDK_LOG_MESSENGER(debug, "Push promise to queue.");
        channelReceivePromiseQueue_.push(channelId, std::move(promise));

        if (channelReceivePromiseQueue_.size() == 1) {
          AASDK_LOG_MESSENGER(debug, "Processing promise.");
          auto inStreamPromise = ReceivePromise::defer(receiveStrand_);
          inStreamPromise->then(
              std::bind(&Messenger::inStreamMessageHandler, this->shared_from_this(), std::placeholders::_1),
              std::bind(&Messenger::rejectReceivePromiseQueue, this->shared_from_this(), std::placeholders::_1));
          messageInStream_->startReceive(std::move(inStreamPromise));
        }
      }
    });
  }

  void Messenger::enqueueSend(Message::Pointer message, SendPromise::Pointer promise) {
    sendStrand_.dispatch(
        [this, self = this->shared_from_this(), message = std::move(message), promise = std::move(promise)]() mutable {
          channelSendPromiseQueue_.emplace_back(std::move(message), std::move(promise));

          if (channelSendPromiseQueue_.size() == 1) {
            this->doSend();
          }
        });
  }

  void Messenger::inStreamMessageHandler(Message::Pointer message) {
    auto channelId = message->getChannelId();
    AASDK_LOG(debug) << "[Messenger::inStreamMessageHandler] Handling message for ChannelId "
                     << channelIdToString(message->getChannelId());

    // If there's a promise on the queue, we resolve the promise with this message....
    if (channelReceivePromiseQueue_.isPending(channelId)) {
      AASDK_LOG_MESSENGER(debug, "Pop and resolve message for message queue.");
      channelReceivePromiseQueue_.pop(channelId)->resolve(std::move(message));
    } else {
      AASDK_LOG_MESSENGER(debug, "Pushing message to receive queue.");
      // Or we push the message to the Message Queue for when we do get a promise
      channelReceiveMessageQueue_.push(std::move(message));
    }

    if (!channelReceivePromiseQueue_.empty()) {
      AASDK_LOG_MESSENGER(debug, "Initiate queue for receiving.");
      auto inStreamPromise = ReceivePromise::defer(receiveStrand_);
      inStreamPromise->then(
          std::bind(&Messenger::inStreamMessageHandler, this->shared_from_this(), std::placeholders::_1),
          std::bind(&Messenger::rejectReceivePromiseQueue, this->shared_from_this(), std::placeholders::_1));
      messageInStream_->startReceive(std::move(inStreamPromise));
    }
  }

  void Messenger::doSend() {
    auto queueElementIter = channelSendPromiseQueue_.begin();
    auto outStreamPromise = SendPromise::defer(sendStrand_);
    outStreamPromise->then(std::bind(&Messenger::outStreamMessageHandler, this->shared_from_this(), queueElementIter),
                           std::bind(&Messenger::rejectSendPromiseQueue, this->shared_from_this(),
                                     std::placeholders::_1));

    messageOutStream_->stream(std::move(queueElementIter->first), std::move(outStreamPromise));
  }

  void Messenger::outStreamMessageHandler(ChannelSendQueue::iterator queueElement) {
    queueElement->second->resolve();
    channelSendPromiseQueue_.erase(queueElement);

    if (!channelSendPromiseQueue_.empty()) {
      this->doSend();
    }
  }

  void Messenger::rejectReceivePromiseQueue(const error::Error &e) {
    while (!channelReceivePromiseQueue_.empty()) {
      channelReceivePromiseQueue_.pop()->reject(e);
    }
  }

  void Messenger::rejectSendPromiseQueue(const error::Error &e) {
    while (!channelSendPromiseQueue_.empty()) {
      auto queueElement(std::move(channelSendPromiseQueue_.front()));
      channelSendPromiseQueue_.pop_front();
      queueElement.second->reject(e);
    }
  }

  void Messenger::stop() {
    // Stop receive operations and reject pending promises
    receiveStrand_.dispatch([this, self = this->shared_from_this()]() {
      channelReceiveMessageQueue_.clear();
      // Reject all pending receive promises to prevent callbacks after stop
      this->rejectReceivePromiseQueue(error::Error(error::ErrorCode::OPERATION_ABORTED));
    });
    
    // Stop send operations and reject pending promises
    sendStrand_.dispatch([this, self = this->shared_from_this()]() {
      // Reject all pending send promises to prevent callbacks after stop
      this->rejectSendPromiseQueue(error::Error(error::ErrorCode::OPERATION_ABORTED));
    });
  }

}

