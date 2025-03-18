#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IAccessoryModeQueryFactory.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class AccessoryModeQueryFactoryMock: public IAccessoryModeQueryFactory
{
public:
    MOCK_METHOD2(createQuery, IAccessoryModeQuery::Pointer(AccessoryModeQueryType queryType, IUSBEndpoint::Pointer usbEndpoint));
};

}
}
}
