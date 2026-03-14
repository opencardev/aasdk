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
// along with aasdk. If not, see <http://www.gnu.org/licenses/>.

#include <aasdk/Messenger/MessageInStream.hpp>
#include <aasdk/Messenger/MessageId.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/Common/Log.hpp>
#include <aasdk/Common/ModernLogger.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>


namespace aasdk::messenger {

  namespace {

    struct MessageTraceConfig {
      bool enabled{false};
      bool videoOnly{true};
      int sampleEvery{1};
    };

    static auto parseEnvBool(const char* value, bool defaultValue) -> bool {
      if (value == nullptr) {
        return defaultValue;
      }

      const std::string token(value);
      if (token == "1" || token == "true" || token == "TRUE" || token == "on" ||
          token == "ON" || token == "yes" || token == "YES") {
        return true;
      }

      if (token == "0" || token == "false" || token == "FALSE" || token == "off" ||
          token == "OFF" || token == "no" || token == "NO") {
        return false;
      }

      return defaultValue;
    }

    static auto parseEnvInt(const char* value, int defaultValue, int minValue, int maxValue) -> int {
      if (value == nullptr) {
        return defaultValue;
      }

      try {
        const int parsed = std::stoi(value);
        return std::max(minValue, std::min(maxValue, parsed));
      } catch (...) {
        return defaultValue;
      }
    }

    static auto readMessageTraceConfig() -> MessageTraceConfig {
      MessageTraceConfig cfg;
      cfg.enabled = parseEnvBool(std::getenv("AASDK_TRACE_MESSAGE"), false);
      cfg.videoOnly = parseEnvBool(std::getenv("AASDK_TRACE_MESSAGE_VIDEO_ONLY"), true);
      cfg.sampleEvery = parseEnvInt(std::getenv("AASDK_TRACE_MESSAGE_SAMPLE_EVERY"), 1, 1, 1000);
      return cfg;
    }

    static auto getMessageTraceConfig() -> MessageTraceConfig {
      static MessageTraceConfig cached = readMessageTraceConfig();
      static auto lastRefresh = std::chrono::steady_clock::now();

      const auto now = std::chrono::steady_clock::now();
      if (now - lastRefresh > std::chrono::seconds(1)) {
        cached = readMessageTraceConfig();
        lastRefresh = now;
      }

      return cached;
    }

    static auto shouldTraceMessage(ChannelId channelId) -> bool {
      static size_t counter = 0;
      const MessageTraceConfig cfg = getMessageTraceConfig();
      if (!cfg.enabled) {
        return false;
      }

      if (cfg.videoOnly && channelId != ChannelId::MEDIA_SINK_VIDEO) {
        return false;
      }

      ++counter;
      return (counter % static_cast<size_t>(cfg.sampleEvery)) == 0;
    }

  } // namespace

  MessageInStream::MessageInStream(boost::asio::io_service &ioService, transport::ITransport::Pointer transport,
                                   ICryptor::Pointer cryptor)
      : strand_(ioService), transport_(std::move(transport)), cryptor_(std::move(cryptor)) {

  }

  void MessageInStream::startReceive(ReceivePromise::Pointer promise) {
    AASDK_LOG_MESSENGER(debug, "startReceiveCalled()");
    strand_.dispatch([this, self = this->shared_from_this(), promise = std::move(promise)]() mutable {
      if (promise_ == nullptr) {
        promise_ = std::move(promise);
        auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
        transportPromise->then(
            [this, self = this->shared_from_this()](common::Data data) mutable {
              this->receiveFrameHeaderHandler(common::DataConstBuffer(data));
            },
            [this, self = this->shared_from_this()](const error::Error &e) mutable {
              AASDK_LOG_MESSENGER(debug, "Rejecting message.");
              promise_->reject(e);
              promise_.reset();
            });

        transport_->receive(FrameHeader::getSizeOf(), std::move(transportPromise));
      } else {
        promise_.reset();
        AASDK_LOG_MESSENGER(debug, "Already Handling Promise");
        promise->reject(error::Error(error::ErrorCode::OPERATION_IN_PROGRESS));
      }
    });
  }

  void MessageInStream::receiveFrameHeaderHandler(const common::DataConstBuffer &buffer) {
    FrameHeader frameHeader(buffer);

    AASDK_LOG(info) << "[MessageInStream] Processing Frame Header: Ch "
                     << channelIdToString(frameHeader.getChannelId()) << " Fr "
                     << frameTypeToString(frameHeader.getType())
                     << " Enc " << (frameHeader.getEncryptionType() == EncryptionType::ENCRYPTED ? "ENCRYPTED" : "PLAIN")
                     << " Msg " << (frameHeader.getMessageType() == MessageType::CONTROL ? "CONTROL" : "SPECIFIC")
                     << " Raw[0]=0x" << std::hex << static_cast<int>(buffer.cdata[0])
                     << " Raw[1]=0x" << static_cast<int>(buffer.cdata[1])
                     << std::dec;

    isValidFrame_ = true;

    auto bufferedMessage = messageBuffer_.find(frameHeader.getChannelId());
    if (bufferedMessage != messageBuffer_.end()) {
      // We have found a message...
      message_ = std::move(bufferedMessage->second);
      messageBuffer_.erase(bufferedMessage);

      AASDK_LOG_MESSENGER(debug, "Found existing message.");

      if (frameHeader.getType() == FrameType::FIRST || frameHeader.getType() == FrameType::BULK) {
        // If it's first or bulk, we need to override the message anyhow, so we will start again.
        // Need to start a new message anyhow
        message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(),
                                             frameHeader.getMessageType());
      }
    } else {
      AASDK_LOG_MESSENGER(debug, "Could not find existing message.");
      // No Message Found in Buffers and this is a middle or last frame, this an error.
      // Still need to process the frame, but we will not resolve at the end.
      message_ = std::make_shared<Message>(frameHeader.getChannelId(), frameHeader.getEncryptionType(),
                                           frameHeader.getMessageType());
      if (frameHeader.getType() == FrameType::MIDDLE || frameHeader.getType() == FrameType::LAST) {
        // This is an error
        isValidFrame_ = false;
      }
    }

    thisFrameType_ = frameHeader.getType();
    const size_t frameSize = FrameSize::getSizeOf(
        frameHeader.getType() == FrameType::FIRST ? FrameSizeType::EXTENDED : FrameSizeType::SHORT);

    auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
    transportPromise->then(
        [this, self = this->shared_from_this()](common::Data data) mutable {
          this->receiveFrameSizeHandler(common::DataConstBuffer(data));
        },
        [this, self = this->shared_from_this()](const error::Error &e) mutable {
          AASDK_LOG_MESSENGER(debug, "Rejecting message.");
          message_.reset();
          promise_->reject(e);
          promise_.reset();
        });

    transport_->receive(frameSize, std::move(transportPromise));
  }

  void MessageInStream::receiveFrameSizeHandler(const common::DataConstBuffer &buffer) {
    auto transportPromise = transport::ITransport::ReceivePromise::defer(strand_);
    transportPromise->then(
        [this, self = this->shared_from_this()](common::Data data) mutable {
          this->receiveFramePayloadHandler(common::DataConstBuffer(data));
        },
        [this, self = this->shared_from_this()](const error::Error &e) mutable {
          AASDK_LOG_MESSENGER(debug, "Rejecting message.");
          message_.reset();
          promise_->reject(e);
          promise_.reset();
        });

    FrameSize frameSize(buffer);
    frameSize_ = (int) frameSize.getFrameSize();
    AASDK_LOG(info) << "[MessageInStream] Frame size parsed: frameSize=" << frameSize.getFrameSize()
                     << " totalSize=" << frameSize.getTotalSize();
    transport_->receive(frameSize.getFrameSize(), std::move(transportPromise));
  }

  void MessageInStream::receiveFramePayloadHandler(const common::DataConstBuffer &buffer) {
    const ChannelId channelId = message_->getChannelId();
    const bool traceMessage = shouldTraceMessage(channelId);
    const size_t payloadSizeBefore = message_->getPayload().size();

    AASDK_LOG(info) << "[MessageInStream] Payload handler: ch=" << channelIdToString(message_->getChannelId())
                     << " enc=" << (message_->getEncryptionType() == EncryptionType::ENCRYPTED ? "ENCRYPTED" : "PLAIN")
                     << " msg=" << (message_->getType() == MessageType::CONTROL ? "CONTROL" : "SPECIFIC")
                     << " frameType=" << frameTypeToString(thisFrameType_)
                     << " frameSize=" << frameSize_
                     << " payloadBytes=" << buffer.size
                     << " cryptorActive=" << (cryptor_->isActive() ? "true" : "false");

    if (message_->getEncryptionType() == EncryptionType::ENCRYPTED) {
      if (!cryptor_->isActive()) {
        // Some devices deliver raw TLS records on control before cryptor activation.
        // Only synthesize ENCAPSULATED_SSL for TLS-looking records to avoid
        // misclassifying regular control payloads (e.g. version responses).
        const bool looksLikeTlsRecord =
            (buffer.size >= 2) &&
            (buffer.cdata[0] >= 0x14 && buffer.cdata[0] <= 0x17) &&
            (buffer.cdata[1] == 0x03);

        if (message_->getChannelId() == ChannelId::CONTROL && looksLikeTlsRecord) {
          message_->insertPayload(messenger::MessageId(3).getData());
        }

        message_->insertPayload(buffer);
        if (traceMessage) {
          AASDK_LOG(info) << "[MessageTrace] encrypted-pass-through"
                          << " ch=" << channelIdToString(channelId)
                          << " payloadBytes=" << buffer.size
                          << " payloadSizeAfter=" << message_->getPayload().size();
        }
      } else {
        try {
          const size_t decryptedBytes = cryptor_->decrypt(message_->getPayload(), buffer, frameSize_);
          if (traceMessage) {
            AASDK_LOG(info) << "[MessageTrace] decrypt"
                            << " ch=" << channelIdToString(channelId)
                            << " frameSize=" << frameSize_
                            << " encryptedBytes=" << buffer.size
                            << " decryptedBytes=" << decryptedBytes
                            << " payloadSizeBefore=" << payloadSizeBefore
                            << " payloadSizeAfter=" << message_->getPayload().size();
          }
        }
        catch (const error::Error &e) {
          AASDK_LOG_MESSENGER(debug, "Rejecting message.");
          message_.reset();
          promise_->reject(e);
          promise_.reset();
          return;
        }
      }
    } else {
      message_->insertPayload(buffer);
    }

    bool isResolved = false;

    // If this is the LAST frame or a BULK frame...
    if ((thisFrameType_ == FrameType::BULK || thisFrameType_ == FrameType::LAST) && isValidFrame_) {
      AASDK_LOG_MESSENGER(debug, "Resolving message.");
      if (traceMessage) {
        AASDK_LOG(info) << "[MessageTrace] resolve"
                        << " ch=" << channelIdToString(channelId)
                        << " frameType=" << frameTypeToString(thisFrameType_)
                        << " totalPayloadBytes=" << message_->getPayload().size();
      }
      promise_->resolve(std::move(message_));
      promise_.reset();
      isResolved = true;

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
          [this, self = this->shared_from_this()](const error::Error &e) mutable {
            message_.reset();
            AASDK_LOG_MESSENGER(debug, "Rejecting message.");
            promise_->reject(e);
            promise_.reset();
          });

      transport_->receive(FrameHeader::getSizeOf(), std::move(transportPromise));
    }
  }

}

