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
#include <aap_protobuf/service/control/message/ChannelOpenResponse.pb.h>
#include <aap_protobuf/service/bluetooth/message/BluetoothPairingResponse.pb.h>
#include <aap_protobuf/service/bluetooth/message/BluetoothAuthenticationData.pb.h>
#include "IBluetoothServiceEventHandler.hpp"

namespace aasdk::channel::bluetooth {

  class IBluetoothService : public virtual IChannel {
  public:
    typedef std::shared_ptr<IBluetoothService> Pointer;

    IBluetoothService() = default;

    virtual ~IBluetoothService() = default;

    virtual void receive(IBluetoothServiceEventHandler::Pointer eventHandler) = 0;

    virtual void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) = 0;

    virtual void sendBluetoothAuthenticationData(
        const aap_protobuf::service::bluetooth::message::BluetoothAuthenticationData &response,
        SendPromise::Pointer promise) = 0;

    virtual void
    sendBluetoothPairingResponse(const aap_protobuf::service::bluetooth::message::BluetoothPairingResponse &response,
                                 SendPromise::Pointer promise) = 0;
  };

}


