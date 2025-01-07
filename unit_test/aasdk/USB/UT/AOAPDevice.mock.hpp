#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IAOAPDevice.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class AOAPDeviceMock: public IAOAPDevice
{
public:
    MOCK_METHOD0(getInEndpoint, IUSBEndpoint&());
    MOCK_METHOD0(getOutEndpoint, IUSBEndpoint&());
};

}
}
}
