#pragma once

#include <gmock/gmock.h>
#include <aasdk/Messenger/IMessageOutStream.hpp>


namespace aasdk
{
namespace messenger
{
namespace ut
{

class MessageOutStreamMock: public IMessageOutStream
{
public:
    MOCK_METHOD2(stream, void(Message::Pointer message, SendPromise::Pointer promise));
};

}
}
}
