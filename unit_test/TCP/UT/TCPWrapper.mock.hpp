#pragma once

#include <gmock/gmock.h>
#include <aasdk/TCP/ITCPWrapper.hpp>


namespace aasdk
{
namespace tcp
{
namespace ut
{

class TCPWrapperMock: public ITCPWrapper
{
public:
    MOCK_METHOD3(asyncWrite, void(boost::asio::ip::tcp::socket& socket, common::DataConstBuffer buffer, Handler handler));
    MOCK_METHOD3(asyncRead, void(boost::asio::ip::tcp::socket& socket, common::DataBuffer buffer, Handler handler));
    MOCK_METHOD1(close, void(boost::asio::ip::tcp::socket& socket));
    MOCK_METHOD4(asyncConnect, void(boost::asio::ip::tcp::socket& socket, const std::string& hostname, uint16_t port, ConnectHandler handler));
    MOCK_METHOD3(connect, boost::system::error_code(boost::asio::ip::tcp::socket& socket, const std::string& hostname, uint16_t port));
};

}
}
}
