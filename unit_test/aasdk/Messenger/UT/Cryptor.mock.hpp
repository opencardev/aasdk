#pragma once

#include <gmock/gmock.h>
#include <aasdk/Messenger/ICryptor.hpp>


namespace aasdk
{
namespace messenger
{
namespace ut
{

class CryptorMock: public ICryptor
{
public:
    MOCK_METHOD0(init, void());
    MOCK_METHOD0(deinit, void());
    MOCK_METHOD0(doHandshake, bool());
    MOCK_METHOD2(encrypt, size_t(common::Data& output, const common::DataConstBuffer& buffer));
    MOCK_METHOD3(decrypt, size_t(common::Data& output, const common::DataConstBuffer& buffer, int length));
    MOCK_METHOD0(readHandshakeBuffer, common::Data());
    MOCK_METHOD1(writeHandshakeBuffer, void(const common::DataConstBuffer& buffer));
    MOCK_CONST_METHOD0(isActive, bool());
};

}
}
}
