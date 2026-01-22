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

#include <memory>
#include <aasdk/USB/USBWrapper.hpp>
#include <aasdk/Common/Data.hpp>
#include <aasdk/IO/Promise.hpp>


namespace aasdk::usb {

    class IUSBEndpoint {
    public:
      using Pointer = std::shared_ptr<IUSBEndpoint>;
      using Promise = io::Promise<size_t>;

      IUSBEndpoint() = default;

      virtual ~IUSBEndpoint() = default;

      virtual uint8_t getAddress() = 0;

      virtual void controlTransfer(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise) = 0;

      virtual void bulkTransfer(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise) = 0;

      virtual void interruptTransfer(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise) = 0;

      virtual void cancelTransfers() = 0;

      virtual DeviceHandle getDeviceHandle() const = 0;
    };

}
