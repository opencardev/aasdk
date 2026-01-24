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

#pragma once

#include <memory>
#include "aasdk/Messenger/ChannelId.hpp"
#include "aasdk/Channel/Promise.hpp"

namespace aasdk::channel {

  /**
   * @class IChannel
   * @brief Abstract interface for a logical protocol channel in Android Auto.
   *
   * IChannel represents one of the 8 multiplexed channels in the Android Auto protocol.
   * Each channel carries domain-specific traffic (e.g., Control, Media, Navigation).
   * Channels are managed by the Messenger and provide a simple send() interface.
   *
   * Channels are identified by ChannelId (0-7):
   * - Channel 0: Control (handshake, error recovery)
   * - Channel 1: Bluetooth (calls, pairing)
   * - Channel 2: Media (music playback, song info)
   * - Channel 3: Navigation (turn-by-turn directions)
   * - Channels 4-7: Additional services (messages, climate, etc.)
   *
   * Each channel carries framed messages; the Messenger multiplexes them over
   * the physical transport (USB or TCP).
   *
   * Lifecycle:
   * - Channel created and registered with Messenger
   * - send(message, promise) called to enqueue message for transmission
   * - Messenger routes message to physical transport
   * - On receipt of incoming message for this channel, application is notified
   * - Channel persists until messenger stops or connection drops
   *
   * Thread Safety: send() is async and callback-based. Implementation must be
   * thread-safe for concurrent sends on different channels.
   */
  class IChannel {
  public:
    IChannel() = default;

    virtual ~IChannel() = default;

    /**
     * @brief Get the unique channel identifier (0-7).
     *
     * @return ChannelId for this channel (fixed at registration)
     */
    virtual messenger::ChannelId getId() const = 0;

    /**
     * @brief Asynchronously send a message on this channel.
     *
     * Enqueues a message for transmission on this channel. Returns immediately;
     * promise resolves when sent or error occurs. The Messenger maintains the
     * per-channel send queue and schedules FIFO transmission.
     *
     * @param message Complete protocol message with payload (channel ID must match)
     * @param promise Promise to resolve when send completes
     *
     * Promise lifecycle:
     * - resolve(): Message successfully queued and scheduled for transmission
     * - reject(error::Error): Send failed (queue overflow, transport error)
     */
    virtual void send(messenger::Message::Pointer message, SendPromise::Pointer promise) = 0;

  };
}
