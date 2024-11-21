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

#include "aasdk/Channel/Channel.hpp"
#include "IWifiProjectionService.hpp"


namespace aasdk::channel::wifiprojection {


  class WifiProjectionService
      : public IWifiProjectionService, public Channel, public std::enable_shared_from_this<WifiProjectionService> {
  public:
    WifiProjectionService(boost::asio::io_service::strand &strand, messenger::IMessenger::Pointer messenger);

    // Senders and Receivers

    void receive(IWifiProjectionServiceEventHandler::Pointer eventHandler) override;

    void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) override;

    void
    sendWifiCredentialsResponse(const aap_protobuf::service::wifiprojection::message::WifiCredentialsResponse &response,
                                SendPromise::Pointer promise) override;

  private:
    using std::enable_shared_from_this<WifiProjectionService>::shared_from_this;

    // Internal Message Handlers

    void messageHandler(messenger::Message::Pointer message, IWifiProjectionServiceEventHandler::Pointer eventHandler);

    void handleWifiCredentialsRequest(const common::DataConstBuffer &payload,
                                      IWifiProjectionServiceEventHandler::Pointer eventHandler);

    void handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                  IWifiProjectionServiceEventHandler::Pointer eventHandler);

  };

}
