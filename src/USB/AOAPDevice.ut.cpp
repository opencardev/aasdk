/*
*  This file is part of aasdk library project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  aasdk is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  aasdk is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with aasdk. If not, see <http://www.gnu.org/licenses/>.
*/

#include <aasdk/USB/UT/USBWrapper.mock.hpp>
#include <aasdk/USB/AOAPDevice.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

TEST(AOAPDevice_UnitTest, AOAPDevice_OutEndpointFirst)
{
    USBWrapperMock usbWrapperMock;
    boost::asio::io_service ioService;
    USBWrapperMock::DummyDeviceHandle dummyDeviceHandle;
    DeviceHandle deviceHandle(reinterpret_cast<libusb_device_handle*>(&dummyDeviceHandle), [](auto*) {});

    libusb_endpoint_descriptor endpointDescriptor[2];
    endpointDescriptor[0].bEndpointAddress = LIBUSB_ENDPOINT_OUT + 4;
    endpointDescriptor[1].bEndpointAddress = LIBUSB_ENDPOINT_IN + 6;

    libusb_interface_descriptor interfaceDescriptor;
    interfaceDescriptor.bInterfaceNumber = 55;
    interfaceDescriptor.bNumEndpoints = 2;
    interfaceDescriptor.endpoint = endpointDescriptor;

    EXPECT_CALL(usbWrapperMock, releaseInterface(deviceHandle, interfaceDescriptor.bInterfaceNumber));
    AOAPDevice aoapDevice(usbWrapperMock, ioService, deviceHandle, &interfaceDescriptor);
    EXPECT_TRUE(endpointDescriptor[0].bEndpointAddress == aoapDevice.getOutEndpoint().getAddress());
    EXPECT_TRUE(endpointDescriptor[1].bEndpointAddress == aoapDevice.getInEndpoint().getAddress());
}

TEST(AOAPDevice_UnitTest, AOAPDevice_InEndpointFirst)
{
    USBWrapperMock usbWrapperMock;
    boost::asio::io_service ioService;
    USBWrapperMock::DummyDeviceHandle dummyDeviceHandle;
    DeviceHandle deviceHandle(reinterpret_cast<libusb_device_handle*>(&dummyDeviceHandle), [](auto*) {});

    libusb_endpoint_descriptor endpointDescriptor[2];
    endpointDescriptor[0].bEndpointAddress = LIBUSB_ENDPOINT_IN + 4;
    endpointDescriptor[1].bEndpointAddress = LIBUSB_ENDPOINT_OUT + 6;

    libusb_interface_descriptor interfaceDescriptor;
    interfaceDescriptor.bInterfaceNumber = 55;
    interfaceDescriptor.bNumEndpoints = 2;
    interfaceDescriptor.endpoint = endpointDescriptor;

    EXPECT_CALL(usbWrapperMock, releaseInterface(deviceHandle, interfaceDescriptor.bInterfaceNumber));
    AOAPDevice aoapDevice(usbWrapperMock, ioService, deviceHandle, &interfaceDescriptor);
    EXPECT_TRUE(endpointDescriptor[0].bEndpointAddress == aoapDevice.getInEndpoint().getAddress());
    EXPECT_TRUE(endpointDescriptor[1].bEndpointAddress == aoapDevice.getOutEndpoint().getAddress());
}

}
}
}
