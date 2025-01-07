#pragma once

#include <gmock/gmock.h>
#include <aasdk/Error/Error.hpp>


namespace aasdk
{
namespace messenger
{
namespace ut
{

class SendPromiseHandlerMock
{
public:
    MOCK_METHOD0(onResolve, void());
    MOCK_METHOD1(onReject, void(const error::Error& e));
};

}
}
}
