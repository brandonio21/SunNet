/**
@file socket_connection.hpp
@brief A modern C++ library for interfacing with Sockets
*/
#pragma once
#include "socketutil.h"

#include <cstdint>
#include <stdexcept>
#include <memory>
#include <atomic>

namespace SunNet {


	/**
	A struct used to hold pointers to struct addrinfo types. 
	Any dynamically allocated addrinfo structs should be wrapped
	in this struct in a std::unique_ptr using addrinfo_delete as a
	custom deleter.
	*/
	struct addrinfo_data {
		bool initialized; /** < Whether the struct is a result of getaddrinfo */
		std::unique_ptr<struct addrinfo> info; /** < The dynamically allocated struct addrinfo */
	};

	/**
	A struct which acts as a std::unique_ptr custom deleter for struct addrinfo_data.
	Ensures that the user never has to manually call freeaddrinfo()
	*/
	struct addrinfo_delete {
		void operator()(struct addrinfo_data* ptr) const {
			if (ptr->initialized) {
				freeaddrinfo(ptr->info.get());
			}
		}
	};

	/**
	 An abstraction layer over sockets which supports modern C++ constructs
	 as well as memory management. All operations in the SocketConnection
	 class are synchronous. 
	 */
	class SocketConnection {

	friend struct SocketCollectionEntry_hash;
	friend class PollService;

	private:
		SOCKET socket_descriptor; /** < The underlying OS socket descriptor */
		std::unique_ptr<struct addrinfo_data, addrinfo_delete> address_info; 

		/** Keep track of the amount of open connections to automatically call initialize_socket_api
		and quit_socket_api */
		static std::atomic_uint open_connection_count;
		static std::atomic_uint initializations;

		void set_socket_info(std::string, std::string address, int flag);
		void initialize_api();

	public:
		/**
		 Construct a SocketConnection instance with domain, type, and protocol
		 details.
		 @param domain The communications domain of the socket
		 @param type the type of socket to be created
		 @param protocol The particular protocol to be used with the socket
		 @throws CannotCreateSocketException if an error occurred while creating
		 the socket
		 */
		SocketConnection(int domain, int type, int protocol);

		/**
		 Construct a SocketConnection instance from an existing
		 socket descriptor
		 @param socket_fd The file descriptor for the socket
		 @param domain The communications domain of the socket
		 @param type The type of the already existing socket
		 @param protocol The protocol of the socket
		 */
		SocketConnection(SOCKET socket_fd, int domain, int type, int protocol);

		/**
		 Destruct the socket connection, closing its socket.
		 */
		virtual ~SocketConnection();

		/**
		 Sends the requested number of bytes from the provided buffer onto the
		 wire.
		 @param bytes The buffer which data will be read from
		 @param num_bytes The number of bytes to send
		 @throws SendException if an error occurred while sending
		 */
		void send(const NETWORK_BYTE* bytes, NETWORK_BYTE_SIZE num_bytes) const;

		/**
		 Reads the number of bytes from the wire into the provided buffer
		 @param buffer The buffer to read into
		 @param num_bytes The number of bytes to read
		 @throws ReceiveException if an error occurred while receiving
		 */
		bool receive(NETWORK_BYTE* buffer, NETWORK_BYTE_SIZE num_bytes) const;

		/**
		Connect the socket to a remote socket
		@param address The address of the remote socket
		@param port The port of the remote socket
		@throws 
		*/
		void connect(std::string address, std::string port);

		/**
		Bind the socket to a specific port and address to make it ready for receiving.
		@param port The port to bind to
		@param address The address to bind to. If no address is specified, bind
		to 0.0.0.0
		@throws BindException if there is an error binding
		*/
		void bind(std::string port, std::string address="");

		/**
		Begin listening for connections, enqueueing them until they are
		popped with accept().
		@param queue The queue size for incoming connections
		@throws ListenException if there is an error listening
		*/
		void listen(int queue) const;

		/**
		Pop a waiting connection off the queue and accept it.
		@return A shared_ptr of the accepted connection
		@throws AcceptException if there is an error accepting
		*/
		std::shared_ptr<SocketConnection> accept() const;
	};

	class ApiInitializationException : public std::runtime_error {
	public:
		ApiInitializationException(std::string msg) : std::runtime_error(msg) {};
	};
	class SocketException : public std::runtime_error {
	public:
		SocketException(std::string msg) : std::runtime_error(msg) {};
	};
	class SendException : public SocketException {
	public:
		SendException(std::string msg) : SocketException(msg) {};
	};
	class ReceiveException : public SocketException {
	public:
		ReceiveException(std::string msg) : SocketException(msg) {};
	};
	class CreateException : public SocketException {
	public:
		CreateException(std::string msg) : SocketException(msg) {};
	};
	class GetAddrInfoException : public SocketException {
	public:
		GetAddrInfoException(std::string msg) : SocketException(msg) {};
	};
	class BindException : public SocketException {
	public:
		BindException(std::string msg) : SocketException(msg) {};
	};
	class ListenException : public SocketException {
	public:
		ListenException(std::string msg) : SocketException(msg) {};
	};
	class AcceptException : public SocketException {
	public:
		AcceptException(std::string msg) : SocketException(msg) {};
	};
	class ConnectException : public SocketException {
	public:
		ConnectException(std::string msg) : SocketException(msg) {};
	};

	typedef std::shared_ptr<SocketConnection> SocketConnection_p;
}
