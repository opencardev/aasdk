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

#include <aap_protobuf/service/bluetooth/message/BluetoothPairingRequest.pb.h>
#include <aap_protobuf/service/bluetooth/message/BluetoothAuthenticationData.pb.h>
#include <aap_protobuf/service/bluetooth/message/BluetoothAuthenticationResult.pb.h>
#include <aap_protobuf/service/control/message/ChannelOpenRequest.pb.h>
#include "aasdk/Error/Error.hpp"

namespace aasdk::channel::bluetooth {

  class IBluetoothServiceEventHandler {
  public:
    typedef std::shared_ptr<IBluetoothServiceEventHandler> Pointer;

    IBluetoothServiceEventHandler() = default;

    virtual ~IBluetoothServiceEventHandler() = default;

    virtual void onChannelOpenRequest(const aap_protobuf::service::control::message::ChannelOpenRequest &request) = 0;

    virtual void
    onBluetoothPairingRequest(const aap_protobuf::service::bluetooth::message::BluetoothPairingRequest &request) = 0;

    virtual void
    onBluetoothAuthenticationResult(const aap_protobuf::service::bluetooth::message::BluetoothAuthenticationResult &request) = 0;

    virtual void onChannelError(const error::Error &e) = 0;
  };

}


