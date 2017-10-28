/** @file socketutil.h
@brief Operating system compatability layer for BSD Sockets functions

Contains macros and functions that act as a compatibility layer for
Windows/Linux in utilizing sockets and WinSock APIs
*/
#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define SOCKET_API_NOT_INITIALIZED 10093

typedef int SOCKET_LEN;
typedef char NETWORK_BYTE;
typedef int NETWORK_BYTE_SIZE;
typedef WSAPOLLFD POLL_DESCRIPTOR;
typedef ULONG NUM_POLL_DESCRIPTORS;

#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <netdb.h>
#include <unistd.h>

#define SOCKET_API_NOT_INITIALIZED -1

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
typedef socklen_t SOCKET_LEN;
typedef unsigned char NETWORK_BYTE;
typedef size_t NETWORK_BYTE_SIZE;
typedef struct pollfd POLL_DESCRIPTOR;
typedef nfds_t NUM_POLL_DESCRIPTORS;
#endif

namespace SunNet {

	/**
	Initializes the OS-specific sockets API. Must be called before any other
	sockets functions.

	@return 0 if initialized without error; negative otherwise.
	*/
	int initialize_socket_api();

	/**
	Quits the OS-specific sockets API. Must be called after all sockets functions
	are complete. After calling, socket functions will no longer work until
	initialize_socket_api is called again.

	@return 0 if quit without error; negative otherwise.
	*/
	int quit_socket_api();

	/**
	Opens a new socket. 
	
	@param domain The socket domain
	@param type The socket type
	@param protocol The socket protocol
	@return A new SOCKET. If an error occurred, returns INVALID_SOCKET
	*/
	SOCKET open_socket(int domain, int type, int protocol);

	/**
	Closes an already opened socket.
	
	@param socket The socket to close
	@return A negative integer if the socket could not be closed; otherwise, 0
	*/
	int close_socket(SOCKET socket);

	/**
	Binds the socket to the given address so that it may
	later accept connections.

	@param socket The socket to bind
	@param addr The address to bind the socket to
	@param len The length of the addr parameter
	@return A status integer. If an error occured, SOCKET_ERROR
	*/
	int bind_socket(SOCKET socket, const struct sockaddr* addr, SOCKET_LEN len);

	/**
	Listens on the socket for incoming connections.

	@param socket The socket to listen on
	@param backlog The amount of connections to keep in the queue
	@return A status integer. If an error occured, SOCKET_ERROR
	*/
	int listen_socket(SOCKET socket, int backlog);

	/**
	Accepts a queued connection so that it may be read from and written to.

	@param socket The socket to accept the connection on
	@param addr A pointer to where the accepted connection's info will be stored
	@param len A pointer to where the length of the acception connection's
	address length will be stored
	@return A SOCKET which is the accepted connection. INVALID_SOCKET if an error
	occurred.
	*/
	SOCKET accept_socket(SOCKET socket, struct sockaddr* addr, SOCKET_LEN* len);

	/**
	Connects a socket to a remote socket so that it may write and read from the
	remote socket.

	@param socket The socket to connect from
	@param addr The address of the socket to connect to
	@param len The len of the addr parameter
	@return A status integer. SOCKET_ERROR if an error occured
	*/
	int connect_socket(SOCKET socket, const struct sockaddr* addr, SOCKET_LEN len);

	/**
	Sends bytes through the socket, which will arrive at the connected socket.
	This function will block until the entirety of the bytes are sent.

	@param socket The socket to send from
	@param buffer The buffer of bytes to send from
	@param len The amount of bytes to send
	@param flags Flags that will be sent to the underlying sockets send() call
	@return A status integer. SOCKET_ERROR if an error occured.
	*/
	int socket_send(SOCKET socket, const NETWORK_BYTE* buffer, NETWORK_BYTE_SIZE len, int flags);

	/**
	Receives bytes from the socket. This function will block until the
	entirety of the bytes are received.

	@param socket The socket to receive from
	@param buffer The buffer to write into
	@param len The number of bytes to read
	@param flags Flags that will be sent to the underlying sockets recv() call
	@return A status integer. SOCKET_ERROR if an error occured. 
	0 If the connection was closed properly before the message finished being read.
	*/
	int socket_receive(SOCKET socket, NETWORK_BYTE* buffer, NETWORK_BYTE_SIZE len, int flags);

	/**
	Polls for the status of one or more sockets (blocking if necessary) to perform
	synchronous IO

	@param descriptors The set of descriptors to poll
	@param count The number of descriptors to poll
	@param timeout The poll timeout
	@return The number of sockets in which events occurred; SOCKET_ERROR if error
	*/
	int socket_poll(POLL_DESCRIPTOR* descriptors, NUM_POLL_DESCRIPTORS count, int timeout);

	/**
	Returns the error code of the most recent error. This is to be used
	when a function outputs SOCKET_ERROR or INVALID_SOCKET and may be
	used to display error messages or debug. 
	@return The error code of the most recent error
	*/
	int get_previous_error_code();
}
