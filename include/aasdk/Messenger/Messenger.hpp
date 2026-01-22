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
     * The Messenger is the core message router in AASDK. It receives raw protocol frames
     * from an IMessageInStream (USB or TCP), demultiplexes them by channel ID into
     * per-channel receive queues, manages send queuing, and multiplexes sends back to
     * IMessageOutStream. Manages asynchronous Promise resolution for each operation.
     *
     * Android Auto protocol multiplexes up to 8 channels (0-7) over a single transport:
     * - Channel 0: Control (handshake, version negotiation)
     * - Channel 1: Bluetooth (phone calls, pairing)
     * - Channel 2: Media (Spotify, YouTube Music)
     * - Channel 3: Navigation (Google Maps, Waze)
     * - Channels 4-7: Additional services
     *
     * Thread Safety: receiveStrand_ serialises receive operations; sendStrand_ serialises
     * send operations. Safe to call enqueueReceive/enqueueSend/stop from any thread.
     */
    class Messenger : public IMessenger, public std::enable_shared_from_this<Messenger> {
    public:
      /**
       * @brief Construct a Messenger with input and output streams.
       *
       * @param ioService Boost ASIO io_service for async operations; must outlive Messenger
       * @param messageInStream Source of incoming protocol frames (typically from Transport)
       * @param messageOutStream Sink for outgoing protocol frames (typically to Transport)
       *
       * Note: Constructor does not start message handling. Callers must invoke
       * enqueueReceive/enqueueSend methods to begin. InStream is typically started
       * separately to begin receiving frames.
       */
      Messenger(boost::asio::io_service &ioService, IMessageInStream::Pointer messageInStream,
                IMessageOutStream::Pointer messageOutStream);

      // Deleted copy operations
      Messenger(const Messenger &) = delete;
      Messenger &operator=(const Messenger &) = delete;

      /**
       * @brief Enqueue a receive operation on a specific channel.
       *
       * Asynchronous: returns immediately; promise resolves when message arrives or error occurs.
       * Multiple enqueueReceive() calls on same channel stack promises resolved in FIFO order.
       *
       * Promise lifecycle:
       * - resolve(Message): Message received on the channel
       * - reject(Error): Channel closed, transport disconnected, or messenger stopped
       *
       * @param channelId Channel identifier (0-7)
       * @param promise Promise to resolve with received message
       */
      void enqueueReceive(ChannelId channelId, ReceivePromise::Pointer promise) override;

      /**
       * @brief Enqueue a message to send on a channel.
       *
       * Asynchronous: returns immediately; promise resolves when sent or error occurs.
       * Message is serialised into protocol frame(s) and queued. Send is FIFO across
       * all channels. Multiple enqueueSend() calls stack in send queue.
       *
       * Promise lifecycle:
       * - resolve(): Message successfully transmitted (or queued with backpressure below threshold)
       * - reject(Error): Send failed (transport error, messenger stopped)
       *
       * @param message Message to send (includes channel ID and payload)
       * @param promise Promise to resolve when send completes or fails
       */
      void enqueueSend(Message::Pointer message, SendPromise::Pointer promise) override;

      /**
       * @brief Gracefully stop the messenger and reject all pending operations.
       *
       * Stops accepting new enqueueReceive/enqueueSend calls. All outstanding promises
       * are rejected with MESSENGER_STOPPED. Clears all queued messages, closes InStream
       * and OutStream. Safe to call multiple times (idempotent).
       *
       * Side effects:
       * - Any ongoing USB/TCP transfer is aborted
       * - Undelivered messages are dropped
       * - InStream/OutStream are stopped
       */
      void stop() override;

    private:
      using std::enable_shared_from_this<Messenger>::shared_from_this;

      /// Send queue element: (Message, Promise) pair
      typedef std::list<std::pair<Message::Pointer, SendPromise::Pointer>> ChannelSendQueue;

      /// Internal: process send queue and transmit frames when enqueued or ready
      void doSend();

      /// Callback: handle incoming message from InStream; demultiplexes to per-channel queues
      void inStreamMessageHandler(Message::Pointer message);

      /// Callback: handle OutStream completion of frame transmission; resolves promise
      void outStreamMessageHandler(ChannelSendQueue::iterator queueElement);

      /// Reject all queued receive promises due to error (transport disconnect)
      void rejectReceivePromiseQueue(const error::Error &e);

      /// Reject all queued send promises due to error or stop
      void rejectSendPromiseQueue(const error::Error &e);

      /// Strand for serialising receive operations and incoming frame handling
      boost::asio::io_service::strand receiveStrand_;

      /// Strand for serialising send operations and outgoing frame serialisation
      boost::asio::io_service::strand sendStrand_;

      /// Source of incoming protocol messages
      IMessageInStream::Pointer messageInStream_;

      /// Sink for outgoing protocol messages
      IMessageOutStream::Pointer messageOutStream_;

      /// Per-channel receive promise queues (one queue per channel 0-7)
      ChannelReceivePromiseQueue channelReceivePromiseQueue_;

      /// Per-channel receive message buffers (one buffer per channel 0-7)
      ChannelReceiveMessageQueue channelReceiveMessageQueue_;

      /// FIFO send queue: (Message, Promise) pairs waiting to transmit
      ChannelSendQueue channelSendPromiseQueue_;

    };

}
