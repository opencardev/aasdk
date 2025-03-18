#pragma once

#include <gmock/gmock.h>
#include <aasdk/Error/Error.hpp>
#include <aasdk/Messenger/Message.hpp>


namespace aasdk
{
namespace messenger
{
namespace ut
{

class ReceivePromiseHandlerMock
{
public:
    MOCK_METHOD1(onResolve, void(Message::Pointer message));
    MOCK_METHOD1(onReject, void(const error::Error& e));
};

}
}
}
