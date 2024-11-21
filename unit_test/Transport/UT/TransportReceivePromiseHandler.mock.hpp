#pragma once

#include <gmock/gmock.h>
#include <aasdk/Common/Data.hpp>
#include <aasdk/Error/Error.hpp>


namespace aasdk
{
namespace transport
{
namespace ut
{

class TransportReceivePromiseHandlerMock
{
public:
    MOCK_METHOD1(onResolve, void(common::Data));
    MOCK_METHOD1(onReject, void(const error::Error& e));
};

}
}
}
