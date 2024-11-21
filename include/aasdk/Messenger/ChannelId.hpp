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

#include <string>

/*
 * TODO: Switch to Dynamic ChannelId's - P5
 * In AA, Channel Id's are dynamic. We use ChannelId here for a static implementation, which, while acceptable,
 * may cause more channels to be open than needs to be. OpenAuto should register a ChannelId and ChannelName
 * (for debugging) with AASDK.
 */

namespace aasdk::messenger {
  enum class ChannelId {
    CONTROL,
    SENSOR,
    MEDIA_SINK,
    MEDIA_SINK_VIDEO,
    MEDIA_SINK_MEDIA_AUDIO,
    MEDIA_SINK_GUIDANCE_AUDIO,
    MEDIA_SINK_SYSTEM_AUDIO,
    MEDIA_SINK_TELEPHONY_AUDIO,
    INPUT_SOURCE,
    MEDIA_SOURCE_MICROPHONE,
    BLUETOOTH,
    RADIO,
    NAVIGATION_STATUS,
    MEDIA_PLAYBACK_STATUS,
    PHONE_STATUS,
    MEDIA_BROWSER,
    VENDOR_EXTENSION,
    GENERIC_NOTIFICATION,
    WIFI_PROJECTION,
    NONE = 255
  };

  std::string channelIdToString(ChannelId channelId);

}

