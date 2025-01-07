#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IUSBWrapper.hpp>
#include <aasdk/Error/Error.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class AccessoryModeQueryChainPromiseHandlerMock
{
public:
    MOCK_METHOD1(onResolve, void(DeviceHandle handle));
    MOCK_METHOD1(onReject, void(const error::Error& e));
};

}
}
}
