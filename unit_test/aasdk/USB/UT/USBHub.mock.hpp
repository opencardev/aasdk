#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IUSBHub.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class USBHubMock: public IUSBHub
{
public:
    MOCK_METHOD1(start, void(Promise::Pointer promise));
    MOCK_METHOD0(cancel, void());
    MOCK_METHOD1(listenHotplugEvents, void(Promise::Pointer promise));
    MOCK_METHOD0(stopListenHotplugEvents, void());
};

}
}
}
