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
#include <aap_protobuf/shared/MessageStatus.pb.h>
#include <aap_protobuf/service/media/sink/message/KeyBindingResponse.pb.h>
#include <aap_protobuf/service/inputsource/message/InputReport.pb.h>
#include <aap_protobuf/service/control/message/ChannelOpenResponse.pb.h>
#include "IInputSourceServiceEventHandler.hpp"


namespace aasdk::channel::inputsource {
  class IInputSourceService : public virtual IChannel {
  public:
    typedef std::shared_ptr<IInputSourceService> Pointer;

    IInputSourceService() = default;

    virtual ~IInputSourceService() = default;

    virtual void receive(IInputSourceServiceEventHandler::Pointer eventHandler) = 0;

    virtual void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) = 0;

    virtual void sendInputReport(const aap_protobuf::service::inputsource::message::InputReport &indication,
                                          SendPromise::Pointer promise) = 0;

    virtual void sendKeyBindingResponse(const aap_protobuf::service::media::sink::message::KeyBindingResponse &response,
                                     SendPromise::Pointer promise) = 0;
  };

}


