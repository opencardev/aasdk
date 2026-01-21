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

/**
 * @file Channel.cpp
 * @brief Service-specific logical channel for Android Auto protocol.
 *
 * Channel provides a service with its own message queue and promise handling,
 * enabling multiple services to coexist on the same transport without blocking
 * each other. Up to 8 logical channels (0-7) can multiplex over a single USB
 * or TCP connection, each with independent send/receive flow.
 *
 * Architecture:
 *   - Each service (Navigation, Media, Phone, etc.) owns one channel
 *   - Channels identified by ID 0-7 in protocol frame header
 *   - Messenger demultiplexes inbound frames to channel queues
 *   - Each channel has independent promise queue for receive operations
 *   - Send operations multiplexed to shared transport
 *
 * Scenario: Concurrent multi-service messaging
 *   - Channel 0 (Navigation): Continuously receives turn updates
 *   - Channel 1 (Media): Sends track change events
 *   - Channel 2 (Phone): Receives incoming call notification
 *   - T+0ms: Nav sends receive(512) for next turn
 *   - T+5ms: Media sends 200-byte metadata update
 *   - T+10ms: Phone service sends receive(256) for call state
 *   - T+20ms: Android sends 256 bytes on channel 2 (incoming call)
 *   - T+20ms: Phone's receive promise resolves immediately
 *   - T+35ms: Metadata update completes transmission (media unblocked)
 *   - T+50ms: Turn update arrives on channel 0; nav unblocked
 *   - Each service progresses at own pace without head-of-line blocking
 *
 * Thread Safety: Channels operate on ASIO strands; send() wraps in PromiseLink
 * to bridge strand contexts. Multiple threads can call send/receive safely.
 */

#include "aasdk/IO/PromiseLink.hpp"
#include "aasdk/Channel/Channel.hpp"

namespace aasdk::channel {
  Channel::Channel(boost::asio::io_service::strand &strand,
                   messenger::IMessenger::Pointer messenger,
                   messenger::ChannelId channelId)
      : strand_(strand), messenger_(std::move(messenger)), channelId_(channelId) {

  }

  messenger::ChannelId Channel::getId() const {
    return channelId_;
  }

  void Channel::send(messenger::Message::Pointer message, SendPromise::Pointer promise) {
#if BOOST_VERSION < 106600
    auto sendPromise = messenger::SendPromise::defer(strand_.get_io_service());
#else
    auto sendPromise = messenger::SendPromise::defer(strand_.context());
#endif

    io::PromiseLink<>::forward(*sendPromise, std::move(promise));
    messenger_->enqueueSend(std::move(message), std::move(sendPromise));
  }

}

