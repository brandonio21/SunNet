/**
@file tcp_socket_connection.hpp
@brief A SocketConnection implementation for TCP.
*/
#pragma once

#include "socket_connection.hpp"

namespace SunNet {

	class TCPSocketConnection : public SocketConnection {

	public:
		TCPSocketConnection() :
			SocketConnection(AF_INET, SOCK_STREAM, IPPROTO_TCP) {}

		TCPSocketConnection(int descriptor) :
			SocketConnection(descriptor, AF_INET, SOCK_STREAM, IPPROTO_TCP) {}

	};
}