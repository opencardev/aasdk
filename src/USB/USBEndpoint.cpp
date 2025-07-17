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

#include <aasdk/USB/USBEndpoint.hpp>
#include <aasdk/USB/IUSBWrapper.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/Common/Log.hpp>
#include <aasdk/Common/ModernLogger.hpp>

namespace aasdk {
  namespace usb {

    USBEndpoint::USBEndpoint(IUSBWrapper &usbWrapper, boost::asio::io_service &ioService, DeviceHandle handle,
                             uint8_t endpointAddress)
        : usbWrapper_(usbWrapper), strand_(ioService), handle_(std::move(handle)), endpointAddress_(endpointAddress) {
    }

    void USBEndpoint::controlTransfer(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise) {
      if (endpointAddress_ != 0) {
        promise->reject(error::Error(error::ErrorCode::USB_INVALID_TRANSFER_METHOD));
      } else {
        auto *transfer = usbWrapper_.allocTransfer(0);
        if (transfer == nullptr) {
          promise->reject(error::Error(error::ErrorCode::USB_TRANSFER_ALLOCATION));
        } else {
          usbWrapper_.fillControlTransfer(transfer, handle_, buffer.data,
                                          reinterpret_cast<libusb_transfer_cb_fn>(&USBEndpoint::transferHandler), this,
                                          timeout);
          this->transfer(transfer, std::move(promise));
        }
      }
    }

    void USBEndpoint::interruptTransfer(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise) {
      if (endpointAddress_ == 0) {
        promise->reject(error::Error(error::ErrorCode::USB_INVALID_TRANSFER_METHOD));
      } else {
        auto *transfer = usbWrapper_.allocTransfer(0);
        if (transfer == nullptr) {
          promise->reject(error::Error(error::ErrorCode::USB_TRANSFER_ALLOCATION));
        } else {
          usbWrapper_.fillInterruptTransfer(transfer, handle_, endpointAddress_, buffer.data, buffer.size,
                                            reinterpret_cast<libusb_transfer_cb_fn>(&USBEndpoint::transferHandler),
                                            this, timeout);
          this->transfer(transfer, std::move(promise));
        }
      }
    }

    void USBEndpoint::bulkTransfer(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise) {
      if (endpointAddress_ == 0) {
        promise->reject(error::Error(error::ErrorCode::USB_INVALID_TRANSFER_METHOD));
      } else {
        auto *transfer = usbWrapper_.allocTransfer(0);
        if (transfer == nullptr) {
          AASDK_LOG(debug) << "[USBEndpoint] Rejecting Promise " << endpointAddress_ << " size " << buffer.size;
          promise->reject(error::Error(error::ErrorCode::USB_TRANSFER_ALLOCATION));
        } else {
          AASDK_LOG(debug) << "[USBEndpoint] Fill Bulk Transfer " << endpointAddress_ << " size " << buffer.size;
          usbWrapper_.fillBulkTransfer(transfer, handle_, endpointAddress_, buffer.data, buffer.size,
                                       reinterpret_cast<libusb_transfer_cb_fn>(&USBEndpoint::transferHandler), this,
                                       timeout);
          this->transfer(transfer, std::move(promise));
        }
      }
    }

    void USBEndpoint::transfer(libusb_transfer *transfer, Promise::Pointer promise) {
      strand_.dispatch([this, self = this->shared_from_this(), transfer, promise = std::move(promise)]() mutable {
        auto submitResult = usbWrapper_.submitTransfer(transfer);

        if (submitResult == libusb_error::LIBUSB_SUCCESS) {
          // guarantee that endpoint will live until all transfers are finished
          if (self_ == nullptr) {
            self_ = std::move(self);
          }

          transfers_.insert(std::make_pair(transfer, std::move(promise)));
        } else {
          AASDK_LOG(debug) << "[USBEndpoint] USB Failure " << submitResult;
          promise->reject(error::Error(error::ErrorCode::USB_TRANSFER, submitResult));
          usbWrapper_.freeTransfer(transfer);
        }
      });
    }

    uint8_t USBEndpoint::getAddress() {
      return endpointAddress_;
    }

    void USBEndpoint::cancelTransfers() {
      strand_.dispatch([this, self = this->shared_from_this()]() mutable {
        for (const auto &transfer: transfers_) {
          usbWrapper_.cancelTransfer(transfer.first);
        }
      });
    }

    DeviceHandle USBEndpoint::getDeviceHandle() const {
      return handle_;
    }

    void USBEndpoint::transferHandler(libusb_transfer *transfer) {
      AASDK_LOG_USB(debug, "transferHandler()");
      auto self = reinterpret_cast<USBEndpoint *>(transfer->user_data)->shared_from_this();

      self->strand_.dispatch([self, transfer]() mutable {
        if (self->transfers_.count(transfer) == 0) {
          AASDK_LOG_USB(debug, "No more transfers.");
          return;
        }

        auto promise(std::move(self->transfers_.at(transfer)));

        if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
          AASDK_LOG_USB(debug, "Transfer Complete.");
          promise->resolve(transfer->actual_length);
        } else {
          AASDK_LOG_USB(debug, "Transfer Cancelled.");
          auto error = transfer->status ==
              LIBUSB_TRANSFER_CANCELLED ? error::Error(error::ErrorCode::OPERATION_ABORTED)
                                                                     : error::Error(error::ErrorCode::USB_TRANSFER,
                                                                                    transfer->status);
          promise->reject(error);
        }

        self->usbWrapper_.freeTransfer(transfer);
        self->transfers_.erase(transfer);

        if (self->transfers_.empty()) {
          self->self_.reset();
        }
      });
    }

  }
}
