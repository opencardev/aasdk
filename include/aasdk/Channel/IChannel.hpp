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
   * @interface IChannel
   * @brief Abstraction for a single logical communication channel in Android Auto protocol.
   *
   * **Purpose:**
   * Each IChannel represents a dedicated communication path for a specific service or subsystem.
   * For example, there's a Control channel for handshake, a Media channel for Spotify playback,
   * a Navigation channel for Maps, etc.
   *
   * **Design Pattern:**
   * - Factory pattern: Channels are typically created by a ChannelManager or similar coordinator
   * - Adapter pattern: IChannel adapts low-level Messenger to high-level service APIs
   * - Promise-based async: send() returns immediately; completion via Promise
   *
   * **Channel Responsibilities:**
   * 1. Identify itself (channel ID, name, purpose)
   * 2. Send messages to the phone on this channel
   * 3. Receive incoming messages from Messenger and deliver to listeners
   *
   * **Channel Registry (Typical):**
   * ```
   * Channel ID    Purpose                           Service
   * 0             Control & Handshake               Auth, version negotiation
   * 1             Bluetooth                         Phone calls, pairing
   * 2             Media                             Spotify, YouTube Music, Podcasts
   * 3             Navigation                        Google Maps, Waze, Here Maps
   * 4             Messages                          SMS, WhatsApp, Messenger
   * 5             Contacts                          Phone contacts, call history
   * 6             HVAC                              Climate control, seat warmers
   * 7             Reserved/Custom                   OEM extensions
   * ```
   *
   * **Typical Lifecycle:**
   * ```
   * 1. Messenger demultiplexes incoming protocol messages by channel ID
   * 2. Each IChannel implementation receives relevant messages
   * 3. Channel parses message, performs business logic (e.g., start media playback)
   * 4. Channel sends response back via send() method
   * 5. Promise resolves when response is queued for transmission
   * 6. Messenger multiplexes response back onto transport (USB/TCP)
   * 7. Phone receives response and updates UI/state
   * ```
   *
   * **Message Flow Example (Media Channel Playing a Song):**
   * ```
   * Time    Source → Dest         Message                     Channel Method
   * T+0ms   Phone → Headunit      [PLAY] Spotify track 42     [Messenger demux]
   * T+10ms  Messenger → Media     deliver({id:42, artist})   receive(msg)
   * T+20ms  Media → Backend       startPlayback(42)           [business logic]
   * T+100ms Backend → Media       [OK] now playing            [state update]
   * T+110ms Media → Messenger     send({status: PLAYING})     send(msg, p)
   * T+120ms Messenger → USB/TCP   [PLAYING response frame]   [multiplexed]
   * T+200ms USB/TCP → Phone       [response arrives]          [phone updates]
   * T+210ms p->resolve()          Transmission complete       Promise resolved
   * ```
   *
   * **Thread Safety:**
   * - send() and getId() can be called from any thread
   * - Typically, message delivery (from Messenger) occurs on a strand, so listeners
   *   should be reentrant or post work to their own strand
   *
   * **Common Implementations:**
   * - ControlChannel: AOAP handshake, device info exchange
   * - BluetoothChannel: Phone state, incoming calls
   * - MediaChannel: Playback, metadata, queue operations
   * - NavigationChannel: Route updates, turn-by-turn guidance
   *
   * **Usage Example (Headunit Service):**
   * ```cpp
   * // Get or create a Media channel
   * auto mediaChannel = channelManager->getChannel(ChannelId::MEDIA);
   *
   * // Send a command to the phone (e.g., play track 42)
   * auto playCmd = std::make_shared<Message>(
   *     mediaChannel->getId(), 
   *     MessageType::PLAY, 
   *     trackData);
   * auto sendPromise = std::make_shared<SendPromise>();
   * sendPromise->onResolve([]() {
   *     LOG(INFO) << "Play command sent to phone";
   * });
   * mediaChannel->send(playCmd, sendPromise);
   *
   * // Receive callback (async)
   * mediaChannel->onMessage([](const Message& msg) {
   *     if (msg.type == MessageType::METADATA) {
   *         LOG(INFO) << "Now playing: " << msg.metadata.title;
   *     }
   * });
   * ```
   *
   * @see Messenger for multi-channel message routing
   * @see ChannelId for channel enumeration
   * @see Message for the message format
   */
  class IChannel {
  public:
    IChannel() = default;

    virtual ~IChannel() = default;

    /**
     * @brief Get the unique channel identifier.
     *
     * **Returns:**
     * A ChannelId enum value (0–7) that uniquely identifies this channel within the
     * Android Auto protocol. Must be consistent across the lifetime of the channel.
     *
     * **Example:**
     * ```cpp
     * auto id = mediaChannel->getId();
     * assert(id == ChannelId::MEDIA);  // for a MediaChannel instance
     * ```
     *
     * @return ChannelId for this channel (immutable)
     */
    virtual messenger::ChannelId getId() const = 0;

    /**
     * @brief Send a message on this channel to the Android device.
     *
     * **Behaviour:**
     * - Asynchronous: returns immediately; completion signalled via Promise
     * - Message is enqueued in the Messenger's multiplexed send queue
     * - Promise resolves when message is successfully queued (or transmitted, depending
     *   on implementation backpressure strategy)
     * - Multiple send() calls stack; Messenger ensures FIFO serialisation
     *
     * **Message Content:**
     * The message includes:
     * - Channel ID: must match getId()
     * - Message type: service-specific (PLAY, PAUSE, METADATA, etc.)
     * - Payload: binary protobuf or raw data
     *
     * **Promise Contract:**
     * - resolve(): Message queued for transmission; caller may reuse/deallocate message data
     * - reject(Error):
     *   - CHANNEL_CLOSED: Channel was stopped or disconnected
     *   - SEND_QUEUE_FULL: Too many pending messages (backpressure)
     *   - TRANSPORT_ERROR: Underlying transport failure
     *
     * **Example: Media Channel Sending Metadata**
     * ```cpp
     * auto metadata = std::make_shared<Message>(
     *     ChannelId::MEDIA,
     *     Type::METADATA,
     *     serializeMetadata("Bohemian Rhapsody", "Queen", 354));
     *
     * auto promise = std::make_shared<SendPromise>();
     * promise->onResolve([]() {
     *     LOG(INFO) << "Metadata transmitted";
     * });
     * promise->onReject([](const Error& err) {
     *     LOG(ERROR) << "Failed to send metadata: " << err.message();
     * });
     *
     * mediaChannel->send(metadata, promise);
     * ```
     *
     * **Example: Navigation Channel Sending Route Update**
     * ```cpp
     * auto routeUpdate = std::make_shared<Message>(
     *     ChannelId::NAVIGATION,
     *     Type::ROUTE_UPDATE,
     *     serializeRoute(currentRoute));
     *
     * auto promise = std::make_shared<SendPromise>();
     * navigationChannel->send(routeUpdate, promise);
     * ```
     *
     * @param [in] message Message to send (includes channel ID and payload)
     * @param [in] promise Promise to resolve when send completes or error occurs
     *
     * @see getId() for channel identification
     * @see Messenger::enqueueSend() for implementation
     */
    virtual void send(messenger::Message::Pointer message, SendPromise::Pointer promise) = 0;

  };
}
