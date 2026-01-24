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

#include <aasdk/Messenger/ChannelId.hpp>

namespace aasdk::messenger {

  std::string channelIdToString(ChannelId channelId) {
    switch (channelId) {
      case ChannelId::CONTROL:
        return "CONTROL";
      case ChannelId::SENSOR:
        return "SENSOR";
      case ChannelId::MEDIA_SINK:
        return "MEDIA_SINK";
      case ChannelId::MEDIA_SINK_VIDEO:
        return "MEDIA_SINK_VIDEO";
      case ChannelId::MEDIA_SINK_MEDIA_AUDIO:
        return "MEDIA_SINK_MEDIA_AUDIO";
      case ChannelId::MEDIA_SINK_GUIDANCE_AUDIO:
        return "MEDIA_SINK_GUIDANCE_AUDIO";
      case ChannelId::MEDIA_SINK_SYSTEM_AUDIO:
        return "MEDIA_SINK_SYSTEM_AUDIO";
      case ChannelId::MEDIA_SINK_TELEPHONY_AUDIO:
        return "MEDIA_SINK_TELEPHONY_AUDIO";
      case ChannelId::INPUT_SOURCE:
        return "INPUT_SOURCE";
      case ChannelId::MEDIA_SOURCE_MICROPHONE:
        return "MEDIA_SOURCE_MICROPHONE";
      case ChannelId::BLUETOOTH:
        return "BLUETOOTH";
      case ChannelId::RADIO:
        return "RADIO";
      case ChannelId::NAVIGATION_STATUS:
        return "NAVIGATION_STATUS";
      case ChannelId::MEDIA_PLAYBACK_STATUS:
        return "MEDIA_PLAYBACK_STATUS";
      case ChannelId::PHONE_STATUS:
        return "PHONE_STATUS";
      case ChannelId::MEDIA_BROWSER:
        return "MEDIA_BROWSER";
      case ChannelId::VENDOR_EXTENSION:
        return "VENDOR_EXTENSION";
      case ChannelId::GENERIC_NOTIFICATION:
        return "GENERIC_NOTIFICATION";
      case ChannelId::WIFI_PROJECTION:
        return "WIFI_PROJECTION";
      case ChannelId::NONE:
        return "NONE";
      default:
        return "(null)";
    }
  }

}


