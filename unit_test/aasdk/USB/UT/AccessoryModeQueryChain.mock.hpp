#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IAccessoryModeQueryChain.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class AccessoryModeQueryChainMock: public IAccessoryModeQueryChain
{
public:
    MOCK_METHOD2(start, void(DeviceHandle handle, Promise::Pointer promise));
    MOCK_METHOD0(cancel, void());
};

}
}
}
