#include "socket_connection.hpp"

#include <string>

namespace SunNet {
	std::atomic_uint SocketConnection::open_connection_count = 0;
	std::atomic_uint SocketConnection::initializations = 0;

	SocketConnection::SocketConnection(int domain, int type, int protocol) {
		if (SocketConnection::open_connection_count++ == 0) {
			this->initialize_api();
		}

		this->address_info = std::unique_ptr<struct addrinfo_data, addrinfo_delete>(new addrinfo_data);
		std::memset(this->address_info.get(), 0, sizeof(struct addrinfo_data));

		this->address_info->info = std::make_unique<struct addrinfo>();
		std::memset(this->address_info->info.get(), 0, sizeof(struct addrinfo));

		this->address_info->info->ai_family = domain;
		this->address_info->info->ai_socktype = type;
		this->address_info->info->ai_protocol = protocol;
		this->address_info->initialized = false;

		SOCKET socket_response = open_socket(domain, type, protocol);

		if (socket_response == INVALID_SOCKET) {
			int socket_err = get_previous_error_code();

			if (socket_err == SOCKET_API_NOT_INITIALIZED) {
				this->initialize_api();
				socket_response = open_socket(domain, type, protocol);
				if (socket_response == INVALID_SOCKET) {
					throw CreateException(std::to_string(get_previous_error_code()));
				}
			}
			else {
				throw CreateException(std::to_string(get_previous_error_code()));
			}
		}

		this->socket_descriptor = socket_response;
	}

	SocketConnection::SocketConnection(SOCKET socket_fd, int domain, int type, int protocol) :
		socket_descriptor(socket_fd){

		if (SocketConnection::open_connection_count++ == 0) {
			this->initialize_api();
		}

		this->address_info = std::unique_ptr<struct addrinfo_data, addrinfo_delete>(new addrinfo_data);
		std::memset(this->address_info.get(), 0, sizeof(struct addrinfo_data));

		this->address_info->info = std::make_unique<struct addrinfo>();
		std::memset(this->address_info->info.get(), 0, sizeof(struct addrinfo));

		this->address_info->info->ai_family = domain;
		this->address_info->info->ai_socktype = type;
		this->address_info->info->ai_protocol = protocol;
		this->address_info->initialized = false;
	}

	SocketConnection::~SocketConnection() {
		close_socket(this->socket_descriptor);

		if (--SocketConnection::open_connection_count == 0) {
			while (SocketConnection::initializations-- > 0) {
				quit_socket_api();
			}
		}
	}

	void SocketConnection::initialize_api() {
		int initialize_result = initialize_socket_api();
		if (initialize_result != 0) {
			throw ApiInitializationException(std::to_string(initialize_result));
		}

		SocketConnection::initializations++;
	}

	void SocketConnection::send(const NETWORK_BYTE* bytes, NETWORK_BYTE_SIZE num_bytes) const {
		NETWORK_BYTE_SIZE num_bytes_sent = 0;

		while (num_bytes_sent < num_bytes) {
			int send_return = socket_send(
				this->socket_descriptor,
				bytes + num_bytes_sent,
				num_bytes - num_bytes_sent,
				0);

			if (send_return == SOCKET_ERROR) {
				throw SendException(std::to_string(get_previous_error_code()));
			}

			num_bytes_sent += (NETWORK_BYTE_SIZE)send_return;
		}
	}


	bool SocketConnection::receive(NETWORK_BYTE* buffer, NETWORK_BYTE_SIZE num_bytes) const {
		NETWORK_BYTE_SIZE num_bytes_recvd = 0;

		while (num_bytes_recvd < num_bytes) {
			int recv_return = recv(
				this->socket_descriptor,
				buffer + num_bytes_recvd,
				num_bytes - num_bytes_recvd,
				0);

			if (recv_return == SOCKET_ERROR) {
				throw ReceiveException(std::to_string(get_previous_error_code()));
			}
			else if (recv_return == 0) {
				return false;
			}

			num_bytes_recvd += (NETWORK_BYTE_SIZE)recv_return;
		}

		return true;
	}


	void SocketConnection::bind(std::string port, std::string address) {
		this->set_socket_info(port, address, AI_PASSIVE);

		int bind_result = bind_socket(this->socket_descriptor,
			this->address_info->info->ai_addr, (SOCKET_LEN) this->address_info->info->ai_addrlen);

		if (bind_result == SOCKET_ERROR) {
			throw BindException(std::to_string(get_previous_error_code()));
		}

	}

	void SocketConnection::connect(std::string address, std::string port) {
		this->set_socket_info(port, address, 0);

		int connect_result = connect_socket(this->socket_descriptor,
			this->address_info->info->ai_addr, (SOCKET_LEN) this->address_info->info->ai_addrlen);

		if (connect_result == SOCKET_ERROR) {
			int err = WSAGetLastError();
			throw ConnectException(std::to_string(get_previous_error_code()));
		}

	}

	void SocketConnection::listen(int queue_size) const {
		int listen_result = listen_socket(this->socket_descriptor, queue_size);

		if (listen_result == SOCKET_ERROR) {
			throw ListenException(std::to_string(get_previous_error_code()));
		}
	}

	SocketConnection_p SocketConnection::accept() const {
		struct sockaddr connection_info;
		SOCKET_LEN info_size = sizeof(connection_info);

		SOCKET connected_socket = accept_socket(this->socket_descriptor, &connection_info, &info_size);
		if (connected_socket == INVALID_SOCKET) {
			throw AcceptException(std::to_string(get_previous_error_code()));
		}

		return std::make_shared<SocketConnection>(connected_socket, connection_info.sa_family,
			this->address_info->info->ai_socktype, this->address_info->info->ai_protocol);
	}

	void SocketConnection::set_socket_info(std::string port, std::string address, int flag) {
		const char* char_addr = address.empty() ? nullptr : address.c_str();
		const char* char_port = port.c_str();
		struct addrinfo* addr_info_result;

		int getaddrinfo_return = getaddrinfo(char_addr, char_port,
			this->address_info->info.get(), &addr_info_result);

		this->address_info->info.reset(addr_info_result);
		this->address_info->initialized = true;

		if (getaddrinfo_return != 0) {
			throw GetAddrInfoException(std::to_string(get_previous_error_code()));
		}
	}
}
