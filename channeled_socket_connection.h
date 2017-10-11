#pragma once
#include "socket_connection.hpp"
#include "channels.h"

namespace SunNet {
	/**
	A wrapper for SocketConnection which operates on channels. Users should use 
	ChanneledSocketConnections only to interact with other ChanneledSocketConnections.

	Thus, when using a ChanneledSocketConnection, the user should prefer the channeled_*
	methods for sending and receiving
	*/
	class ChanneledSocketConnection : public SocketConnection {
	public:
		ChanneledSocketConnection(int domain, int type, int protocol) :
			SocketConnection(domain, type, protocol) {}

		ChanneledSocketConnection(SOCKET socket_fd, int domain, int type, int protocol) :
			SocketConnection(socket_fd, domain, type, protocol) {}

		/**
		Send a message along a channel. The channel id is deduced from the template
		parameter

		@param message The message to send
		*/
		template <typename TMessageType>
		void channeled_send(TMessageType* message) {
			CHANNEL_ID channel_id = Channels::getChannelId<TMessageType>();

			this->send((NETWORK_BYTE*)&channel_id, sizeof(CHANNEL_ID));
			this->send((NETWORK_BYTE*)message, sizeof(TMessageType));
		}
		
		/**
		Read the channel id from the connection.
		*/
		CHANNEL_ID channeled_read_id() {
			CHANNEL_ID channel_id;

			if (!this->receive(&channel_id, sizeof(CHANNEL_ID))) {
				throw ConnectionClosedException();
			}

			return channel_id;
		}

		/**
		Read a message from the channel. The number of bytes to read is determined
		by the channel type send via the template parameter
		
		@return A unique_ptr to the read bytes
		*/
		template <typename TMessageType>
		std::unique_ptr<NETWORK_BYTE[]> channeled_read() {
			return this->read(Channels::getChannelId<TMessageType>());
		}

		/**
		Read a message from the channel. The number of bytes to read is determined
		by the channel id sent in

		@param id The id of the channel to read from
		@return A unique_ptr to the read bytes
		*/
		std::unique_ptr<NETWORK_BYTE[]> channeled_read(CHANNEL_ID id) {
			std::shared_ptr<ChannelInterface> channel = Channels::getChannel(id);
			std::unique_ptr<NETWORK_BYTE[]> data = std::make_unique<NETWORK_BYTE[]>(channel->getMessageSize());

			if (!this->receive(data.get(), channel->getMessageSize())) {
				throw ConnectionClosedException();
			}

			/* 
			Move ownership to the caller. If the caller does not capture this,
			the bytes will be freed at the end of this method
			*/
			return data;
		}

		class ConnectionClosedException : std::exception {};
	};

	typedef std::shared_ptr<ChanneledSocketConnection> ChanneledSocketConnection_p;
}
