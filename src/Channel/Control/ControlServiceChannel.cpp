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

#include <boost/endian/conversion.hpp>
#include <aasdk/Version.hpp>
#include <aasdk/IO/PromiseLink.hpp>
#include <aasdk/Channel/Control/ControlServiceChannel.hpp>
#include <aasdk/Channel/Control/IControlServiceChannelEventHandler.hpp>
#include <aasdk/Common/Log.hpp>


namespace aasdk {
  namespace channel {
    namespace control {

      ControlServiceChannel::ControlServiceChannel(boost::asio::io_service::strand &strand,
                                                   messenger::IMessenger::Pointer messenger)
          : Channel(strand, messenger, messenger::ChannelId::CONTROL) {

      }

      void ControlServiceChannel::sendVersionRequest(SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendVersionRequest()";

        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_VERSION_REQUEST).getData());

        common::Data versionBuffer(4, 0);
        reinterpret_cast<uint16_t &>(versionBuffer[0]) = boost::endian::native_to_big(AASDK_MAJOR);
        reinterpret_cast<uint16_t &>(versionBuffer[2]) = boost::endian::native_to_big(AASDK_MINOR);
        message->insertPayload(versionBuffer);
        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendHandshake(common::Data handshakeBuffer, SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendHandshake()";
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_ENCAPSULATED_SSL).getData());
        message->insertPayload(handshakeBuffer);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendAuthComplete(const aap_protobuf::service::control::message::AuthResponse &response,
                                                   SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendAuthComplete()";
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_AUTH_COMPLETE).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendServiceDiscoveryResponse(
          const aap_protobuf::service::control::message::ServiceDiscoveryResponse &response,
          SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendServiceDiscoveryResponse()";
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(messenger::MessageId(
            aap_protobuf::service::control::message::ControlMessageType::MESSAGE_SERVICE_DISCOVERY_RESPONSE).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void
      ControlServiceChannel::sendAudioFocusResponse(
          const aap_protobuf::service::control::message::AudioFocusNotification &response,
          SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendAudioFocusResponse()";
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_AUDIO_FOCUS_NOTIFICATION).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendShutdownRequest(
          const aap_protobuf::service::control::message::ByeByeRequest &request,
          SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendShutdownRequest()";
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BYEBYE_REQUEST).getData());
        message->insertPayload(request);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendShutdownResponse(
          const aap_protobuf::service::control::message::ByeByeResponse &response,
          SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendShutdownResponse()";
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BYEBYE_RESPONSE).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendNavigationFocusResponse(
          const aap_protobuf::service::control::message::NavFocusNotification &response,
          SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendNavigationFocusResponse()";
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::ENCRYPTED,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(
                aap_protobuf::service::control::message::ControlMessageType::MESSAGE_NAV_FOCUS_NOTIFICATION).getData());
        message->insertPayload(response);

        this->send(std::move(message), std::move(promise));
      }

      void
      ControlServiceChannel::sendVoiceSessionFocusResponse(
          const aap_protobuf::service::control::message::VoiceSessionNotification &response,
          SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendVoiceSessionFocusResponse()";
      }

      void ControlServiceChannel::sendPingResponse(const aap_protobuf::service::control::message::PingResponse &request,
                                                   SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendPingResponse()";
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_PING_RESPONSE).getData());
        message->insertPayload(request);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::sendPingRequest(const aap_protobuf::service::control::message::PingRequest &request,
                                                  SendPromise::Pointer promise) {
        AASDK_LOG(debug) << "[ControlServiceChannel] sendPingRequest()";
        auto message(std::make_shared<messenger::Message>(channelId_, messenger::EncryptionType::PLAIN,
                                                          messenger::MessageType::SPECIFIC));
        message->insertPayload(
            messenger::MessageId(aap_protobuf::service::control::message::ControlMessageType::MESSAGE_PING_REQUEST).getData());
        message->insertPayload(request);

        this->send(std::move(message), std::move(promise));
      }

      void ControlServiceChannel::receive(IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] receive()";
        auto receivePromise = messenger::ReceivePromise::defer(strand_);
        receivePromise->then(
            std::bind(&ControlServiceChannel::messageHandler, this->shared_from_this(), std::placeholders::_1,
                      eventHandler),
            std::bind(&IControlServiceChannelEventHandler::onChannelError, eventHandler, std::placeholders::_1));

        messenger_->enqueueReceive(channelId_, std::move(receivePromise));
      }

      void ControlServiceChannel::messageHandler(messenger::Message::Pointer message,
                                                 IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] messageHandler()";

        messenger::MessageId messageId(message->getPayload());
        common::DataConstBuffer payload(message->getPayload(), messageId.getSizeOf());

        AASDK_LOG(debug) << "[ControlServiceChannel] MessageId: " << messageId.getId();

        switch (messageId.getId()) {
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_VERSION_RESPONSE:
            this->handleVersionResponse(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_ENCAPSULATED_SSL:
            eventHandler->onHandshake(payload);
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_SERVICE_DISCOVERY_REQUEST:
            this->handleServiceDiscoveryRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_PING_REQUEST:
            this->handlePingRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_PING_RESPONSE:
            this->handlePingResponse(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_AUDIO_FOCUS_REQUEST:
            this->handleAudioFocusRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_NAV_FOCUS_REQUEST:
            this->handleNavigationFocusRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_VOICE_SESSION_NOTIFICATION:
            this->handleVoiceSessionRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BATTERY_STATUS_NOTIFICATION:
            this->handleBatteryStatusNotification(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BYEBYE_REQUEST:
            this->handleShutdownRequest(payload, std::move(eventHandler));
            break;
          case aap_protobuf::service::control::message::ControlMessageType::MESSAGE_BYEBYE_RESPONSE:
            this->handleShutdownResponse(payload, std::move(eventHandler));
            break;
          default:
            AASDK_LOG(error) << "[ControlServiceChannel] Message Id not Handled: " << messageId.getId();
            this->receive(std::move(eventHandler));
            break;
        }
      }

      void ControlServiceChannel::handleVersionResponse(const common::DataConstBuffer &payload,
                                                        IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handleVersionResponse()";

        const size_t elements = payload.size / sizeof(uint16_t);
        const uint16_t *versionResponse = reinterpret_cast<const uint16_t *>(payload.cdata);

        aap_protobuf::shared::MessageStatus status = static_cast<aap_protobuf::shared::MessageStatus>(boost::endian::big_to_native(
            versionResponse[2]));
        AASDK_LOG(info) << "[ControlServiceChannel] Handling Version - Major: " << versionResponse[0] << " Minor: "
                        << versionResponse[1] << "Status: " << status;

        eventHandler->onVersionResponse(versionResponse[0], versionResponse[1], status);
      }

      void ControlServiceChannel::handleServiceDiscoveryRequest(const common::DataConstBuffer &payload,
                                                                IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handleServiceDiscoveryRequest()";
        aap_protobuf::service::control::message::ServiceDiscoveryRequest request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onServiceDiscoveryRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleAudioFocusRequest(const common::DataConstBuffer &payload,
                                                          IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handleAudioFocusRequest()";
        aap_protobuf::service::control::message::AudioFocusRequest request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onAudioFocusRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleVoiceSessionRequest(const common::DataConstBuffer &payload,
                                                            IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handleVoiceSessionRequest()";
        aap_protobuf::service::control::message::VoiceSessionNotification request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onVoiceSessionRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleBatteryStatusNotification(const common::DataConstBuffer &payload,
                                                            IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handleBatteryStatusNotification()";

        aap_protobuf::service::control::message::BatteryStatusNotification request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onBatteryStatusNotification(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleShutdownRequest(const common::DataConstBuffer &payload,
                                                        IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handleShutdownRequest()";
        aap_protobuf::service::control::message::ByeByeRequest request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onByeByeRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleShutdownResponse(const common::DataConstBuffer &payload,
                                                         IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handleShutdownResponse()";
        aap_protobuf::service::control::message::ByeByeResponse response;
        if (response.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onByeByeResponse(response);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handleNavigationFocusRequest(const common::DataConstBuffer &payload,
                                                               IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handleNavigationFocusRequest()";
        aap_protobuf::service::control::message::NavFocusRequestNotification request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onNavigationFocusRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handlePingRequest(const common::DataConstBuffer &payload,
                                                    IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handlePingRequest()";
        aap_protobuf::service::control::message::PingRequest request;
        if (request.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onPingRequest(request);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

      void ControlServiceChannel::handlePingResponse(const common::DataConstBuffer &payload,
                                                     IControlServiceChannelEventHandler::Pointer eventHandler) {
        AASDK_LOG(debug) << "[ControlServiceChannel] handlePingResponse()";
        aap_protobuf::service::control::message::PingResponse response;
        if (response.ParseFromArray(payload.cdata, payload.size)) {
          eventHandler->onPingResponse(response);
        } else {
          eventHandler->onChannelError(error::Error(error::ErrorCode::PARSE_PAYLOAD));
        }
      }

    }
  }
}
