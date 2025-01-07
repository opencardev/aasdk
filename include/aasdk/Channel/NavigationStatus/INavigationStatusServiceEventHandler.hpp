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

#include <aap_protobuf/service/control/message/ChannelOpenRequest.pb.h>
#include <aap_protobuf/service/navigationstatus/message/NavigationNextTurnDistanceEvent.pb.h>
#include <aap_protobuf/service/navigationstatus/message/NavigationNextTurnEvent.pb.h>
#include <aap_protobuf/service/navigationstatus/message/NavigationStatus.pb.h>
#include "aasdk/Error/Error.hpp"

namespace aasdk::channel::navigationstatus {

  class INavigationStatusServiceEventHandler {
  public:
    typedef std::shared_ptr<INavigationStatusServiceEventHandler> Pointer;

    INavigationStatusServiceEventHandler() = default;

    virtual ~INavigationStatusServiceEventHandler() = default;

    virtual void onChannelOpenRequest(const aap_protobuf::service::control::message::ChannelOpenRequest &request) = 0;

    virtual void onChannelError(const error::Error &e) = 0;

    virtual void onStatusUpdate(const aap_protobuf::service::navigationstatus::message::NavigationStatus &navStatus) = 0;

    virtual void onTurnEvent(const aap_protobuf::service::navigationstatus::message::NavigationNextTurnEvent &turnEvent) = 0;

    virtual void
    onDistanceEvent(const aap_protobuf::service::navigationstatus::message::NavigationNextTurnDistanceEvent &distanceEvent) = 0;
  };

}


