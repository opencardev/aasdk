
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
 * @file MediaBrowserService.cpp
 * @brief Media browser channel for browsing device music library.
 *
 * MediaBrowserService enables the head unit to browse Android device media
 * library (local files, streaming services like Spotify, cloud storage).
 * Provides hierarchical navigation from root node through folders, playlists,
 * artists, albums, and individual songs.
 *
 * Message flow:
 *   1. Head unit requests MEDIA_GET_NODE for current browsing point
 *   2. Android responds with node tree (folders, files)
 *   3. User selects item; head unit sends MEDIA_BROWSE_INPUT (select/navigate)
 *   4. Android updates model; sends new MEDIA_*_NODE tree
 *   5. Display refreshes with updated content
 *
 * Node types:
 *   - MEDIA_ROOT_NODE: Top-level library (all music)
 *   - MEDIA_SOURCE_NODE: Music source (Spotify, local files, etc.)
 *   - MEDIA_LIST_NODE: Folder, artist, or playlist
 *   - MEDIA_SONG_NODE: Playable track (leaf node)
 *
 * Scenario: Browsing Spotify on head unit
 *   - T+0ms: User taps Music app on head unit
 *   - T+5ms: MediaBrowserService sends MEDIA_GET_NODE
 *   - T+50ms: Android responds with MEDIA_ROOT_NODE (Spotify, Local, etc.)
 *   - T+50ms: Head unit displays options: "Spotify", "Local Files"
 *   - T+100ms: User selects "Spotify"
 *   - T+100ms: Head unit sends MEDIA_BROWSE_INPUT (navigate to Spotify)
 *   - T+150ms: Android responds with MEDIA_SOURCE_NODE (Recently Played, Search, etc.)
 *   - T+150ms: Display updated; user navigates playlists
 *   - User continues browsing without re-establishing connection
 *
 * Design: Stateful browsing session maintained by Android device; head unit
 * sends incremental navigation commands; Android responds with updated tree.
 *
 * This implementation can drive multiple LCD screens for advanced infotainment.
 */

#include <aap_protobuf/service/mediabrowser/MediaBrowserMessageId.pb.h>
#include <aasdk/Channel/MediaBrowser/IMediaBrowserServiceEventHandler.hpp>
#include <aasdk/Channel/MediaBrowser/MediaBrowserService.hpp>
#include "aasdk/Common/Log.hpp"
#include <aasdk/Common/ModernLogger.hpp>

namespace aasdk::channel::mediabrowser {

  MediaBrowserService::MediaBrowserService(boost::asio::io_service::strand &strand,
                                           messenger::IMessenger::Pointer messenger)
      : Channel(strand, std::move(messenger), messenger::ChannelId::MEDIA_BROWSER) {

  }

  void MediaBrowserService::receive(IMediaBrowserServiceEventHandler::Pointer eventHandler) {

    AASDK_LOG_CHANNEL_MEDIA_BROWSER(debug, "receive()");
    auto receivePromise = messenger::ReceivePromise::defer(strand_);
    receivePromise->then(
        std::bind(&MediaBrowserService::messageHandler, this->shared_from_this(), std::placeholders::_1,
                  eventHandler),
        std::bind(&IMediaBrowserServiceEventHandler::onChannelError, eventHandler, std::placeholders::_1));

    messenger_->enqueueReceive(channelId_, std::move(receivePromise));
  }

  void MediaBrowserService::sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                                                    SendPromise::Pointer promise) {
    AASDK_LOG_CHANNEL_MEDIA_BROWSER(debug, "sendChannelOpenResponse()");
    auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                      messenger::MessageType::CONTROL));
    message->insertPayload(
        messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_RESPONSE).getData());
    message->insertPayload(response);

    this->send(std::move(message), std::move(promise));
  }

  void MediaBrowserService::messageHandler(messenger::Message::Pointer message,
                                           IMediaBrowserServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_BROWSER(debug, "messageHandler()");

    messenger::MessageId messageId(message->getPayload());
    common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

    switch (messageId.getId()) {
      case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_CHANNEL_OPEN_REQUEST:
        this->handleChannelOpenRequest(payload, std::move(eventHandler));
      case aap_protobuf::service::mediabrowser::MediaBrowserMessageId::MEDIA_ROOT_NODE:
      case aap_protobuf::service::mediabrowser::MediaBrowserMessageId::MEDIA_SOURCE_NODE:
      case aap_protobuf::service::mediabrowser::MediaBrowserMessageId::MEDIA_LIST_NODE:
      case aap_protobuf::service::mediabrowser::MediaBrowserMessageId::MEDIA_SONG_NODE:
      case aap_protobuf::service::mediabrowser::MediaBrowserMessageId::MEDIA_GET_NODE:
      case aap_protobuf::service::mediabrowser::MediaBrowserMessageId::MEDIA_BROWSE_INPUT:
      default:
        AASDK_LOG(error) << "[MediaBrowserService] Message Id not Handled: " << messageId.getId();
        this->receive(std::move(eventHandler));
        break;
    }
  }

  void MediaBrowserService::handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                                     IMediaBrowserServiceEventHandler::Pointer eventHandler) {
    AASDK_LOG_CHANNEL_MEDIA_BROWSER(debug, "handleChannelOpenRequest()");
    aap_protobuf::service::control::message::ChannelOpenRequest request;
    if (request.ParseFromArray(payload.cdata, payload.size)) {
      eventHandler->onChannelOpenRequest(request);
    } else {
      eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
    }
  }
}


