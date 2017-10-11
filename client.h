#pragma once
#include "socket_connection.hpp"
#include "pollservice.h"
#include <vector>
#include <thread>
#include <initializer_list>

namespace SunNet {
	/**
	A collection of states that the client can be in.
		+---+
		|   V
	  +--------+        +-----------+         +-------------+
	->| CLOSED |  ----> | CONNECTED |  ---->  | DESTRUCTING |
	  +--------+        +-----------+         +-------------+
		  ^                   |
		  +-------------------+
	*/
	enum ClientState {
		CLIENT_CLOSED, /** < When the client is created, but not yet connected to a server */
		CLIENT_CONNECTED, /** < When a client is connected to a server and actively polling the server */

		CLIENT_DESTRUCTING /** < When a client is being destructed and must cease operations */
	};

	/**
	A class which represents a client, which can connect to a server, send, and receive
	data. The client also polls the server connection and has virtual "hooks" that the
	inheritor can implement to their liking.

	The client operates in two-threads - a main thread and a thread for polling. This
	makes things a bit scary. Try not to touch the internals of the client unless you
	want to deal with possible race conditions (vrrrrrm)
	*/
	template <class TSocketConnection>
	class Client {
	private:
		PollService poll_service;

		ClientState state;

		void assert_valid_state(std::initializer_list<ClientState> const& valid_states) {
			bool is_valid_state = false;
			for (const ClientState& valid_state : valid_states) {
				if (this->state == valid_state) {
					is_valid_state = true;
					break;
				}
			}

			if (!is_valid_state) {
				throw InvalidStateTransitionException();
			}
		}

		void state_transition(std::initializer_list<ClientState> const & valid_from_states, ClientState new_state) {
			assert_valid_state(valid_from_states);
			this->state = new_state;
		}

		/*
		Since we only know how to construct a connection at Client construction time,
		we save the construction details in this function for when we re-open
		the connection
		*/
		std::function<SocketConnection_p()> connection_create_func;

		

	protected:
		std::shared_ptr<SocketConnection> connection;

		/* Hooks to be implemented by the inheritor */
		virtual void handle_client_error() = 0;
		virtual void handle_client_ready_to_read() = 0;
		virtual void handle_poll_timeout() = 0;
		virtual void handle_client_disconnect() = 0;

	public:
		template <class ... ArgTypes>
		Client(int poll_timeout, ArgTypes ... args) : state(CLIENT_CLOSED) {
			this->connection_create_func = []() { return std::make_shared<TSocketConnection>(args...); };
			this->poll_service = PollService(poll_timeout);
		}

		/**
		Destruct the client, doing the following:
		1. Change state (if necessary)
		2. Stop the polling thread (if necessary)
		3. Close the connection
		*/
		virtual ~Client() {
			/* 
			We don't care about properly disconnecting. Force a
			state change instead. This will stop the poll thread.

			The state might not be CLOSED. The user should have called disconnect()
			prior to the client being destructed. We will do our best, though. We will
			signal the polling thread to stop. If the polling thread calls any virtual
			methods, C++ will except us and we're doomed. Hopefully the polling thread
			hits a "checkpoint" and bails out before it calls any virtual methods.
			*/
			this->state = CLIENT_DESTRUCTING;

			/* 
			Join the poll thread if need be. Please god don't let it call any virtual
			methods... If it does, we crash.
			*/

			/* Now, reset the connection. */
			this->connection.reset();
		}

		/**
		Connect the underlying connection to the given address and port.
		If the connection succeeds, change state and begin polling

		@param address the address to connect to
		@param port the port to connect to
		*/
		void connect(std::string address, std::string port) {
			this->assert_valid_state({ CLIENT_CLOSED });

			this->connection = this->connection_create_func();
			this->connection->connect(address, port);
			this->poll_service.add_socket(this->connection);

			this->state_transition({ CLIENT_CLOSED }, CLIENT_CONNECTED);
		}

		/**
		Forcibly disconnect. this involves changing state, joining the poll thread,
		and closing the connection. If the client is in a non-connected state, this
		will throw.

		@throws InvalidStateTransitionException if the client is not connected
		*/
		void disconnect() {
			this->state_transition({ CLIENT_CONNECTED, CLIENT_CLOSED }, CLIENT_CLOSED);

			/* 
			It is possible that disconnect() is being called from the poll_thread.
			We should only join if that is _not_ the case.


			Also no rush. The polling thread will not bail out at any checkpoints.
			The user is properly disconnecting us, so let's take our sweet time :)
			*/

			this->poll_service.clear_sockets();

			/* Bye bye, connection. */
			this->connection.reset();
		}

		/**
		Send data along the connection

		@param buffer The bytes to send
		@param size the number of bytes to send
		*/
		void send(const NETWORK_BYTE* buffer, NETWORK_BYTE_SIZE size) {
			this->connection->send(buffer, size);
		}

		/**
		Reead data from the connection

		@param buffer The buffer to read into
		@param size The number of bytes to read
		*/
		void read(NETWORK_BYTE* buffer, NETWORK_BYTE_SIZE size) {
			this->connection->receive(buffer, size);
		}

		/**
		The function that the polling thread uses. While the client is
		connected, polls the connection looking for status updates. When something
		is ready to occur, the corresponding "hook" is called.

		Handles poll timeouts, client errors, and whent he client is ready
		to read data from the server

		@throws InvalidSocketPollException if a catastrophic errors occurs and for whatever
		reason poll() returns a socket that we don't know about.
		*/
		bool poll() {
			if (this->state != CLIENT_CONNECTED) {
				return false;
			}
			SocketCollection_p ready_sockets = this->poll_service.poll();

			if (ready_sockets->size() == 0) {
				/* CHECKPOINT */
				if (this->state == CLIENT_DESTRUCTING) return false;
				this->handle_poll_timeout();
				return false;
			}

			for (auto socket_iter = ready_sockets->begin(); socket_iter != ready_sockets->end(); ++socket_iter) {

				if (socket_iter->connection == this->connection) {
					if (socket_iter->status == SOCKET_STATUS_ERROR) {
						/* CHECKPOINT */
						if (this->state == CLIENT_DESTRUCTING) return false;
						this->handle_client_error();
					}
					else if (socket_iter->status == SOCKET_STATUS_DISCONNECT) {
						/* CHECKPOINT */
						if (this->state == CLIENT_DESTRUCTING) return false;
						this->handle_client_disconnect();
					}
					else {
						/* CHECKPOINT */
						if (this->state == CLIENT_DESTRUCTING) return false;
						this->handle_client_ready_to_read();
					}
				}
				else {
					throw InvalidSocketPollException();
				}
			}
			return true;
		}


		class ClientException : public std::exception {};
		class InvalidStateTransitionException : public ClientException {};
		class InvalidSocketPollException : public ClientException {};
	};
}
