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
 * @file AOAPDevice.cpp
 * @brief Android Open Accessory Protocol device interface implementation.
 *
 * AOAPDevice wraps a USB device in AOAP mode, providing send/receive endpoints
 * for bidirectional communication. AOAP allows Android devices to recognise
 * the head unit as a trusted accessory without USB host mode (which requires
 * special drivers on Android). Device enters AOAP mode after manufacturer/model
 * strings are sent via control transfers (see AccessoryModeStartQuery).
 *
 * Design:
 *   - Discovers device configuration and interface descriptors
 *   - Claims interface number from USB configuration
 *   - Extracts IN endpoint (device->head unit) and OUT endpoint (head unit->device)
 *   - Manages endpoint lifecycle (cancellation on destruction)
 *
 * Scenario: Device connection and AOAP mode transition
 *   - T+0ms: Android device plugged into head unit USB port
 *   - T+0ms: USB hub enumerates device, checks for AOAP support
 *   - T+50ms: Device found; AccessoryModeStartQuery sends strings (manufacturer,
 *     model, description, version, URI, serial)
 *   - T+100ms: Android device reboots into AOAP mode
 *   - T+200ms: Device re-enumerates; now in AOAP mode with new VID/PID
 *   - T+200ms: AOAPDevice created, claims AOAP interface
 *   - T+205ms: getInEndpoint() / getOutEndpoint() ready for messaging
 *
 * Endpoint assignment:
 *   - First endpoint checked: if IN (bEndpointAddress & 0x80), assign to inEndpoint_
 *   - Second endpoint (always OUT): assigned to outEndpoint_
 *   - Or reversed if first is OUT: inEndpoint_ gets second (IN), outEndpoint_ gets first
 *
 * Thread Safety: Non-thread-safe. Must be created/destroyed on single strand.
 * Endpoints (inEndpoint_, outEndpoint_) handle concurrent async operations.
 */

#include <stdexcept>
#include <tuple>
#include <aasdk/USB/USBEndpoint.hpp>
#include <aasdk/USB/AOAPDevice.hpp>
#include <aasdk/Error/Error.hpp>
#include <aasdk/Common/Log.hpp>


namespace aasdk {
  namespace usb {

    AOAPDevice::AOAPDevice(IUSBWrapper &usbWrapper, boost::asio::io_service &ioService, DeviceHandle handle,
                           uint8_t interfaceNumber, unsigned char inEndpointAddress,
                           unsigned char outEndpointAddress)
        : usbWrapper_(usbWrapper), handle_(std::move(handle)), interfaceNumber_(interfaceNumber) {
      inEndpoint_ = std::make_shared<USBEndpoint>(usbWrapper_, ioService, handle_, inEndpointAddress);
      outEndpoint_ = std::make_shared<USBEndpoint>(usbWrapper_, ioService, handle_, outEndpointAddress);
    }

    AOAPDevice::~AOAPDevice() {
      inEndpoint_->cancelTransfers();
      outEndpoint_->cancelTransfers();
      usbWrapper_.releaseInterface(handle_, interfaceNumber_);
    }

    IUSBEndpoint &AOAPDevice::getInEndpoint() {
      return *inEndpoint_;
    }

    IUSBEndpoint &AOAPDevice::getOutEndpoint() {
      return *outEndpoint_;
    }

    IAOAPDevice::Pointer
    AOAPDevice::create(IUSBWrapper &usbWrapper, boost::asio::io_service &ioService, DeviceHandle handle) {
      auto configDescriptorHandle = AOAPDevice::getConfigDescriptor(usbWrapper, handle);
      auto interface = AOAPDevice::getInterface(configDescriptorHandle);
      auto interfaceDescriptor = AOAPDevice::getInterfaceDescriptor(interface);

      if (interfaceDescriptor->bNumEndpoints < 2) {
        throw error::Error(error::ErrorCode::USB_INVALID_DEVICE_ENDPOINTS);
      }

      const auto [interfaceNumber, inEndpointAddress, outEndpointAddress] =
          AOAPDevice::extractEndpointDetails(interfaceDescriptor);

      auto result = usbWrapper.claimInterface(handle, interfaceNumber);

      // Recovery path for stale interface ownership after abrupt transport teardown.
      if (result == LIBUSB_ERROR_BUSY) {
        AASDK_LOG(warning) << "[AOAPDevice] claimInterface busy on iface="
                           << static_cast<int>(interfaceNumber)
                           << ", attempting release+retry";
        usbWrapper.releaseInterface(handle, interfaceNumber);
        result = usbWrapper.claimInterface(handle, interfaceNumber);
      }

      if (result != 0) {
        throw error::Error(error::ErrorCode::USB_CLAIM_INTERFACE, result);
      }

      return std::make_unique<AOAPDevice>(usbWrapper, ioService, std::move(handle), interfaceNumber,
                  inEndpointAddress, outEndpointAddress);
    }

    ConfigDescriptorHandle AOAPDevice::getConfigDescriptor(IUSBWrapper &usbWrapper, DeviceHandle handle) {
      ConfigDescriptorHandle configDescriptorHandle;
      libusb_device *device = usbWrapper.getDevice(handle);

      auto result = usbWrapper.getConfigDescriptor(device, 0, configDescriptorHandle);
      if (result != 0) {
        throw error::Error(error::ErrorCode::USB_OBTAIN_CONFIG_DESCRIPTOR, result);
      }

      if (configDescriptorHandle == nullptr) {
        throw error::Error(error::ErrorCode::USB_INVALID_CONFIG_DESCRIPTOR, result);
      }

      return configDescriptorHandle;
    }

    const libusb_interface *AOAPDevice::getInterface(const ConfigDescriptorHandle &configDescriptorHandle) {
      if (configDescriptorHandle->bNumInterfaces == 0) {
        throw error::Error(error::ErrorCode::USB_EMPTY_INTERFACES);
      }

      return &(configDescriptorHandle->interface[0]);
    }

    const libusb_interface_descriptor *AOAPDevice::getInterfaceDescriptor(const libusb_interface *interface) {
      if (interface->num_altsetting == 0) {
        throw error::Error(error::ErrorCode::USB_OBTAIN_INTERFACE_DESCRIPTOR);
      }

      return &interface->altsetting[0];
    }

    std::tuple<uint8_t, unsigned char, unsigned char>
    AOAPDevice::extractEndpointDetails(const libusb_interface_descriptor *interfaceDescriptor) {
      const auto firstEndpointAddress = interfaceDescriptor->endpoint[0].bEndpointAddress;
      const auto secondEndpointAddress = interfaceDescriptor->endpoint[1].bEndpointAddress;

      if ((firstEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
        return {interfaceDescriptor->bInterfaceNumber, firstEndpointAddress, secondEndpointAddress};
      }

      return {interfaceDescriptor->bInterfaceNumber, secondEndpointAddress, firstEndpointAddress};
    }

  }
}
