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

/**
 * @file USBEndpoint.cpp
 * @brief USB endpoint abstraction for control, interrupt, and bulk transfers.
 *
 * USBEndpoint encapsulates libusb asynchronous transfer operations for a single
 * USB endpoint. Three transfer types enabled:
 *   - Control transfers (endpoint 0): For device configuration (vendor commands)
 *   - Interrupt transfers: Small, low-latency messages (typically 8-64 bytes)
 *   - Bulk transfers: High-volume, high-latency messaging (max 512 bytes)
 *
 * AOAP uses bulk transfers exclusively (64-byte packets bidirectional for
 * frame-based messaging). Frame structure:
 *   Byte 0: Frame type (or command ID)
 *   Byte 1-2: Channel ID and encryption
 *   Byte 3-4: Message size (16-bit little-endian)
 *   Byte 5-68: Payload (63 bytes max per frame; larger messages split)
 *
 * Scenario: Sending 500-byte metadata message over USB bulk (T+0 to T+50ms)
 *   - T+0ms: Messenger calls enqueueSend(metadata_message, promise)
 *   - T+1ms: Multiplexes into 8 frames (63+63+63+63+63+63+63+50 bytes)
 *   - T+2ms: Transport calls USBEndpoint::bulkTransfer(frame1, 10000ms, p1)
 *   - T+2ms: libusb schedules USB bulk transfer on endpoint 0x81 (IN from device)
 *   - T+5ms: Frame 1 sent (1ms latency); transferHandler called
 *   - T+5ms: p1 resolves; Transport sends next frame
 *   - T+8ms: Frame 2 sent
 *   - T+11ms: Frame 3 sent
 *   - ...
 *   - T+45ms: Frame 8 (final) sent
 *   - T+50ms: All 500 bytes transmitted; original promise resolved
 *   - User sees metadata updated on display
 *
 * Error handling:
 *   - Timeout: Promise rejected after 10 seconds (typical timeout)
 *   - Device disconnected: LIBUSB_ERROR_NO_DEVICE
 *   - Stalled endpoint: LIBUSB_ERROR_PIPE (protocol error from device)
 *
 * Thread Safety: All transfers queued asynchronously; completion callbacks
 * (transferHandler) run on libusb event thread. Strand usage ensures thread-safe
 * promise resolution.
 */

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

        if (aasdk::common::ModernLogger::getInstance().isVerboseUsb()) {
          AASDK_LOG(info) << "[USBEndpoint] submitTransfer result=" << submitResult << " endpoint=" << static_cast<int>(endpointAddress_);
        }

        if (submitResult == libusb_error::LIBUSB_SUCCESS) {
          // guarantee that endpoint will live until all transfers are finished
          if (self_ == nullptr) {
            self_ = std::move(self);
          }

          transfers_.insert({transfer, std::move(promise)});
        } else {
          if (aasdk::common::ModernLogger::getInstance().isVerboseUsb()) {
            AASDK_LOG(info) << "[USBEndpoint] USB Failure submitResult=" << submitResult << " endpoint=" << static_cast<int>(endpointAddress_);
          }
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
      if (aasdk::common::ModernLogger::getInstance().isVerboseUsb()) {
        AASDK_LOG_USB(info, "transferHandler() endpoint=" << static_cast<int>(reinterpret_cast<USBEndpoint *>(transfer->user_data)->endpointAddress_));
      }
      auto self = reinterpret_cast<USBEndpoint *>(transfer->user_data)->shared_from_this();

      self->strand_.dispatch([self, transfer]() mutable {
        if (self->transfers_.count(transfer) == 0) {
          if (aasdk::common::ModernLogger::getInstance().isVerboseUsb()) {
            AASDK_LOG_USB(info, "No more transfers.");
          }
          return;
        }

        auto promise(std::move(self->transfers_.at(transfer)));

        if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
          if (aasdk::common::ModernLogger::getInstance().isVerboseUsb()) {
            AASDK_LOG_USB(info, "Transfer Complete. actual_length=" << transfer->actual_length);
          }
          promise->resolve(transfer->actual_length);
        } else {
          if (aasdk::common::ModernLogger::getInstance().isVerboseUsb()) {
            AASDK_LOG_USB(info, "Transfer Cancelled. status=" << transfer->status);
          }
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
