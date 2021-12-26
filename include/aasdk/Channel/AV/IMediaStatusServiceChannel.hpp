/*
*  This file is part of aasdk library project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <memory>
#include <aasdk_proto/ChannelOpenResponseMessage.pb.h>
#include <aasdk/Messenger/ChannelId.hpp>
#include <aasdk/Channel/Promise.hpp>
#include <aasdk/Channel/AV/IMediaStatusServiceChannelEventHandler.hpp>


namespace aasdk
{
namespace channel
{
namespace av
{

class IMediaStatusServiceChannel
{
public:
    typedef std::shared_ptr<IMediaStatusServiceChannel> Pointer;

    IMediaStatusServiceChannel() = default;
    virtual ~IMediaStatusServiceChannel() = default;

    virtual void receive(IMediaStatusServiceChannelEventHandler::Pointer eventHandler) = 0;
    virtual void sendChannelOpenResponse(const proto::messages::ChannelOpenResponse& response, SendPromise::Pointer promise) = 0;
    virtual messenger::ChannelId getId() const = 0;
};

}
}
}
