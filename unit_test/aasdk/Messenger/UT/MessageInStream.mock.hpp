#pragma once

#include <gmock/gmock.h>
#include <aasdk/Messenger/IMessageInStream.hpp>


namespace aasdk
{
namespace messenger
{
namespace ut
{

class MessageInStreamMock: public IMessageInStream
{
public:
    MOCK_METHOD1(startReceive, void(ReceivePromise::Pointer promise));
};

}
}
}
