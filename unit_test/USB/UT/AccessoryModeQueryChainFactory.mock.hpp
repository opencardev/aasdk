#pragma once

#include <gmock/gmock.h>
#include <aasdk/USB/IAccessoryModeQueryChainFactory.hpp>


namespace aasdk
{
namespace usb
{
namespace ut
{

class AccessoryModeQueryChainFactoryMock: public IAccessoryModeQueryChainFactory
{
public:
    MOCK_METHOD0(create, IAccessoryModeQueryChain::Pointer());
};

}
}
}
