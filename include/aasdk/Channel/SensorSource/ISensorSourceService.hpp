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
#include <aap_protobuf/service/sensorsource/message/SensorStartResponseMessage.pb.h>
#include <aap_protobuf/service/sensorsource/message/SensorBatch.pb.h>
#include <aap_protobuf/service/control/message/ChannelOpenResponse.pb.h>
#include "ISensorSourceServiceEventHandler.hpp"

namespace aasdk::channel::sensorsource {

  class ISensorSourceService : public virtual IChannel {
  public:
    typedef std::shared_ptr<ISensorSourceService> Pointer;

    ISensorSourceService() = default;

    virtual ~ISensorSourceService() = default;

    virtual void receive(ISensorSourceServiceEventHandler::Pointer eventHandler) = 0;

    virtual void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) = 0;

    virtual void sendSensorEventIndication(const aap_protobuf::service::sensorsource::message::SensorBatch &indication,
                                           SendPromise::Pointer promise) = 0;

    virtual void
    sendSensorStartResponse(const aap_protobuf::service::sensorsource::message::SensorStartResponseMessage &response,
                            SendPromise::Pointer promise) = 0;
  };

}


