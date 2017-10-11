/**
@file pollservice.h
@brief Definitions for PollService, a class which polls a collection of
sockets and allows action on ones which are ready to read
*/
#pragma once

#include "socket_connection.hpp"
#include "socket_collection.h"
#include <vector>
#include <unordered_map>

namespace SunNet {

	/**
	A C++ wrapper class around WinSock/Sockets poll().

	Keeps track of a collection of SocketConnection objects and allows
	operation when they are ready to be read.
	*/
	class PollService {
	private:
		int timeout; /** < How long to wait before declaring no socket can be read */

		std::vector<POLL_DESCRIPTOR> descriptors; /** < The corresponding poll() descriptors */
		std::unordered_map<SOCKET, std::pair<int, SocketConnection_p>> poll_descriptor_map; /** < Points to elements in descriptors for fast updating */

		std::shared_ptr<SocketCollection> results;
	public:

		PollService(int timeout = 10);

		PollService(const SocketConnection_p socket, int timeout = 10);

		/**
		Create a PollService object from a collection of sockets.

		@param sockets The sockets to monitor
		@param timeout how long to wait before declaring no socket can be read (in ms)
		*/
		template <class Iter>
		PollService(Iter& begin, Iter& end, int timeout = 10) : PollService(timeout) {
			this->add_sockets(begin, end);
		}

		void add_socket(const SocketConnection_p socket);

		/**
		Adds a collection of sockets to the poll's collection of sockets.

		@param sockets The sockets to add
		*/
		template <class Iter>
		void add_sockets(Iter& begin, Iter& end) {
			for (auto it = begin, it != end; ++it) {
				this->add_socket(*it);
			}
		}

		void remove_socket(const SocketConnection_p socket);

		void clear_sockets();

		/**
		Poll all sockets, returning those ready to be read

		@throws PollException if there was an error with the OS level poll()
		@throws PollReturnEventException if a specific socket encountered an error
		@throws InvalidSocketConnectionException if a watched socket's 
		corresponding descriptor could not be found
		@return A collection of sockets ready to be read
		*/
		SocketCollection_p poll();
	};

	class PollException : public SocketException {
	public:
		PollException(std::string msg) : SocketException(msg) {};
	};

	class InvalidSocketConnectionException : public PollException {
	private:
		SOCKET descriptor;
	public:
		InvalidSocketConnectionException(std::string msg, SOCKET descriptor) : PollException(msg) {
			this->descriptor = descriptor;
		}

		SOCKET get_descriptor() {
			return this->descriptor;
		}
	};

	class PollReturnEventException : public PollException {
	private:
		SocketConnection_p socket;
	public:
		PollReturnEventException(std::string msg, SocketConnection_p socket) : PollException(msg) {
			this->socket = socket;
		}

		SocketConnection_p get_socket() {
			return this->socket;
		}
	};
}

