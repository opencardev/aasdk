#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IUSBEndpoint.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class AccessoryModeQueryPromiseHandlerMock
{
public:
    MOCK_METHOD1(onResolve, void(IUSBEndpoint::Pointer endpoint));
    MOCK_METHOD1(onReject, void(const error::Error& e));
};

}
}
}
