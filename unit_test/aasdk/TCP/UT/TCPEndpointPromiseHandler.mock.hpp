#pragma once

#include <gmock/gmock.h>
#include <aasdk/TCP/ITCPEndpoint.hpp>


namespace aasdk
{
namespace tcp
{
namespace ut
{

class TCPEndpointPromiseHandlerMock
{
public:
    MOCK_METHOD1(onResolve, void(size_t transferredBytes));
    MOCK_METHOD1(onReject, void(const error::Error& e));
};

}
}
}
