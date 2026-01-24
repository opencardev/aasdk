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

#include "aasdk/Channel/Promise.hpp"
#include "aasdk/Channel/IChannel.hpp"
#include "aasdk/Messenger/ChannelId.hpp"
#include <aap_protobuf/service/control/message/BatteryStatusNotification.pb.h>
#include <aap_protobuf/service/control/message/ByeByeRequest.pb.h>
#include <aap_protobuf/service/control/message/ByeByeResponse.pb.h>
#include <aap_protobuf/service/control/message/AuthResponse.pb.h>
#include <aap_protobuf/service/control/message/ServiceDiscoveryResponse.pb.h>
#include <aap_protobuf/service/control/message/AudioFocusNotification.pb.h>
#include <aap_protobuf/service/control/message/NavFocusNotification.pb.h>
#include <aap_protobuf/shared/MessageStatus.pb.h>
#include <aap_protobuf/service/control/message/PingRequest.pb.h>
#include <aap_protobuf/service/control/message/PingResponse.pb.h>
#include <aap_protobuf/service/control/message/VoiceSessionNotification.pb.h>
#include <aasdk/Common/Data.hpp>
#include <aasdk/Channel/Control/IControlServiceChannelEventHandler.hpp>


namespace aasdk::channel::control {

  class IControlServiceChannel : public virtual IChannel {
  public:
    typedef std::shared_ptr<IControlServiceChannel> Pointer;

    IControlServiceChannel() = default;

    virtual ~IControlServiceChannel() = default;

    virtual void receive(IControlServiceChannelEventHandler::Pointer eventHandler) = 0;

    virtual void sendVersionRequest(SendPromise::Pointer promise) = 0;

    virtual void sendHandshake(common::Data handshakeBuffer, SendPromise::Pointer promise) = 0;

    virtual void sendAuthComplete(const aap_protobuf::service::control::message::AuthResponse &response,
                                  SendPromise::Pointer promise) = 0;

    virtual void sendServiceDiscoveryResponse(
        const aap_protobuf::service::control::message::ServiceDiscoveryResponse &response,
        SendPromise::Pointer promise) = 0;

    virtual void
    sendAudioFocusResponse(
        const aap_protobuf::service::control::message::AudioFocusNotification &response,
        SendPromise::Pointer promise) = 0;

    virtual void
    sendShutdownRequest(const aap_protobuf::service::control::message::ByeByeRequest &request,
                        SendPromise::Pointer promise) = 0;

    virtual void
    sendShutdownResponse(const aap_protobuf::service::control::message::ByeByeResponse &response,
                         SendPromise::Pointer promise) = 0;

    virtual void
    sendNavigationFocusResponse(
        const aap_protobuf::service::control::message::NavFocusNotification &response,
        SendPromise::Pointer promise) = 0;

    virtual void
    sendVoiceSessionFocusResponse(const aap_protobuf::service::control::message::VoiceSessionNotification &response,
                                  SendPromise::Pointer promise) = 0;

    virtual void
    sendPingRequest(const aap_protobuf::service::control::message::PingRequest &request, SendPromise::Pointer promise) = 0;

    virtual void
    sendPingResponse(const aap_protobuf::service::control::message::PingResponse &response,
                     SendPromise::Pointer promise) = 0;
  };
}


