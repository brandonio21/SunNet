#include "socketutil.h"

namespace SunNet {

	int initialize_socket_api() {
#ifdef _WIN32
		WSAData wsaData;
		return WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
		return 0;
#endif
	}

	int quit_socket_api() {
#ifdef _WIN32
		return WSACleanup();
#else
		return 0;
#endif
	}

	SOCKET open_socket(int domain, int type, int protocol) {
		SOCKET socket_response = socket(domain, type, protocol);
#ifdef _WIN32
		return socket_response;
#else
		if (socket_response < 0) {
			return INVALID_SOCKET;
		}
		else {
			return socket_response;
		}
#endif
	}

	int close_socket(SOCKET socket) {
		int status = 0;
#ifdef _WIN32
		status = shutdown(socket, SD_BOTH);
		if (status == 0) {
			status = closesocket(socket);
		}
#else
		status = shutdown(socket, SHUT_RDWR);
		if (status == 0) {
			status = close(sock);
		}
#endif

		return status;
	}

	int get_previous_error_code() {
#ifdef _WIN32
		return WSAGetLastError();
#else
		return errno;
#endif
	}

	int bind_socket(SOCKET socket, const struct sockaddr* addr, SOCKET_LEN len) {
		return bind(socket, addr, len);
	}

	int listen_socket(SOCKET socket, int backlog) {
		return listen(socket, backlog);
	}

	SOCKET accept_socket(SOCKET socket, struct sockaddr* addr, SOCKET_LEN* len) {
		return accept(socket, addr, len);
	}

	int connect_socket(SOCKET socket, const struct sockaddr* addr, SOCKET_LEN len) {
		return connect(socket, addr, len);
	}

	int socket_send(SOCKET socket, const NETWORK_BYTE* buffer, NETWORK_BYTE_SIZE len, int flags) {
		return send(socket, buffer, len, flags);
	}

	int socket_receive(SOCKET socket, NETWORK_BYTE* buffer, NETWORK_BYTE_SIZE len, int flags) {
		return recv(socket, buffer, len, flags);
	}

	int socket_poll(POLL_DESCRIPTOR* descriptors, NUM_POLL_DESCRIPTORS count, int timeout) {
#ifdef _WIN32
		return WSAPoll(descriptors, count, timeout);
#else
		return poll(descriptors, count, timeout);
#endif
	}

}
