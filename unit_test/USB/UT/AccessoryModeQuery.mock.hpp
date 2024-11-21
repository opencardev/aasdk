#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IAccessoryModeQuery.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class AccessoryModeQueryMock: public IAccessoryModeQuery
{
public:
    MOCK_METHOD1(start, void(Promise::Pointer promise));
    MOCK_METHOD0(cancel, void());
};

}
}
}
