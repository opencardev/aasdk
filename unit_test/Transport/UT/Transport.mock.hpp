#pragma once

#include <gmock/gmock.h>
#include <aasdk/Transport/ITransport.hpp>


namespace aasdk
{
namespace transport
{
namespace ut
{

class TransportMock: public ITransport
{
public:
    MOCK_METHOD2(receive, void(size_t size, ReceivePromise::Pointer promise));
    MOCK_METHOD2(send, void(common::Data data, SendPromise::Pointer promise));
    MOCK_METHOD0(stop, void());
};

}
}
}
