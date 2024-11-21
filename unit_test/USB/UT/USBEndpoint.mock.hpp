#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IUSBEndpoint.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class USBEndpointMock: public IUSBEndpoint
{
public:
    MOCK_METHOD0(getAddress, uint8_t());
    MOCK_METHOD3(controlTransfer, void(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise));
    MOCK_METHOD3(bulkTransfer, void(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise));
    MOCK_METHOD3(interruptTransfer, void(common::DataBuffer buffer, uint32_t timeout, Promise::Pointer promise));
    MOCK_METHOD0(cancelTransfers, void());
    MOCK_CONST_METHOD0(getDeviceHandle, DeviceHandle());
};

}
}
}
