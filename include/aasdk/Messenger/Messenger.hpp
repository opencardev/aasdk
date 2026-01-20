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

#include <utility>
#include <boost/asio.hpp>
#include <list>
#include <aasdk/Messenger/IMessenger.hpp>
#include <aasdk/Messenger/IMessageInStream.hpp>
#include <aasdk/Messenger/IMessageOutStream.hpp>
#include <aasdk/Messenger/ChannelReceiveMessageQueue.hpp>
#include <aasdk/Messenger/ChannelReceivePromiseQueue.hpp>


namespace aasdk::messenger {

    /**
     * @class Messenger
     * @brief Multiplexes bidirectional Android Auto protocol messages across logical channels.
     *
     * **Purpose:**
     * The Messenger is the core message router in AASDK. It:
     * 1. Receives raw protocol frames from an IMessageInStream (USB or TCP)
     * 2. Demultiplexes frames by channel ID into per-channel receive queues
     * 3. Enqueues outgoing channel messages into a send queue
     * 4. Multiplexes queued sends back onto IMessageOutStream
     * 5. Manages asynchronous Promise resolution for each operation
     *
     * **Architecture:**
     * ```
     *   Android Device          <──USB or TCP──>      Messenger                Channels
     *   (phone or radio)                      InStream (deserialise)      enqueueReceive()
     *        │                                   │                             │
     *        ├─ Frame (Ch1, data)  ──────>  [receiveStrand]  ────────>    Channel0
     *        ├─ Frame (Ch2, data)  ──────>  channelReceiveMessageQueue_   Channel1
     *        └─ Frame (Ch3, data)  ──────>  [sendStrand] (multiplexed)   Channel2
     *                               │           │                             │
     *     enqueueSend() ────────>  [sendStrand] ────────>  OutStream       USB/TCP
     *                               doSend()    (serialise & send frames)
     * ```
     *
     * **Channel Multiplexing:**
     * Android Auto protocol multiplexes up to 8 channels (0–7) over a single physical transport:
     * - Channel 0: Control (handshake, version negotiation)
     * - Channel 1: Bluetooth (phone calls, pairing)
     * - Channel 2: Media (Spotify, YouTube Music)
     * - Channel 3: Navigation (Google Maps, Waze)
     * - Channel 4: Messages & Calling
     * - Channel 5: Phone metadata (contacts, call history)
     * - Channel 6: HVAC (climate, infotainment control)
     * - Channel 7: Reserved
     *
     * Each channel may have many concurrent sends/receives; Messenger serialises them.
     *
     * **Queueing Strategy:**
     * ```
     * Receive Side (async, in order):
     *   Frame arrives  ──>  parseChannelId()  ──>  Find/create queue[channelId]
     *        │                                                │
     *        └────────>  channelReceiveMessageQueue_  ──>  Promise1, Promise2, ...
     *
     * Send Side (async, FIFO per channel):
     *   enqueueSend(msg1, p1)  ──>  channelSendPromiseQueue_  ──>  doSend()  ──>  OutStream
     *   enqueueSend(msg2, p2)  ──>  [batch in doSend loop]  ──>  Frame1
     *                                                            Frame2
     * ```
     *
     * **Thread Safety:**
     * - receiveStrand_: Serialises all receive operations and incoming frame handling
     * - sendStrand_: Serialises all send operations and outgoing frame serialisation
     * - Safe to call enqueueReceive()/enqueueSend()/stop() from any thread
     *
     * **Scenario: Concurrent Multi-Channel Message Flow**
     * ```
     * Time    Event                             Receive Queue   Send Queue    Transport
     * T+0ms   [Phone sends Auth on Ch0]         [Auth pending]  []            ─
     * T+50ms  enqueueReceive(Ch0, p1) called   matches Auth    []            ─
     * T+60ms  p1->resolve(Auth)                []              []            ─
     * T+100ms [Phone sends Media msg on Ch2]   [Media pending] []            ─
     * T+110ms enqueueReceive(Ch2, p2) called   matches Media   []            ─
     * T+120ms p2->resolve(Media)               []              []            ─
     * T+150ms enqueueSend(Response on Ch0)     []              [Response]    ─
     * T+160ms doSend() serialises Response     []              []            Frame1 sent
     * T+200ms [Phone sends App list on Ch3]    [AppList pend]  []            ─
     * T+210ms enqueueReceive(Ch3, p3) called   matches AppList []            ─
     * T+220ms p3->resolve(AppList)             []              []            ─
     * T+300ms [Concurrent] App switches        [Announce pend] [Config req]  ─
     * T+310ms multiple channel messages                                      Frame2, Frame3 sent
     * ```
     *
     * **Lifecycle: Connection → Active → Shutdown**
     * ```
     * 1. Transport established (device connected)
     * 2. Messenger created with InStream & OutStream
     * 3. enqueueReceive(Ch0, authPromise)      – wait for phone authentication
     * 4. [Phone sends auth]  → promise resolves
     * 5. enqueueSend(authResponse, okPromise) – send our response
     * 6. [Many exchanges across channels...]
     * 7. User disconnects device or error occurs
     * 8. stop() called → all pending promises rejected with MESSENGER_STOPPED
     * 9. Messenger destroyed; InStream/OutStream cleaned up
     * ```
     *
     * **Error Handling:**
     * - If InStream signals error (e.g., USB disconnect): all receive promises rejected
     * - If OutStream signals error (e.g., socket closed): all send promises rejected
     * - If a message is malformed: Error is logged, promise rejected, messenger continues
     *
     * **Usage Example:**
     * ```cpp
     * auto inStream = std::make_shared<MessageInStream>(...);
     * auto outStream = std::make_shared<MessageOutStream>(...);
     * auto messenger = std::make_shared<Messenger>(io_service, inStream, outStream);
     *
     * // Receive on Control channel
     * auto controlPromise = std::make_shared<ReceivePromise>();
     * controlPromise->onResolve([](const Message& msg) {
     *     LOG(INFO) << "Received on Control: " << msg.dump();
     * });
     * messenger->enqueueReceive(channel::Id::CONTROL, controlPromise);
     *
     * // Send on Media channel
     * auto mediaMsg = std::make_shared<Message>(channel::Id::MEDIA, ...);
     * auto sendPromise = std::make_shared<SendPromise>();
     * sendPromise->onResolve([]() {
     *     LOG(INFO) << "Media message sent";
     * });
     * messenger->enqueueSend(mediaMsg, sendPromise);
     * ```
     *
     * @see IMessenger for the interface contract
     * @see IMessageInStream for incoming frame source
     * @see IMessageOutStream for outgoing frame sink
     * @see channel::Id for channel enumeration
     */
    class Messenger : public IMessenger, public std::enable_shared_from_this<Messenger> {
    public:
      /**
       * @brief Construct a Messenger with input and output streams.
       *
       * **Parameters:**
       * - ioService: Boost ASIO io_service for async operations; must outlive Messenger
       * - messageInStream: Source of incoming protocol frames (typically from Transport)
       * - messageOutStream: Sink for outgoing protocol frames (typically to Transport)
       *
       * **Initialization:**
       * The constructor does not start message handling; callers must invoke IMessenger methods
       * (enqueueReceive/enqueueSend) to begin. The InStream is typically started separately to
       * begin receiving frames.
       *
       * **Thread Safety:**
       * Constructor may be called from any thread, but must complete before any other method.
       *
       * **Example:**
       * ```cpp
       * auto messenger = std::make_shared<Messenger>(io_service, inStream, outStream);
       * ```
       */
      Messenger(boost::asio::io_service &ioService, IMessageInStream::Pointer messageInStream,
                IMessageOutStream::Pointer messageOutStream);

      // Deleted copy operations (non-copyable due to strands and state)
      Messenger(const Messenger &) = delete;
      Messenger &operator=(const Messenger &) = delete;

      /**
       * @brief Enqueue a receive operation on a specific channel.
       *
       * **Behaviour:**
       * - Asynchronous: returns immediately; promise resolves when message arrives or error occurs
       * - Each call queues a receive promise for the given channel
       * - Promises are resolved in FIFO order for the channel
       * - Multiple enqueueReceive() calls on the same channel stack promises
       *
       * **Promise Lifecycle:**
       * - resolve(Message): Message received on the channel
       * - reject(Error): Channel closed, transport disconnected, or messenger stopped
       *
       * **Example: Receive Multi-Channel Messages**
       * ```cpp
       * // Receive on Control (will resolve when phone sends auth)
       * auto ctrlPromise = std::make_shared<ReceivePromise>();
       * ctrlPromise->onResolve([](const Message& msg) { /* handle */ });
       * messenger->enqueueReceive(ChannelId::CONTROL, ctrlPromise);
       *
       * // Receive on Media (independent; both can be outstanding)
       * auto mediaPromise = std::make_shared<ReceivePromise>();
       * messenger->enqueueReceive(ChannelId::MEDIA, mediaPromise);
       * ```
       *
       * @param [in] channelId Channel identifier (0–7)
       * @param [in] promise Promise to resolve with received message
       *
       * @see enqueueSend() for the corresponding send operation
       */
      void enqueueReceive(ChannelId channelId, ReceivePromise::Pointer promise) override;

      /**
       * @brief Enqueue a message to send on a channel.
       *
       * **Behaviour:**
       * - Asynchronous: returns immediately; promise resolves when sent or error occurs
       * - Message is serialised into protocol frame(s) and queued
       * - Send is FIFO across all channels (fair scheduling)
       * - Multiple enqueueSend() calls stack in a send queue
       *
       * **Promise Lifecycle:**
       * - resolve(): Message successfully transmitted (or queued with backpressure < threshold)
       * - reject(Error): Send failed (transport error, messenger stopped)
       *
       * **Flow Control:**
       * The send queue has a maximum depth (implementation-specific, typically 100 frames).
       * If exceeded, new sends are rejected with SEND_QUEUE_OVERFLOW.
       *
       * **Example: Send Across Multiple Channels**
       * ```cpp
       * // Send auth response on Control
       * auto authResp = std::make_shared<Message>(ChannelId::CONTROL, ...);
       * auto authPromise = std::make_shared<SendPromise>();
       * messenger->enqueueSend(authResp, authPromise);
       *
       * // Send media info on Media (queued concurrently)
       * auto mediaInfo = std::make_shared<Message>(ChannelId::MEDIA, ...);
       * auto mediaPromise = std::make_shared<SendPromise>();
       * messenger->enqueueSend(mediaInfo, mediaPromise);
       * ```
       *
       * @param [in] message Message to send (includes channel ID and payload)
       * @param [in] promise Promise to resolve when send completes or fails
       *
       * @see enqueueReceive() for the corresponding receive operation
       */
      void enqueueSend(Message::Pointer message, SendPromise::Pointer promise) override;

      /**
       * @brief Gracefully stop the messenger and reject all pending operations.
       *
       * **Behaviour:**
       * - Stops accepting new enqueueReceive()/enqueueSend() calls
       * - All outstanding promises are rejected with MESSENGER_STOPPED
       * - Clears all queued messages
       * - Closes InStream and OutStream
       * - Safe to call multiple times (idempotent)
       *
       * **Side Effects:**
       * - Any ongoing USB/TCP transfer is aborted
       * - Undelivered messages are dropped
       * - InStream/OutStream are stopped
       *
       * **Typical Usage:**
       * ```cpp
       * // Cleanup on shutdown or error
       * if (messenger) {
       *     messenger->stop();
       *     messenger.reset();
       * }
       * ```
       *
       * @see IMessenger::stop() for interface contract
       */
      void stop() override;

    private:
      using std::enable_shared_from_this<Messenger>::shared_from_this;

      /// Send queue element: (Message, Promise) pair
      typedef std::list<std::pair<Message::Pointer, SendPromise::Pointer>> ChannelSendQueue;

      /**
       * @brief Internal: process send queue and transmit frames.
       *
       * Called internally when:
       * - A new message is enqueued via enqueueSend()
       * - An OutStream frame send completes successfully
       *
       * Pops messages from the send queue, serialises them into frames, and
       * schedules asynchronous OutStream sends. Runs on sendStrand_ to ensure
       * serialisation.
       */
      void doSend();

      /**
       * @brief Callback: handle incoming message from InStream.
       *
       * Called when InStream delivers a complete protocol message. This method:
       * 1. Extracts channel ID from the message
       * 2. Finds or creates the per-channel receive queue
       * 3. Matches queued ReceivePromises with the message
       * 4. Resolves matched promises in FIFO order
       *
       * Runs on receiveStrand_ to ensure thread safety.
       */
      void inStreamMessageHandler(Message::Pointer message);

      /**
       * @brief Callback: handle OutStream completion of a frame transmission.
       *
       * Called when OutStream successfully transmits a serialised frame. This method:
       * 1. Resolves the associated send promise
       * 2. Triggers doSend() to process the next queued message
       *
       * Runs on sendStrand_ to ensure serialisation.
       */
      void outStreamMessageHandler(ChannelSendQueue::iterator queueElement);

      /**
       * @brief Reject all queued receive promises due to an error.
       *
       * Used when InStream reports an error (transport disconnect). Clears all
       * per-channel receive queues and rejects outstanding promises.
       */
      void rejectReceivePromiseQueue(const error::Error &e);

      /**
       * @brief Reject all queued send promises due to an error.
       *
       * Used when OutStream reports an error or when stop() is called.
       * Clears the send queue and rejects outstanding promises.
       */
      void rejectSendPromiseQueue(const error::Error &e);

      /// Strand for serialising receive operations
      boost::asio::io_service::strand receiveStrand_;

      /// Strand for serialising send operations
      boost::asio::io_service::strand sendStrand_;

      /// Source of incoming protocol messages
      IMessageInStream::Pointer messageInStream_;

      /// Sink for outgoing protocol messages
      IMessageOutStream::Pointer messageOutStream_;

      /// Per-channel receive promise queues (one queue per channel)
      ChannelReceivePromiseQueue channelReceivePromiseQueue_;

      /// Per-channel receive message buffers (one buffer per channel)
      ChannelReceiveMessageQueue channelReceiveMessageQueue_;

      /// FIFO send queue: (Message, Promise) pairs waiting to be serialised and transmitted
      ChannelSendQueue channelSendPromiseQueue_;

    };

}
