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

#include <aasdk/Messenger/ServiceId.hpp>

namespace aasdk {
  namespace messenger {
    std::string serviceIdToString(ServiceId serviceId) {
      switch (serviceId) {
        case ServiceId::CONTROL:
          return "CONTROL";
        case ServiceId::SENSOR:
          return "SENSOR";
        case ServiceId::MEDIA_SINK:
          return "MEDIA_SINK";
        case ServiceId::INPUT_SOURCE:
          return "INPUT_SOURCE";
        case ServiceId::MEDIA_SOURCE:
          return "MEDIA_SOURCE";
        case ServiceId::BLUETOOTH:
          return "BLUETOOTH";
        case ServiceId::RADIO:
          return "RADIO";
        case ServiceId::NAVIGATION_STATUS:
          return "NAVIGATION_STATUS";
        case ServiceId::MEDIA_PLAYBACK_STATUS:
          return "MEDIA_PLAYBACK_STATUS";
        case ServiceId::PHONE_STATUS:
          return "PHONE_STATUS";
        case ServiceId::MEDIA_BROWSER:
          return "MEDIA_BROWSER";
        case ServiceId::VENDOR_EXTENSION:
          return "VENDOR_EXTENSION";
        case ServiceId::GENERIC_NOTIFICATION:
          return "GENERIC_NOTIFICATION";
        case ServiceId::WIFI_PROJECTION:
          return "WIFI_PROJECTION";
        case ServiceId::NONE:
          return "NONE";
        default:
          return "(null)";
      }
    }

  }
}
