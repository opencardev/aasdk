#pragma once

#include <gmock/gmock.h>
#include <aasdk/TCP/ITCPEndpoint.hpp>


namespace aasdk
{
namespace tcp
{
namespace ut
{

class TCPEndpointMock: public ITCPEndpoint
{
public:
    MOCK_METHOD2(send, void(common::DataConstBuffer buffer, Promise::Pointer promise));
    MOCK_METHOD2(receive, void(common::DataBuffer buffer, Promise::Pointer promise));
    MOCK_METHOD0(stop, void());
};

}
}
}
