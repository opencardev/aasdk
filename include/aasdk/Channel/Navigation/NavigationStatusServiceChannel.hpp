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

#include <aasdk/Channel/ServiceChannel.hpp>
#include <aasdk/Channel/Navigation/INavigationStatusServiceChannel.hpp>


namespace aasdk
{
namespace channel
{
namespace navigation
{

class NavigationStatusServiceChannel: public INavigationStatusServiceChannel, public ServiceChannel, public std::enable_shared_from_this<NavigationStatusServiceChannel>
{
public:
    NavigationStatusServiceChannel(boost::asio::io_service::strand& strand, messenger::IMessenger::Pointer messenger);

    void receive(INavigationStatusServiceChannelEventHandler::Pointer eventHandler) override;
    messenger::ChannelId getId() const override;
    void sendChannelOpenResponse(const proto::messages::ChannelOpenResponse& response, SendPromise::Pointer promise) override;

private:
    using std::enable_shared_from_this<NavigationStatusServiceChannel>::shared_from_this;
    void messageHandler(messenger::Message::Pointer message, INavigationStatusServiceChannelEventHandler::Pointer eventHandler);
    void handleChannelOpenRequest(const common::DataConstBuffer& payload, INavigationStatusServiceChannelEventHandler::Pointer eventHandler);
    void handleStatusUpdate(const common::DataConstBuffer& payload, INavigationStatusServiceChannelEventHandler::Pointer eventHandler);
    void handleTurnEvent(const common::DataConstBuffer& payload, INavigationStatusServiceChannelEventHandler::Pointer eventHandler);
    void handleDistanceEvent(const common::DataConstBuffer& payload, INavigationStatusServiceChannelEventHandler::Pointer eventHandler);
};

}
}
}
