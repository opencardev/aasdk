// This file is part of aasdk library project.
// Copyright (C) 2018 f1x.studio (Michal Szwaj)
// Copyright (C) 2024 CubeOne (Simon Dean - simon.dean@cubeone.co.uk)
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

#include <aasdk/Error/Error.hpp>
#include <aasdk/Common/Data.hpp>
#include <aap_protobuf/service/control/message/AudioFocusRequest.pb.h>
#include <aap_protobuf/service//control/message/BatteryStatusNotification.pb.h>
#include <aap_protobuf/service/control/message/ByeByeResponse.pb.h>
#include <aap_protobuf/service/control/message/NavFocusRequestNotification.pb.h>
#include <aap_protobuf/service/control/message/PingRequest.pb.h>
#include <aap_protobuf/service/control/message/PingResponse.pb.h>
#include <aap_protobuf/service/control/message/ServiceDiscoveryRequest.pb.h>
#include <aap_protobuf/service/control/message/VoiceSessionNotification.pb.h>

namespace aasdk::channel::control {

  class IControlServiceChannelEventHandler {
  public:
    typedef std::shared_ptr<IControlServiceChannelEventHandler> Pointer;

    IControlServiceChannelEventHandler() = default;

    virtual ~IControlServiceChannelEventHandler() = default;

    virtual void onVersionResponse(uint16_t majorCode, uint16_t minorCode,
                                   aap_protobuf::shared::MessageStatus status) = 0;

    virtual void onHandshake(const common::DataConstBuffer &payload) = 0;

    virtual void onServiceDiscoveryRequest(
        const aap_protobuf::service::control::message::ServiceDiscoveryRequest &request) = 0;

    virtual void
    onAudioFocusRequest(const aap_protobuf::service::control::message::AudioFocusRequest &request) = 0;

    virtual void onByeByeRequest(const aap_protobuf::service::control::message::ByeByeRequest &request) = 0;

    virtual void
    onByeByeResponse(const aap_protobuf::service::control::message::ByeByeResponse &response) = 0;

    virtual void onBatteryStatusNotification(const aap_protobuf::service::control::message::BatteryStatusNotification &notification) = 0;

    virtual void
    onNavigationFocusRequest(
        const aap_protobuf::service::control::message::NavFocusRequestNotification &request) = 0;

    virtual void
    onVoiceSessionRequest(const aap_protobuf::service::control::message::VoiceSessionNotification &request) = 0;

    virtual void onPingRequest(const aap_protobuf::service::control::message::PingRequest &request) = 0;

    virtual void onPingResponse(const aap_protobuf::service::control::message::PingResponse &response) = 0;

    virtual void onChannelError(const error::Error &e) = 0;


  };
}


