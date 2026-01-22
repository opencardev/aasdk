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

#include <unordered_map>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <aasdk/USB/IUSBWrapper.hpp>
#include <aasdk/USB/IUSBEndpoint.hpp>


namespace aasdk::usb {

    class USBEndpoint : public IUSBEndpoint,
                        public std::enable_shared_from_this<USBEndpoint> {
    public:
      USBEndpoint(IUSBWrapper &usbWrapper, boost::asio::io_service &ioService, DeviceHandle handle,
                  uint8_t endpointAddress = 0x00);

      // Deleted copy operations
      USBEndpoint(const USBEndpoint &) = delete;
      USBEndpoint &operator=(const USBEndpoint &) = delete;

      void controlTransfer(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise) override;

      void bulkTransfer(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise) override;

      void interruptTransfer(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise) override;

      uint8_t getAddress() override;

      void cancelTransfers() override;

      DeviceHandle getDeviceHandle() const override;

    private:
      using Transfers = std::unordered_map<libusb_transfer *, Promise::Pointer>;

      using std::enable_shared_from_this<USBEndpoint>::shared_from_this;

      void transfer(libusb_transfer *transfer, Promise::Pointer promise);

      static void transferHandler(libusb_transfer *transfer);

      IUSBWrapper &usbWrapper_;
      boost::asio::io_service::strand strand_;
      DeviceHandle handle_;
      uint8_t endpointAddress_;
      Transfers transfers_;
      std::shared_ptr<USBEndpoint> self_;
    };

}
