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

#include <memory>
#include "aasdk/Channel/Promise.hpp"
#include "aasdk/Channel/IChannel.hpp"
#include "aasdk/Messenger/ChannelId.hpp"
#include <aap_protobuf/service/media/source/message/MicrophoneResponse.pb.h>
#include <aap_protobuf/service/media/source/message/MicrophoneResponse.pb.h>
#include <aap_protobuf/service/media/source/message/MicrophoneRequest.pb.h>
#include <aap_protobuf/service/media/shared/message/Setup.pb.h>
#include <aap_protobuf/service/media/shared/message/Config.pb.h>
#include <aap_protobuf/service/control/message/ChannelOpenResponse.pb.h>
#include "aasdk/Messenger/Timestamp.hpp"
#include "IMediaSourceServiceEventHandler.hpp"


namespace aasdk::channel::mediasource {

  class IMediaSourceService : public virtual IChannel {
  public:
    typedef std::shared_ptr<IMediaSourceService> Pointer;

    IMediaSourceService() = default;

    virtual ~IMediaSourceService() = default;

    virtual void receive(IMediaSourceServiceEventHandler::Pointer eventHandler) = 0;

    virtual void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) = 0;

    virtual void
    sendChannelSetupResponse(const aap_protobuf::service::media::shared::message::Config &response,
                             SendPromise::Pointer promise) = 0;

    virtual void sendMediaSourceWithTimestampIndication(messenger::Timestamp::ValueType, const common::Data &data,
                                                        SendPromise::Pointer promise) = 0;

    virtual void
    sendMicrophoneOpenResponse(const aap_protobuf::service::media::source::message::MicrophoneResponse &response,
                               SendPromise::Pointer promise) = 0;
  };

}
