
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
#include "IVideoMediaSinkServiceEventHandler.hpp"
#include "aasdk/Channel/Promise.hpp"
#include "aasdk/Channel/IChannel.hpp"
#include "aasdk/Messenger/ChannelId.hpp"
#include <aap_protobuf/service/media/shared/message/Setup.pb.h>
#include <aap_protobuf/service/media/shared/message/Config.pb.h>
#include <aap_protobuf/service/media/source/message/Ack.pb.h>
#include <aap_protobuf/service/media/video/message/VideoFocusNotification.pb.h>
#include <aap_protobuf/service/media/video/message/VideoFocusRequestNotification.pb.h>
#include <aap_protobuf/service/control/message/ChannelOpenResponse.pb.h>

namespace aasdk::channel::mediasink::video {

  class IVideoMediaSinkService : public virtual IChannel {
  public:
    typedef std::shared_ptr<IVideoMediaSinkService> Pointer;

    IVideoMediaSinkService() = default;

    virtual ~IVideoMediaSinkService() = default;

    virtual void receive(IVideoMediaSinkServiceEventHandler::Pointer eventHandler) = 0;

    virtual void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) = 0;

    virtual void
    sendChannelSetupResponse(const aap_protobuf::service::media::shared::message::Config &response,
                             SendPromise::Pointer promise) = 0;

    virtual void
    sendMediaAckIndication(
        const aap_protobuf::service::media::source::message::Ack &indication,
        SendPromise::Pointer promise) = 0;

    virtual void
    sendVideoFocusIndication(
        const aap_protobuf::service::media::video::message::VideoFocusNotification &indication,
        SendPromise::Pointer promise) = 0;

  };

}




