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

#include <aasdk/Transport/USBTransport.hpp>
#include <aasdk/Common/Log.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>


namespace aasdk {
  namespace transport {

    USBTransport::USBTransport(std::shared_ptr<boost::asio::io_service> ioService, usb::IAOAPDevice::Pointer aoapDevice)
        : Transport(std::move(ioService)), aoapDevice_(std::move(aoapDevice)) {}

    void USBTransport::enqueueReceive(common::DataBuffer buffer) {
      const auto inEndpoint = aoapDevice_->getInEndpoint().getAddress();
      AASDK_LOG(debug) << "[USBTransport] enqueueReceive endpoint=0x" << std::hex
                      << static_cast<int>(inEndpoint) << std::dec
                      << " requestedBytes=" << buffer.size;

      auto usbEndpointPromise = usb::IUSBEndpoint::Promise::defer(receiveStrand_);
      usbEndpointPromise->then([this, self = this->shared_from_this(), inEndpoint](auto bytesTransferred) {
                                 AASDK_LOG(debug) << "[USBTransport] receiveComplete endpoint=0x"
                                                 << std::hex << static_cast<int>(inEndpoint)
                                                 << std::dec << " bytesTransferred=" << bytesTransferred;
                                 receiveNoDeviceRetryCount_ = 0;
                                 receiveInterruptedRetryCount_ = 0;
                                 this->receiveHandler(bytesTransferred);
                               },
                               [this, self = this->shared_from_this(), inEndpoint](auto e) {
                                 AASDK_LOG(warning) << "[USBTransport] receiveError endpoint=0x"
                                                    << std::hex << static_cast<int>(inEndpoint)
                                                    << std::dec << " code=" << static_cast<int>(e.getCode())
                                                    << " native=" << e.getNativeCode()
                                                    << " what=" << e.what();

                                 // Bounded recovery for LIBUSB_ERROR_NO_DEVICE (native=5) on the IN
                                 // endpoint: Android briefly resets the USB link after AOAP handshake.
                                 // Re-arm the bulk transfer after a short delay instead of cascading
                                 // the error to all channels immediately.
                                 static constexpr uint32_t kLibusbNoDevice = 5u;
                                 // Bounded recovery for LIBUSB_ERROR_INTERRUPTED (native=-4 / 4294967292)
                                 // on the IN endpoint: Linux EINTR from signal; data still in USB buffer.
                                 // Retry immediately rather than disrupting receive queue.
                                 static constexpr uint32_t kLibusbInterrupted = 4294967292u;

                                 if (e.getNativeCode() == kLibusbNoDevice &&
                                     receiveNoDeviceRetryCount_ < cReceiveNoDeviceRetryMax) {
                                   receiveNoDeviceRetryCount_++;
                                   AASDK_LOG(warning)
                                       << "[USBTransport] receiveError native=5 transient recovery "
                                       << receiveNoDeviceRetryCount_ << "/" << cReceiveNoDeviceRetryMax
                                       << " delayMs=" << cReceiveNoDeviceRetryDelayMs;
                                   this->scheduleReceiveRetry(cReceiveNoDeviceRetryDelayMs);
                                   return;
                                 }

                                 if (e.getNativeCode() == kLibusbInterrupted &&
                                     receiveInterruptedRetryCount_ < cReceiveInterruptedRetryMax) {
                                   receiveInterruptedRetryCount_++;
                                   AASDK_LOG(debug)
                                       << "[USBTransport] receiveError native=-4 interrupted retry "
                                       << receiveInterruptedRetryCount_ << "/" << cReceiveInterruptedRetryMax
                                       << " delayMs=" << cReceiveInterruptedRetryDelayMs;
                                   this->scheduleReceiveRetry(cReceiveInterruptedRetryDelayMs);
                                   return;
                                 }

                                 receiveNoDeviceRetryCount_ = 0;
                                 receiveInterruptedRetryCount_ = 0;
                                 this->rejectReceivePromises(e);
                               });

      aoapDevice_->getInEndpoint().bulkTransfer(buffer, cReceiveTimeoutMs, std::move(usbEndpointPromise));
    }

    void USBTransport::scheduleReceiveRetry(uint32_t delayMs) {
      auto timer = std::make_shared<boost::asio::steady_timer>(
          *ioServicePtr_, std::chrono::milliseconds(delayMs));
      timer->async_wait(
          receiveStrand_.wrap([this, self = this->shared_from_this(), timer](
                                  const boost::system::error_code& timerError) {
            if (timerError) {
              return;
            }
            if (!receiveQueue_.empty()) {
              AASDK_LOG(warning) << "[USBTransport] receiveRetry re-arming IN endpoint";
              // Discard the uncommitted fill() slot from the failed transfer.
              // Without this, each retry would append another 16 KB zero-chunk to
              // the DataSink; distributeReceivedData() would later consume those
              // zeros as real protocol frames, corrupting the AA session before
              // any channel can open.  The self-healing fill() in DataSink would
              // catch this even without an explicit rollback(), but we call it
              // here explicitly to make the retry semantics unambiguous.
              receivedDataSink_.rollback();
              auto buf = receivedDataSink_.fill();
              this->enqueueReceive(buf);
            }
          }));
    }

    void USBTransport::enqueueSend(SendQueue::iterator queueElement) {
      this->doSend(queueElement, 0);
    }

    void USBTransport::doSend(SendQueue::iterator queueElement, common::Data::size_type offset) {
      const auto outEndpoint = aoapDevice_->getOutEndpoint().getAddress();
      const auto remainingBytes = queueElement->first.size() - offset;
      AASDK_LOG(debug) << "[USBTransport] doSend endpoint=0x" << std::hex
                      << static_cast<int>(outEndpoint) << std::dec
                      << " offset=" << offset
                      << " remainingBytes=" << remainingBytes
                      << " totalMessageBytes=" << queueElement->first.size();

      auto usbEndpointPromise = usb::IUSBEndpoint::Promise::defer(sendStrand_);
      usbEndpointPromise->then(
          [this, self = this->shared_from_this(), queueElement, offset, outEndpoint](size_t bytesTransferred) mutable {
            AASDK_LOG(debug) << "[USBTransport] sendComplete endpoint=0x" << std::hex
                            << static_cast<int>(outEndpoint) << std::dec
                            << " offset=" << offset
                            << " bytesTransferred=" << bytesTransferred
                            << " totalMessageBytes=" << queueElement->first.size();
            this->sendHandler(queueElement, offset, bytesTransferred);
          },
          [this, self = this->shared_from_this(), queueElement, offset, outEndpoint](const error::Error &e) mutable {
            AASDK_LOG(warning) << "[USBTransport] sendError endpoint=0x" << std::hex
                               << static_cast<int>(outEndpoint) << std::dec
                               << " offset=" << offset
                               << " totalMessageBytes=" << queueElement->first.size()
                               << " code=" << static_cast<int>(e.getCode())
                               << " native=" << e.getNativeCode()
                               << " what=" << e.what();
            queueElement->second->reject(e);
            sendQueue_.erase(queueElement);

            if (!sendQueue_.empty()) {
              this->doSend(sendQueue_.begin(), 0);
            }
          });

      aoapDevice_->getOutEndpoint().bulkTransfer(common::DataBuffer(queueElement->first, offset), cSendTimeoutMs,
                                                 std::move(usbEndpointPromise));
    }

    void USBTransport::sendHandler(SendQueue::iterator queueElement, common::Data::size_type offset,
                                   size_t bytesTransferred) {
      if (offset + bytesTransferred < queueElement->first.size()) {
        this->doSend(queueElement, offset + bytesTransferred);
      } else {
        queueElement->second->resolve();
        sendQueue_.erase(queueElement);

        if (!sendQueue_.empty()) {
          this->doSend(sendQueue_.begin(), 0);
        }
      }
    }

    void USBTransport::stop() {
      aoapDevice_->getInEndpoint().cancelTransfers();
      aoapDevice_->getOutEndpoint().cancelTransfers();
    }

  }
}
