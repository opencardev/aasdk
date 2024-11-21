#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IUSBEndpoint.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class USBEndpointPromiseHandlerMock
{
public:
    MOCK_METHOD1(onResolve, void(size_t transferredBytes));
    MOCK_METHOD1(onReject, void(const error::Error& e));
};

}
}
}
