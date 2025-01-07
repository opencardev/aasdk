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

#include <aap_protobuf/service/media/shared/message/Setup.pb.h>
#include <aap_protobuf/service/media/shared/message/Start.pb.h>
#include <aap_protobuf/service/media/shared/message/Stop.pb.h>
#include <aap_protobuf/service/media/source/message/Ack.pb.h>
#include <aap_protobuf/service/media/shared/message/Config.pb.h>
#include <aap_protobuf/service/media/source/message/MicrophoneRequest.pb.h>
#include <aap_protobuf/service/control/message/ChannelOpenRequest.pb.h>
#include "aasdk/Error/Error.hpp"


namespace aasdk::channel::mediasource {

  class IMediaSourceServiceEventHandler {
  public:
    typedef std::shared_ptr<IMediaSourceServiceEventHandler> Pointer;

    IMediaSourceServiceEventHandler() = default;

    virtual ~IMediaSourceServiceEventHandler() = default;

    virtual void onChannelOpenRequest(const aap_protobuf::service::control::message::ChannelOpenRequest &request) = 0;

    virtual void onMediaChannelSetupRequest(const aap_protobuf::service::media::shared::message::Setup &request) = 0;

    virtual void
    onMediaSourceOpenRequest(const aap_protobuf::service::media::source::message::MicrophoneRequest &request) = 0;

    virtual void
    onMediaChannelAckIndication(
        const aap_protobuf::service::media::source::message::Ack &indication) = 0;

    virtual void onChannelError(const error::Error &e) = 0;
  };

}


