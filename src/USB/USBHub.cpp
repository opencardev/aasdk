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
 * @file USBHub.cpp
 * @brief USB device hotplug detection and Android Open Accessory Protocol negotiation.
 *
 * USBHub monitors USB bus for device connections and automatically transitions
 * compatible Android devices into AOAP (Android Open Accessory Protocol) mode.
 * AOAP allows head units to connect as "trusted accessories" without requiring
 * USB host mode capabilities on Android.
 *
 * Architecture:
 *   - Uses libusb hotplug callbacks to detect device arrival
 *   - Checks if device is Android or already in AOAP mode
 *   - If Android, sends AOAP mode strings via USB control transfers
 *   - Device reboots into AOAP mode; re-enumerates with new VID/PID
 *   - Second arrival triggers real device connection
 *
 * Scenario: User plugs Android phone into car head unit (T+0 to T+500ms)
 *   - T+0ms: User plugs USB cable into head unit port
 *   - T+10ms: USB hub detects device; hotplugEventsHandler called
 *   - T+15ms: USBHub checks device: Android (vendor ID 0x18D1)
 *   - T+20ms: QueryChain dispatched; sends manufacturer="OpenCarDev"
 *   - T+40ms: Sends model="crankshaft-mvp"
 *   - T+60ms: Sends URI="https://opencardev.org"
 *   - T+80ms: Sends AOAP mode activation command
 *   - T+100ms: Android device detects AOAP activation
 *   - T+200ms: Android resets; re-enumerates into AOAP mode
 *   - T+210ms: Device arrives again; new VID/PID (0x18D1:0x4EE2) indicates AOAP
 *   - T+220ms: AOAPDevice created; messaging ready
 *   - T+300ms: Services discovered and opened
 *   - T+500ms: User interface responsive; ready for navigation/media
 *
 * Query chain executes sequentially; Android device must respond to each
 * string query before AOAP mode activation command. If Android doesn't respond
 * (timeout), device skipped and hub continues monitoring for other devices.
 *
 * Thread Safety: All operations dispatched on strand_ for serialised access.
 * hotplugEventsHandler runs on libusb event thread; dispatches work to strand.
 * Shared self_ pointer ensures USBHub lifetime during async operations.
 */

#include <thread>
#include <aasdk/USB/IUSBWrapper.hpp>
#include <aasdk/USB/USBHub.hpp>
#include <aasdk/USB/AccessoryModeQueryChain.hpp>
#include <aasdk/Error/Error.hpp>


namespace aasdk {
  namespace usb {

    USBHub::USBHub(IUSBWrapper &usbWrapper, boost::asio::io_service &ioService,
                   IAccessoryModeQueryChainFactory &queryChainFactory)
        : usbWrapper_(usbWrapper), strand_(ioService), queryChainFactory_(queryChainFactory) {
    }

    void USBHub::start(Promise::Pointer promise) {
      strand_.dispatch([this, self = this->shared_from_this(), promise = std::move(promise)]() {
        if (hotplugPromise_ != nullptr) {
          hotplugPromise_->reject(error::Error(error::ErrorCode::OPERATION_ABORTED));
          hotplugPromise_.reset();
        }

        hotplugPromise_ = std::move(promise);

        if (self_ == nullptr) {
          self_ = this->shared_from_this();
          hotplugHandle_ = usbWrapper_.hotplugRegisterCallback(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                                                               LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY,
                                                               LIBUSB_HOTPLUG_MATCH_ANY,
                                                               LIBUSB_HOTPLUG_MATCH_ANY,
                                                               reinterpret_cast<libusb_hotplug_callback_fn>(&USBHub::hotplugEventsHandler),
                                                               reinterpret_cast<void *>(this));
        }
      });
    }

    void USBHub::cancel() {
      strand_.dispatch([this, self = this->shared_from_this()]() mutable {
        if (hotplugPromise_ != nullptr) {
          hotplugPromise_->reject(error::Error(error::ErrorCode::OPERATION_ABORTED));
          hotplugPromise_.reset();
        }

        std::for_each(queryChainQueue_.begin(), queryChainQueue_.end(),
                      std::bind(&IAccessoryModeQueryChain::cancel, std::placeholders::_1));

        if (self_ != nullptr) {
          hotplugHandle_.reset();
          self_.reset();
        }
      });
    }

    int USBHub::hotplugEventsHandler(libusb_context *usbContext, libusb_device *device, libusb_hotplug_event event,
                                     void *userData) {
      if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        auto self = reinterpret_cast<USBHub *>(userData)->shared_from_this();
        self->strand_.dispatch(std::bind(&USBHub::handleDevice, self, device));
      }

      return 0;
    }

    bool USBHub::isAOAPDevice(const libusb_device_descriptor &deviceDescriptor) const {
      return deviceDescriptor.idVendor == cGoogleVendorId &&
             (deviceDescriptor.idProduct == cAOAPId || deviceDescriptor.idProduct == cAOAPWithAdbId);
    }

    void USBHub::handleDevice(libusb_device *device) {
      if (hotplugPromise_ == nullptr) {
        return;
      }

      libusb_device_descriptor deviceDescriptor;
      if (usbWrapper_.getDeviceDescriptor(device, deviceDescriptor) != 0) {
        return;
      }

      DeviceHandle handle;
      auto openResult = usbWrapper_.open(device, handle);

      if (openResult != 0) {
        return;
      }

      if (this->isAOAPDevice(deviceDescriptor)) {
        hotplugPromise_->resolve(std::move(handle));
        hotplugPromise_.reset();
      } else {
        ////////// Workaround for VMware
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        //////////

        queryChainQueue_.emplace_back(queryChainFactory_.create());

        auto queueElementIter = std::prev(queryChainQueue_.end());
        auto queryChainPromise = IAccessoryModeQueryChain::Promise::defer(strand_);
        queryChainPromise->then([this, self = this->shared_from_this(), queueElementIter](DeviceHandle handle) mutable {
                                  queryChainQueue_.erase(queueElementIter);
                                },
                                [this, self = this->shared_from_this(), queueElementIter](
                                    const error::Error &e) mutable {
                                  queryChainQueue_.erase(queueElementIter);
                                });

        queryChainQueue_.back()->start(std::move(handle), std::move(queryChainPromise));
      }
    }

  }
}
