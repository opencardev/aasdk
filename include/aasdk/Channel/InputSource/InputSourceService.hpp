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
#include "IInputSourceService.hpp"

namespace aasdk::channel::inputsource {

  class InputSourceService
      : public IInputSourceService, public Channel, public std::enable_shared_from_this<InputSourceService> {
  public:
    InputSourceService(boost::asio::io_service::strand &strand, messenger::IMessenger::Pointer messenger);

    // Senders and Receivers

    void receive(IInputSourceServiceEventHandler::Pointer eventHandler) override;

    void
    sendChannelOpenResponse(const aap_protobuf::service::control::message::ChannelOpenResponse &response,
                            SendPromise::Pointer promise) override;

    void sendInputReport(const aap_protobuf::service::inputsource::message::InputReport &indication,
                                  SendPromise::Pointer promise) override;

    void sendKeyBindingResponse(const aap_protobuf::service::media::sink::message::KeyBindingResponse &response,
                             SendPromise::Pointer promise) override;

  private:
    using std::enable_shared_from_this<InputSourceService>::shared_from_this;

    // Internal Message Handlers

    void messageHandler(messenger::Message::Pointer message, IInputSourceServiceEventHandler::Pointer eventHandler);

    void
    handleKeyBindingRequest(const common::DataConstBuffer &payload, IInputSourceServiceEventHandler::Pointer eventHandler);

    void handleChannelOpenRequest(const common::DataConstBuffer &payload,
                                  IInputSourceServiceEventHandler::Pointer eventHandler);
  };

}


